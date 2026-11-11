"""Build or reuse a verified single Windows launcher. No ROMs or saves."""
import argparse
import io
import os
from pathlib import Path
import shlex
import subprocess
import sys
import zipfile
import zlib

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'port/windows64'))
import build_cache as cache
from stamp_runtime_icon import stamp_icon

VERSION = '0.11.0'
BRAND = 'kingdom-hearts-chain-of-memories-zwebti-mix'
RELEASE = 'Kingdom-Hearts-Chain-of-Memories-Zwebti-Mix'
DEVELOPMENT = ROOT / 'build/Kingdom-Hearts-Chain-of-Memories-Zwebti-Mix-Development'
INTERMEDIATE = DEVELOPMENT / 'launcher'
DEFAULT_OUTPUT = INTERMEDIATE / 'out'
COMPILER = Path('C:/Windows/Microsoft.NET/Framework64/v4.0.30319/csc.exe')
EXE_NAME = 'Kingdom Hearts Chain of Memories - Zwebti Mix.exe'
STAMPED_NAME = 'Kingdom-Hearts-Chain-of-Memories-Zwebti-Mix-stamped-runtime.exe'
REFERENCES = ('System.Windows.Forms.dll', 'System.Drawing.dll',
              'System.IO.Compression.dll', 'System.IO.Compression.FileSystem.dll',
              'System.Web.Extensions.dll')


def archive_files(runtime_source):
    mod = ROOT / 'mod/action-edition'
    files = {name: mod / name for name in
             ('launch.ps1', 'PLAY.cmd', 'kh_com.ini', 'START_HERE.html', 'README.md')}
    for suffix in ('-logo.png', '-icon.png', '-icon.bmp', '.ico'):
        files['branding/' + BRAND + suffix] = mod / 'branding' / (BRAND + suffix)
    for name in ('rekhcom64.exe', 'guest-us.bin', 'SDL3.dll'):
        files['runtime/' + name] = runtime_source / name
    for path in (runtime_source / 'assets').rglob('*'):
        if path.is_file():
            files['runtime/assets/' + path.relative_to(runtime_source / 'assets').as_posix()] = path
    # Explicit overrides exclude stale copies from the effective input manifest.
    files['runtime/assets/icon.bmp'] = mod / 'branding' / (BRAND + '-icon.bmp')
    files['runtime/assets/branding/' + BRAND + '-logo.png'] = mod / 'branding' / (BRAND + '-logo.png')
    for path in (runtime_source.parent / 'licenses').glob('*'):
        if path.is_file():
            files['licenses/' + path.name] = path
    files['licenses/ReKHCoM-CC0.md'] = ROOT / 'LICENSE.md'
    return files


def validate_files(files):
    for name, path in files.items():
        if not path.is_file():
            raise SystemExit('Missing runtime input: ' + str(path))
        if path.suffix.lower() in ('.gba', '.sav', '.state', '.kst', '.khst', '.dmp'):
            raise SystemExit('Personal game data cannot be packaged: ' + name)


def compiler_inputs(compiler, environment):
    """Resolve the standard Framework compiler's explicit and implicit inputs.

    The supported default response files contain references, not extra source
    or resource options. Fail closed if that contract changes rather than cache
    an untracked compiler input. Every existing search candidate is hashed.
    """
    paths = [compiler]
    reference_names = set(REFERENCES) | {'mscorlib.dll'}
    response_paths = []
    for response in (compiler.with_suffix('.rsp'), ROOT / 'csc.rsp'):
        if response in response_paths or not response.is_file():
            continue
        response_paths.append(response)
        paths.append(response)
        for line in response.read_text(encoding='utf-8-sig').splitlines():
            if not line.strip() or line.lstrip().startswith('#'):
                continue
            for token in shlex.split(line, posix=False):
                prefix, separator, value = token.partition(':')
                if separator and prefix.lower() in ('/r', '/reference', '-r', '-reference'):
                    reference_names.update(part.strip('"') for part in value.split(','))
                else:
                    raise ValueError('Untracked compiler response option: ' + token)
    config = compiler.with_name(compiler.name + '.config')
    if config.is_file():
        paths.append(config)
    folders = [ROOT, compiler.parent]
    for value in environment.get('LIB', '').split(os.pathsep):
        if value:
            folder = Path(value)
            folders.append(folder if folder.is_absolute() else ROOT / folder)
    reference_map = []
    for name in sorted(reference_names, key=str.casefold):
        reference = Path(name)
        candidates = ([reference] if reference.is_absolute() else
                      [folder / reference for folder in folders])
        found = sorted({str(path.resolve()): path for path in candidates if path.is_file()}.items())
        if not found:
            raise FileNotFoundError('Compiler reference could not be resolved: ' + name)
        paths.extend(path for _, path in found)
        reference_map.append((name, [identity for identity, _ in found]))
    return paths, reference_map


