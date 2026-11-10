"""Replace Windows icon resources in a copied runtime; verify executable code.

Uses only Python's standard library and the Windows resource API. Never edits
the supplied source executable. The PE code/data sections must remain identical.
"""
import argparse
import ctypes
from ctypes import wintypes
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import tempfile


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def pe_sections(path):
    data = Path(path).read_bytes()
    if data[:2] != b'MZ':
        raise ValueError('Input is not a Windows executable')
    pe = struct.unpack_from('<I', data, 0x3C)[0]
    if data[pe:pe + 4] != b'PE\0\0':
        raise ValueError('Invalid PE signature')
    count = struct.unpack_from('<H', data, pe + 6)[0]
    optional_size = struct.unpack_from('<H', data, pe + 20)[0]
    result = {}
    for index in range(count):
        offset = pe + 24 + optional_size + index * 40
        name = data[offset:offset + 8].rstrip(b'\0').decode('ascii')
        virtual_size, _, raw_size, raw_offset = struct.unpack_from('<IIII', data, offset + 8)
        if name != '.rsrc':
            result[name] = (virtual_size, data[raw_offset:raw_offset + raw_size])
    return result


def api():
    if os.name != 'nt':
        raise OSError('Windows resource stamping requires Windows')
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.LoadLibraryExW.argtypes = [wintypes.LPCWSTR, wintypes.HANDLE, wintypes.DWORD]
    kernel.LoadLibraryExW.restype = wintypes.HANDLE
    kernel.FreeLibrary.argtypes = [wintypes.HMODULE]
    kernel.FreeLibrary.restype = wintypes.BOOL
    kernel.FindResourceExW.argtypes = [wintypes.HMODULE, ctypes.c_void_p, ctypes.c_void_p, wintypes.WORD]
    kernel.FindResourceExW.restype = wintypes.HANDLE
    kernel.SizeofResource.argtypes = [wintypes.HMODULE, wintypes.HANDLE]
    kernel.SizeofResource.restype = wintypes.DWORD
    kernel.LoadResource.argtypes = [wintypes.HMODULE, wintypes.HANDLE]
    kernel.LoadResource.restype = wintypes.HANDLE
    kernel.LockResource.argtypes = [wintypes.HANDLE]
    kernel.LockResource.restype = ctypes.c_void_p
    kernel.BeginUpdateResourceW.argtypes = [wintypes.LPCWSTR, wintypes.BOOL]
    kernel.BeginUpdateResourceW.restype = wintypes.HANDLE
    kernel.UpdateResourceW.argtypes = [wintypes.HANDLE, ctypes.c_void_p, ctypes.c_void_p,
                                      wintypes.WORD, ctypes.c_void_p, wintypes.DWORD]
    kernel.UpdateResourceW.restype = wintypes.BOOL
    kernel.EndUpdateResourceW.argtypes = [wintypes.HANDLE, wintypes.BOOL]
    kernel.EndUpdateResourceW.restype = wintypes.BOOL
    return kernel


def resource_arg(name):
    if isinstance(name, int):
        return ctypes.c_void_p(name)
    return ctypes.cast(ctypes.c_wchar_p(name), ctypes.c_void_p)


def resource_name(value):
    return value if value <= 65535 else ctypes.wstring_at(value)


