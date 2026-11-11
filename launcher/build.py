"""Build a single Windows launcher from a known game runtime. No ROMs or saves."""
import argparse
import hashlib
from pathlib import Path
import shutil
import subprocess
import zipfile
from stamp_runtime_icon import stamp_icon

ROOT = Path(__file__).resolve().parents[1]
VERSION = '0.10.1'
BRAND = 'kingdom-hearts-chain-of-memories-zwebti-mix'
RELEASE = 'Kingdom-Hearts-Chain-of-Memories-Zwebti-Mix'


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--runtime-source', type=Path, required=True,
                        help='Existing runtime folder containing host, guest and SDL3')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    intermediate = ROOT / 'build' / 'launcher'
    intermediate.mkdir(parents=True, exist_ok=True)
    archive = intermediate / 'release.zip'
    mod = ROOT / 'mod' / 'action-edition'
    files = {}
    for name in ('launch.ps1', 'PLAY.cmd', 'kh_com.ini', 'START_HERE.html', 'README.md'):
        files[name] = mod / name
    for name in (BRAND + '-logo.png', BRAND + '-icon.png', BRAND + '-icon.bmp', BRAND + '.ico'):
        files['branding/' + name] = mod / 'branding' / name
    for name in ('rekhcom64.exe', 'guest-us.bin', 'SDL3.dll'):
        files['runtime/' + name] = args.runtime_source / name
    # Retain every code/data section while replacing the Windows taskbar icon.
    stamped_host = intermediate / 'stamped-rekhcom64.exe'
    stamp_icon(args.runtime_source / 'rekhcom64.exe', stamped_host,
               mod / 'branding' / (BRAND + '.ico'))
    files['runtime/rekhcom64.exe'] = stamped_host
    for path in (args.runtime_source / 'assets').rglob('*'):
        if path.is_file():
            files['runtime/assets/' + path.relative_to(args.runtime_source / 'assets').as_posix()] = path
    files['runtime/assets/icon.bmp'] = mod / 'branding' / (BRAND + '-icon.bmp')
    files['runtime/assets/branding/' + BRAND + '-logo.png'] = mod / 'branding' / (BRAND + '-logo.png')
    licenses = ROOT / 'build' / 'launcher' / 'licenses'
    licenses.mkdir(exist_ok=True)
    shutil.copy2(ROOT / 'LICENSE.md', licenses / 'ReKHCoM-CC0.md')
    # Retain the original binary distribution notices alongside that runtime.
    distribution_licenses = args.runtime_source.parent / 'licenses'
    for path in distribution_licenses.glob('*'):
        if path.is_file():
            files['licenses/' + path.name] = path
    files['licenses/ReKHCoM-CC0.md'] = licenses / 'ReKHCoM-CC0.md'
    for name, path in files.items():
        if not path.is_file():
            raise SystemExit('Missing runtime input: ' + str(path))
        if path.suffix.lower() in ('.gba', '.sav', '.state', '.kst', '.khst', '.dmp'):
            raise SystemExit('Personal game data cannot be packaged: ' + name)
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as package:
        for name, path in sorted(files.items()):
            package.write(path, name)
    release = args.output / f'{RELEASE}-{VERSION}.zip'
    shutil.copy2(archive, release)
    digest = hashlib.sha256(release.read_bytes()).hexdigest()
    (args.output / (release.name + '.sha256')).write_text(digest + '  ' + release.name + '\n', encoding='ascii')
    compiler = Path('C:/Windows/Microsoft.NET/Framework64/v4.0.30319/csc.exe')
    exe = args.output / 'Kingdom Hearts Chain of Memories - Zwebti Mix.exe'
    command = [str(compiler), '/nologo', '/target:winexe', '/platform:x64', '/optimize+',
               '/out:' + str(exe), '/win32icon:' + str(mod / 'branding' / (BRAND + '.ico')),
               '/r:System.Windows.Forms.dll', '/r:System.Drawing.dll',
               '/r:System.IO.Compression.dll', '/r:System.IO.Compression.FileSystem.dll',
               '/r:System.Web.Extensions.dll',
               '/resource:' + str(archive) + ',release.zip',
               '/resource:' + str(mod / 'branding' / (BRAND + '-logo.png')) + ',logo.png',
               '/resource:' + str(mod / 'branding' / (BRAND + '.ico')) + ',heart.ico',
               str(ROOT / 'launcher/Launcher.cs'), str(ROOT / 'launcher/ReleaseStore.cs')]
    subprocess.run(command, check=True)
    print(f'{exe}: {exe.stat().st_size:,} bytes')
    print(f'Release SHA256: {digest}')


if __name__ == '__main__':
    main()
