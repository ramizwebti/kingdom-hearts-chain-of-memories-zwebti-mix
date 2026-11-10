#!/usr/bin/env python3
"""Historical Kingdom Hearts Chain of Memories: Zwebti Mix package builder.
Current single-file releases are built with launcher/build.py instead.
Never includes ROMs, saves, generated memory dumps or development toolchains.
"""
from pathlib import Path
import hashlib
import json
import shutil
import subprocess
from zipfile import ZIP_DEFLATED, ZipFile

ROOT = Path(__file__).resolve().parents[2]
BASE = '51edf7efa2c49c97a1c389ae2e4cd7e3b749448c'
OUT = ROOT / 'build/action-package'
FOLDER = OUT / 'Kingdom-Hearts-Chain-of-Memories-Zwebti-Mix'
ZIP = ROOT.parent / 'Kingdom-Hearts-Chain-of-Memories-Zwebti-Mix-Windows.zip'


def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT)


def main():
    # Keep the previous playable ZIP intact until the current image and source
    # have both passed review. The complete source archive below comes from HEAD.
    if git('status', '--porcelain', '--untracked-files=normal', '--', 'src', 'include', 'port', 'mod').strip():
        raise SystemExit('Commit the intended source and documentation before packaging.')
    guest_sha = hashlib.sha256((ROOT / 'build/windows64/out/guest-us.bin').read_bytes()).hexdigest()
    for rel in ('build/cohesion-final/report.json', 'build/cohesion-visuals/report.json'):
        report = json.loads((ROOT / rel).read_text())
        if not (report.get('passed') and report.get('guest_unchanged') and
                report.get('guest_sha256') == guest_sha):
            raise SystemExit('Current game image has not passed: ' + rel)
    if FOLDER.exists():
        shutil.rmtree(FOLDER)
    FOLDER.mkdir(parents=True)
    for name in ('PLAY.cmd', 'launch.ps1', 'START_HERE.html', 'kh_com.ini', 'README.md', 'RESEARCH.md', 'VALIDATION.md', 'CHANGELOG.md'):
        shutil.copy2(ROOT / 'mod/action-edition' / name, FOLDER / name)
    launcher_name = 'Kingdom Hearts Chain of Memories - Zwebti Mix.exe'
    shutil.copy2(ROOT / 'build/windows64/out' / launcher_name, FOLDER / launcher_name)
    shutil.copytree(ROOT / 'mod/action-edition/branding', FOLDER / 'branding')
    shutil.copytree(ROOT / 'mod/action-edition/development', FOLDER / 'development')
    previews = ROOT / 'build/action-previews'
    for name in ('zwebti-v05-combat.gif', 'zwebti-v05-menu.png', 'zwebti-v05-combat.png'):
        if (previews / name).exists():
            shutil.copy2(previews / name, FOLDER / 'development' / name)
    summary = ROOT / 'build/action-validation-summary.json'
    if summary.exists():
        shutil.copy2(summary, FOLDER / 'development/validation-summary.json')
    runtime = FOLDER / 'runtime'
    runtime.mkdir()
    for name in ('rekhcom64.exe', 'guest-us.bin', 'SDL3.dll'):
        shutil.copy2(ROOT / 'build/windows64/out' / name, runtime / name)
    shutil.copytree(ROOT / 'port/assets/fonts', runtime / 'assets/fonts')
    branding = ROOT / 'mod/action-edition/branding'
    shutil.copy2(branding / 'kingdom-hearts-chain-of-memories-zwebti-mix-icon.bmp', runtime / 'assets/icon.bmp')
    (runtime / 'assets/branding').mkdir(parents=True, exist_ok=True)
    shutil.copy2(branding / 'kingdom-hearts-chain-of-memories-zwebti-mix-logo.png',
                 runtime / 'assets/branding/kingdom-hearts-chain-of-memories-zwebti-mix-logo.png')
    licenses = FOLDER / 'licenses'
    licenses.mkdir()
    shutil.copy2(ROOT / 'LICENSE.md', licenses / 'ReKHCoM-CC0.md')
    shutil.copy2(ROOT / 'port/assets/fonts/NOTICE.md', licenses / 'Fonts-NOTICE.md')
    sdl = sorted((ROOT.parent / 'toolchain').glob('SDL3-*'))[-1]
    shutil.copy2(sdl / 'LICENSE.txt', licenses / 'SDL3-LICENSE.txt')
    shutil.copy2(ROOT / 'port/ps5/licenses/THIRD-PARTY.txt', licenses / 'Upstream-PS5-THIRD-PARTY.txt')
    shutil.copy2(ROOT / 'port/ps5/licenses/LICENSE-GPL-3.0.txt', licenses / 'GPL-3.0.txt')
    source = FOLDER / 'source'
    source.mkdir()
    (source / 'BASE_REVISION.txt').write_text(BASE + '\nhttps://github.com/MatyMeatBoy/ReKHCoM\n')
    (source / 'MOD_REVISION.txt').write_bytes(git('rev-parse', 'HEAD'))
    subprocess.check_call(['git', 'archive', '--format=tar.gz', '-o', str(source / 'ReKHCoM-source.tar.gz'), 'HEAD'], cwd=ROOT)
    # Files are included in full as well as in an ordinary patch so later
    # development can resume without depending on transient workspace state.
    paths = set(git('diff', '--name-only', BASE).decode().splitlines())
    paths.update(git('ls-files', '--others', '--exclude-standard').decode().splitlines())
    paths = sorted(p for p in paths if p.startswith(('src/', 'include/', 'port/', 'mod/'))
                   and (ROOT / p).is_file())
    for rel in paths:
        dst = source / 'files' / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT / rel, dst)
    # Tracked new files are part of this patch after the development commit.
    (source / 'changes.patch').write_bytes(git('diff', '--binary', BASE))
    (source / 'FILES.json').write_text(json.dumps(paths, indent=2) + '\n')
    tests = FOLDER / 'development/test-reports'
    tests.mkdir()
    for rel in ('build/action-native-v3/report.json', 'build/action-final-validation/report.json',
                'build/action-shipping-final/report.json',
                'build/action-shipping-validation/report.json',
                'build/action-field-native-v2/field-movement/report.json',
                'build/action-jafar-reaction/report.json',
                'build/action-polish-final/report.json',
                'build/action-polish-visuals/report.json',
                'build/action-polish-checks.json',
                'build/action-flow-final/report.json',
                'build/action-flow-visuals/report.json',
                'build/action-flow-checks.json',
                'build/zwebti-branding-validation.json',
                'build/zwebti-practice-unit.json',
                'build/zwebti-practice-final/report.json',
                'build/cohesion-final/report.json',
                'build/cohesion-visuals/report.json',
                'build/cohesion-checks.json',
                'build/cohesion-controller-checks.json',
                'build/cohesion-fx-checks.json',
                'build/cohesion-comparison.json',
                'build/cohesion-magic-impact.json',
                'build/cohesion-magic-source-audit.json',
                'build/cohesion-visuals/pixel-verification.json'):
        p = ROOT / rel
        if p.exists():
            name = rel.replace('build/', '').replace('/', '_')
            shutil.copy2(p, tests / name)
    checksums = []
    for p in sorted(FOLDER.rglob('*')):
        if not p.is_file():
            continue
        if p.suffix.lower() in ('.gba', '.sav', '.state', '.dmp'):
            raise SystemExit('Unwanted input or personal data in package: ' + str(p))
        checksums.append(hashlib.sha256(p.read_bytes()).hexdigest() + '  ' + p.relative_to(FOLDER).as_posix())
    (FOLDER / 'SHA256SUMS.txt').write_text('\n'.join(checksums) + '\n')
    with ZipFile(ZIP, 'w', ZIP_DEFLATED, compresslevel=9) as z:
        for p in sorted(FOLDER.rglob('*')):
            if p.is_file():
                z.write(p, p.relative_to(OUT))
    with ZipFile(ZIP) as z:
        assert z.testzip() is None
        assert not any(Path(n).suffix.lower() in ('.gba', '.sav', '.state', '.dmp') for n in z.namelist())
    print(json.dumps({'file': str(ZIP), 'bytes': ZIP.stat().st_size,
                      'sha256': hashlib.sha256(ZIP.read_bytes()).hexdigest(),
                      'source_files': len(paths)}, indent=2))


if __name__ == '__main__':
    main()