def resources(path):
    """Read every resource, also used to verify non-icon resources survive."""
    kernel = api()
    module = kernel.LoadLibraryExW(str(Path(path).resolve()), None, 0x02 | 0x20)
    if not module:
        raise ctypes.WinError(ctypes.get_last_error())
    result = {}
    errors = []
    TypeCallback = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HMODULE,
                                     ctypes.c_void_p, ctypes.c_ssize_t)
    NameCallback = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HMODULE,
                                     ctypes.c_void_p, ctypes.c_void_p, ctypes.c_ssize_t)
    LangCallback = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HMODULE,
                                     ctypes.c_void_p, ctypes.c_void_p, wintypes.WORD,
                                     ctypes.c_ssize_t)
    kernel.EnumResourceTypesW.argtypes = [wintypes.HMODULE, TypeCallback, ctypes.c_ssize_t]
    kernel.EnumResourceTypesW.restype = wintypes.BOOL
    kernel.EnumResourceNamesW.argtypes = [wintypes.HMODULE, ctypes.c_void_p, NameCallback, ctypes.c_ssize_t]
    kernel.EnumResourceNamesW.restype = wintypes.BOOL
    kernel.EnumResourceLanguagesW.argtypes = [wintypes.HMODULE, ctypes.c_void_p,
                                             ctypes.c_void_p, LangCallback, ctypes.c_ssize_t]
    kernel.EnumResourceLanguagesW.restype = wintypes.BOOL

    @LangCallback
    def language_callback(mod, kind, name, language, context):
        resource = kernel.FindResourceExW(mod, kind, name, language)
        if not resource:
            errors.append(ctypes.get_last_error())
            return False
        size = kernel.SizeofResource(mod, resource)
        pointer = kernel.LockResource(kernel.LoadResource(mod, resource))
        if not pointer and size:
            errors.append(ctypes.get_last_error())
            return False
        result[(resource_name(kind), resource_name(name), language)] = ctypes.string_at(pointer, size)
        return True

    @NameCallback
    def name_callback(mod, kind, name, context):
        if not kernel.EnumResourceLanguagesW(mod, kind, name, language_callback, 0):
            errors.append(ctypes.get_last_error())
            return False
        return True

    @TypeCallback
    def type_callback(mod, kind, context):
        if not kernel.EnumResourceNamesW(mod, kind, name_callback, 0):
            errors.append(ctypes.get_last_error())
            return False
        return True

    try:
        if not kernel.EnumResourceTypesW(module, type_callback, 0):
            errors.append(ctypes.get_last_error())
    finally:
        kernel.FreeLibrary(module)
    if errors:
        raise ctypes.WinError(errors[0])
    return result


def icon_frames(path):
    data = Path(path).read_bytes()
    reserved, kind, count = struct.unpack_from('<HHH', data)
    if reserved != 0 or kind != 1 or not 1 <= count <= 256:
        raise ValueError('Invalid Windows ICO header')
    frames = []
    for index in range(count):
        entry = data[6 + 16 * index:22 + 16 * index]
        width, height, colors, reserved, planes, bits, size, offset = struct.unpack('<BBBBHHII', entry)
        payload = data[offset:offset + size]
        if len(payload) != size or offset < 6 + 16 * count:
            raise ValueError('Invalid ICO frame bounds')
        frames.append((struct.pack('<BBBBHHI', width, height, colors, reserved, planes, bits, size), payload))
    return frames