def compiler_command(output, archive):
    brand = ROOT / 'mod/action-edition/branding' / BRAND
    return [str(COMPILER), '/nologo', '/target:winexe', '/platform:x64', '/optimize+',
            '/out:' + str(output / EXE_NAME), '/win32icon:' + str(brand) + '.ico',
            *('/r:' + name for name in REFERENCES),
            '/resource:' + str(archive) + ',release.zip',
            '/resource:' + str(brand) + '-logo.png,logo.png',
            '/resource:' + str(brand) + '.ico,heart.ico',
            str(ROOT / 'launcher/Launcher.cs'), str(ROOT / 'launcher/ReleaseStore.cs')]


def input_key(runtime_source, command, environment):
    files = archive_files(runtime_source)
    validate_files(files)
    tools, references = compiler_inputs(COMPILER, environment)
    dependencies = [Path(__file__), Path(cache.__file__),
                    Path(__file__).with_name('stamp_runtime_icon.py'),
                    ROOT / 'launcher/Launcher.cs', ROOT / 'launcher/ReleaseStore.cs',
                    *files.values(), *tools]
    recipe = {
        'kind': 'Kingdom Hearts Chain of Memories: Zwebti Mix Windows launcher',
        'version': VERSION, 'brand': BRAND, 'release': RELEASE,
        'archive_files': [(name, str(path.resolve())) for name, path in sorted(files.items())],
        'compiler_command': command, 'compiler_cwd': str(ROOT.resolve()),
        'reference_candidates': references,
        'zip': {'date_time': [1980, 1, 1, 0, 0, 0], 'create_system': 3,
                'external_attr': 0o100644 << 16, 'compression': 'deflate', 'level': 9},
        'python': sys.version, 'zlib': zlib.ZLIB_RUNTIME_VERSION,
    }
    key = cache.content_key(dependencies, recipe=recipe,
                            environment={name: environment.get(name, '') for name in ('LIB', 'TEMP', 'TMP')})
    return key, files


def deterministic_archive(files):
    buffer = io.BytesIO()
    with zipfile.ZipFile(buffer, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as package:
        for name, path in sorted(files.items()):
            info = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
            info.create_system = 3
            info.external_attr = 0o100644 << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            package.writestr(info, path.read_bytes(), compress_type=zipfile.ZIP_DEFLATED,
                             compresslevel=9)
    return buffer.getvalue()


def build(runtime_source, output, force=False):
    runtime_source, output = Path(runtime_source).resolve(), Path(output).resolve()
    archive = INTERMEDIATE / 'release.zip'
    stamped_host = INTERMEDIATE / STAMPED_NAME
    release = output / f'{RELEASE}-{VERSION}.zip'
    checksum = output / (release.name + '.sha256')
    exe = output / EXE_NAME
    manifest = INTERMEDIATE / 'launcher-build-cache.json'
    command = compiler_command(output, archive)
    environment = dict(os.environ)
    temporary = DEVELOPMENT / 'temp'
    environment.update(TEMP=str(temporary), TMP=str(temporary))
    key, files = input_key(runtime_source, command, environment)
    outputs = (archive, stamped_host, release, checksum, exe)
    if not force and cache.cache_hit(manifest, key, outputs):
        return {'reused': True, 'exe': exe, 'release': release,
                'release_sha256': cache.file_sha256(release)}
    INTERMEDIATE.mkdir(parents=True, exist_ok=True)
    output.mkdir(parents=True, exist_ok=True)
    temporary.mkdir(parents=True, exist_ok=True)
    # The stamping helper validates the complete source and preserves code/data.
    stamp_icon(files['runtime/rekhcom64.exe'], stamped_host,
               ROOT / 'mod/action-edition/branding' / (BRAND + '.ico'))
    files['runtime/rekhcom64.exe'] = stamped_host
    cache.write_if_changed(archive, deterministic_archive(files))
    cache.copy_if_changed(archive, release)
    digest = cache.file_sha256(release)
    cache.write_if_changed(checksum, digest + '  ' + release.name + '\n', encoding='ascii')
    subprocess.run(command, check=True, cwd=ROOT, env=environment)
    after_key, _ = input_key(runtime_source, command, environment)
    if after_key != key:
        raise ValueError('Build inputs changed during compilation; outputs were not cached. Re-run the build.')
    cache.record_cache(manifest, key, outputs)
    return {'reused': False, 'exe': exe, 'release': release, 'release_sha256': digest}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime-source', type=Path, required=True,
                        help='Existing runtime folder containing host, guest and SDL3')
    parser.add_argument('--output', type=Path, default=DEFAULT_OUTPUT,
                        help='Output folder (default: the stable project Development launcher/out folder)')
    parser.add_argument('--force', action='store_true', help='Rebuild even when every cached artifact matches')
    args = parser.parse_args()
    result = build(args.runtime_source, args.output, args.force)
    print(('Reused verified launcher: ' if result['reused'] else 'Built launcher: ') + str(result['exe']))
    print('Release SHA256: ' + result['release_sha256'])


if __name__ == '__main__':
    main()
