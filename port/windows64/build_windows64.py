#!/usr/bin/env python3
"""Build the native Windows x64 SDL host and x32 game images (32-bit pointers).
Use --host-only to reuse previously built guest images without compiling the game.
"""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
OUT = ROOT / 'build/windows64/out'

def main():
    global OUT
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--regions', nargs='+', choices=['eu', 'us', 'jp'], default=['eu', 'us', 'jp'])
    ap.add_argument('--jobs', type=int, default=1)
    ap.add_argument('--host-only', action='store_true')
    ap.add_argument('--output', type=Path, default=OUT,
                    help='Isolated output directory inside this checkout build directory')
    ap.add_argument('--sdl-runtime', type=Path,
                    help='Reuse an existing SDL3.dll while compiling against the local SDK')
    ap.add_argument('--icon', type=Path, help='Heart-only ICO to embed in native executable resources')
    ap.add_argument('--test', action='store_true', help='run the ABI and low mapping regression test')
    args = ap.parse_args()
    if args.jobs < 1: ap.error('--jobs must be positive')
    OUT=args.output.resolve()
    if not OUT.is_relative_to((ROOT/'build').resolve()):
        ap.error('--output must stay inside this checkout build directory')
    if args.sdl_runtime and not args.sdl_runtime.is_file(): ap.error('missing --sdl-runtime')
    toolchains=sorted(path for path in (ROOT.parent/'toolchain').glob('llvm-mingw-*') if path.is_dir())
    sdks=sorted(path for path in (ROOT.parent/'toolchain').glob('SDL3-*') if path.is_dir())
    if not toolchains or not sdks: ap.error('missing extracted local LLVM-MinGW or SDL3 SDK')
    tc=toolchains[-1]/'bin'
    sdl=sdks[-1]/'x86_64-w64-mingw32'
    # llvm-mingw provides native Linux and Windows tool distributions. The
    # guest is freestanding ELF x32; the host always targets Windows.
    def tool(name):
        return str(tc / (name + '.exe' if (tc / (name + '.exe')).is_file() else name))
    compiler = tool('x86_64-w64-mingw32-clang')
    OUT.mkdir(parents=True, exist_ok=True)
    from prepare_movie_core import prepare
    prepare(ROOT, OUT.parent)
    env = dict(os.environ, CLANG=tool('clang'), LLD=tool('ld.lld'),
               OBJCOPY=tool('llvm-objcopy'), REKHCOM_GUEST_DUALSENSE='0', REKHCOM_GUEST_LINUX64='1')
    if not args.host_only:
        for region in args.regions:
            subprocess.check_call([sys.executable, str(ROOT / 'port/ps5/build_ps5.py'), 'guest',
                                   '--region', region, '--jobs', str(args.jobs)], cwd=ROOT, env=env)
            shutil.copyfile(ROOT / f'port/ps5/build/guest-{region}/guest.bin', OUT / f'guest-{region}.bin')
    flags = [compiler, '--target=x86_64-w64-mingw32', '-O2',
             f'-I{ROOT / "port/ps5/shared"}', f'-I{ROOT / "port/ps5/third_party/stb"}',
             f'-I{sdl / "include"}', f'-I{OUT.parent}', f'-L{sdl / "lib"}',
             '-lmfplat', '-lmfreadwrite', '-lmfuuid', '-lole32', '-lpropsys', '-lgdi32', '-lws2_32']
    branding = ROOT / 'mod/action-edition/branding'
    named_icon=branding/'kingdom-hearts-chain-of-memories-zwebti-mix.ico'
    icon=(args.icon or named_icon).resolve()
    if not icon.is_file():
        raise SystemExit('Missing Kingdom Hearts Chain of Memories: Zwebti Mix icon: ' + str(icon))
    if any(character in icon.as_posix() for character in ('"','\r','\n')):
        ap.error('unsupported icon path')
    resource_source=(HERE/'zwebti_mix.rc').read_text()
    anchor='#define ZWEBTI_ICON_FILE "kingdom-hearts-chain-of-memories-zwebti-mix.ico"'
    if resource_source.count(anchor)!=1: raise SystemExit('native icon resource source contract changed')
    resource_script=OUT.parent/'kingdom-hearts-chain-of-memories-zwebti-mix.rc'
    resource_script.write_text(resource_source.replace(anchor,'#define ZWEBTI_ICON_FILE "'+icon.as_posix()+'"'))
    resource_objects = []
    for role in ('game', 'launcher'):
        resource = OUT / f'zwebti-mix-{role}-res.o'
        command = [tool('x86_64-w64-mingw32-windres'), '-I', str(branding),
                   '-i', str(resource_script), '-o', str(resource), '-O', 'coff']
        if role == 'launcher':
            command += ['-D', 'ZWEBTI_LAUNCHER=1']
        subprocess.check_call(command, cwd=ROOT)
        resource_objects.append(resource)
    subprocess.check_call(flags + ['-municode', '-mwindows', str(HERE / 'windows64_host.c'), str(resource_objects[0]),
                                   '-lSDL3', '-lm', '-o', str(OUT / 'rekhcom64.exe')])
    subprocess.check_call([compiler, '--target=x86_64-w64-mingw32', '-O2', '-municode', '-mwindows',
                           str(HERE / 'launcher.c'), str(resource_objects[1]), '-luser32',
                           '-o', str(OUT / 'Kingdom Hearts Chain of Memories - Zwebti Mix.exe')])
    shutil.copyfile(args.sdl_runtime or sdl/'bin/SDL3.dll', OUT/'SDL3.dll')
    if args.test:
        subprocess.check_call(flags + [str(HERE / 'test_movie.c'), '-lSDL3', '-lm', '-o', str(OUT / 'test_movie.exe')])
        test = OUT / 'test_windows64.exe'
        subprocess.check_call(flags + [str(HERE / 'test_windows64.c'), str(HERE / 'test_preserved.S'), '-lSDL3', '-lm', '-o', str(test)])
        subprocess.check_call([str(test)], cwd=OUT)
        subprocess.check_call([sys.executable, str(HERE / "test_boot_status.py"), str(OUT / "rekhcom64.exe")])
        bios = OUT / 'test_guest_biosfault.exe'  # the game's NULL accesses read the BIOS stand-in (#564)
        subprocess.check_call([compiler, '--target=x86_64-w64-mingw32', '-O2', f'-I{ROOT}',
                               str(ROOT / 'port/tools/test_guest_biosfault.c'), '-o', str(bios)])
        subprocess.check_call([str(bios)], cwd=OUT)
        if subprocess.run([str(bios), '--host-null'], cwd=OUT, capture_output=True).returncode != 3:
            raise SystemExit('a host NULL read outside the guest text must still crash')
    print(OUT)
if __name__ == '__main__': main()