def stamp_icon(source, destination, icon):
    source, destination, icon = map(lambda p: Path(p).resolve(), (source, destination, icon))
    if source == destination:
        raise ValueError('Source and destination must differ; source is read-only')
    before_hash = sha256(source)
    before_sections = pe_sections(source)
    before_resources = resources(source)
    frames = icon_frames(icon)
    group = struct.pack('<HHH', 0, 1, len(frames))
    canonical_icons = {}
    for number, (entry, payload) in enumerate(frames, start=1):
        canonical_icons[(3, number, 0)] = payload
        group += entry + struct.pack('<H', number)
    canonical_icons[(14, 1, 0)] = group
    destination.parent.mkdir(parents=True, exist_ok=True)
    source_icons = {key: payload for key, payload in before_resources.items()
                    if key[0] in (3, 14)}
    if source_icons == canonical_icons:
        # The required neutral group and every frame are already present.
        # Compare complete files: equal sections alone cannot validate headers.
        reused = destination.is_file() and sha256(destination) == before_hash
        if not reused:
            shutil.copy2(source, destination)
        if sha256(destination) != before_hash:
            raise RuntimeError('Copied runtime did not match the verified source')
        if pe_sections(destination) != before_sections:
            raise RuntimeError('Code/data sections changed during runtime copy')
        if resources(destination) != before_resources:
            raise RuntimeError('Resources changed during runtime copy')
        if sha256(source) != before_hash:
            raise RuntimeError('Original runtime unexpectedly changed')
        return {
            'source_sha256': before_hash,
            'stamped_sha256': before_hash,
            'icon_sha256': sha256(icon),
            'icon_frames': len(frames),
            'unchanged_sections': sorted(before_sections),
            'non_icon_resources_preserved': True,
            'icon_resources_reextracted_and_verified': True,
            'original_unchanged': True,
            'changed': not reused,
            'reused': reused,
            'resource_edit_performed': False,
        }
    fd, temporary_name = tempfile.mkstemp(prefix='icon-', suffix='.exe', dir=destination.parent)
    os.close(fd)
    temporary = Path(temporary_name)
    try:
        shutil.copy2(source, temporary)
        kernel = api()
        handle = kernel.BeginUpdateResourceW(str(temporary), False)
        if not handle:
            raise ctypes.WinError(ctypes.get_last_error())
        committed = False
        try:
            # The host requests group 1 for both SDL icon sizes. Rebuild all
            # icon resources, keeping version/manifests/other resource types.
            for kind, name, language in before_resources:
                if kind in (3, 14):
                    if not kernel.UpdateResourceW(handle, resource_arg(kind), resource_arg(name),
                                                  language, None, 0):
                        raise ctypes.WinError(ctypes.get_last_error())
            for number, (entry, payload) in enumerate(frames, start=1):
                buffer = ctypes.create_string_buffer(payload)
                if not kernel.UpdateResourceW(handle, resource_arg(3), resource_arg(number),
                                              0, buffer, len(payload)):
                    raise ctypes.WinError(ctypes.get_last_error())
            buffer = ctypes.create_string_buffer(group)
            if not kernel.UpdateResourceW(handle, resource_arg(14), resource_arg(1), 0, buffer, len(group)):
                raise ctypes.WinError(ctypes.get_last_error())
            if not kernel.EndUpdateResourceW(handle, False):
                raise ctypes.WinError(ctypes.get_last_error())
            committed = True
        finally:
            if not committed:
                kernel.EndUpdateResourceW(handle, True)
        if pe_sections(temporary) != before_sections:
            raise RuntimeError('Code/data sections changed; refusing the modified runtime')
        after_resources = resources(temporary)
        for key, payload in before_resources.items():
            if key[0] not in (3, 14) and after_resources.get(key) != payload:
                raise RuntimeError('A non-icon resource changed')
        for number, (_, payload) in enumerate(frames, start=1):
            if after_resources.get((3, number, 0)) != payload:
                raise RuntimeError('Re-extracted icon frame did not match the source ICO')
        if after_resources.get((14, 1, 0)) != group:
            raise RuntimeError('Re-extracted icon group did not match the source ICO')
        if sha256(source) != before_hash:
            raise RuntimeError('Original runtime unexpectedly changed')
        os.replace(temporary, destination)
        return {
            'source_sha256': before_hash,
            'stamped_sha256': sha256(destination),
            'icon_sha256': sha256(icon),
            'icon_frames': len(frames),
            'unchanged_sections': sorted(before_sections),
            'non_icon_resources_preserved': True,
            'icon_resources_reextracted_and_verified': True,
            'original_unchanged': True,
            'changed': True,
            'reused': False,
            'resource_edit_performed': True,
        }
    finally:
        temporary.unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime-source', type=Path, required=True)
    parser.add_argument('--destination', type=Path, required=True)
    parser.add_argument('--icon', type=Path, required=True)
    args = parser.parse_args()
    source, destination = args.runtime_source.resolve(), args.destination.resolve()
    if source == destination or source in destination.parents:
        raise ValueError('Use a separate staging destination outside the source runtime')
    required = ('rekhcom64.exe', 'guest-us.bin', 'SDL3.dll')
    for name in required:
        if not (source / name).is_file():
            raise FileNotFoundError(source / name)
    destination.mkdir(parents=True, exist_ok=True)
    report = stamp_icon(source / required[0], destination / required[0], args.icon)
    for name in required[1:]:
        shutil.copy2(source / name, destination / name)
    shutil.copytree(source / 'assets', destination / 'assets', dirs_exist_ok=True)
    distribution_licenses = source.parent / 'licenses'
    target_licenses = destination.parent / 'licenses'
    if distribution_licenses.is_dir() and distribution_licenses.resolve() != target_licenses.resolve():
        shutil.copytree(distribution_licenses, target_licenses, dirs_exist_ok=True)
    bmp = args.icon.parent / 'kingdom-hearts-chain-of-memories-zwebti-mix-icon.bmp'
    if bmp.is_file():
        shutil.copy2(bmp, destination / 'assets/icon.bmp')
    report['guest_unchanged'] = sha256(source / 'guest-us.bin') == sha256(destination / 'guest-us.bin')
    report['sdl_unchanged'] = sha256(source / 'SDL3.dll') == sha256(destination / 'SDL3.dll')
    report_path = destination.parent / 'runtime-icon-verification.json'
    report_path.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
