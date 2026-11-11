"""Package this project's verified runtime on GitHub-hosted Windows only.

The sole input is the independently checked SHA256 of the fixed runtime asset
on this version's draft release. No ROM, player profile or local toolchain is
needed. This script never publishes a release.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import stat
import subprocess
import sys
import zipfile

import build as launcher_build
from stamp_runtime_icon import pe_sections, resources

ROOT = Path(__file__).resolve().parents[1]
REPOSITORY = 'ramizwebti/kingdom-hearts-chain-of-memories-zwebti-mix'
PROJECT = 'Kingdom Hearts Chain of Memories: Zwebti Mix'
DEVELOPMENT = launcher_build.DEVELOPMENT
MAX_ARCHIVE = 256 * 1024 * 1024
MAX_EXPANDED = 512 * 1024 * 1024
MAX_FILE = 128 * 1024 * 1024
MAX_ENTRIES = 20000
REQUIRED_RUNTIME = {'runtime/guest-us.bin', 'runtime/rekhcom64.exe', 'runtime/SDL3.dll'}


def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def checked_sha256(value):
    if not re.fullmatch(r'[0-9a-fA-F]{64}', value or ''):
        raise ValueError('Expected runtime SHA256 must contain exactly 64 hexadecimal characters')
    return value.lower()


def safe_name(info):
    name = info.filename
    if not name or name.startswith('/') or '\\' in name or ':' in name:
        raise ValueError('Unsafe archive path: ' + repr(name))
    pieces = name.removesuffix('/').split('/')
    for part in pieces:
        if (not part or part in ('.', '..') or part.endswith((' ', '.')) or
                re.search(r'[\x00-\x1f<>"|?*]', part) or
                re.match(r'(?i)^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\.|$)', part)):
            raise ValueError('Unsafe Windows archive path: ' + repr(name))
        if part.casefold() in ('data', 'practice-data', 'profile', 'saves', '.git'):
            raise ValueError('Player/source data is forbidden in an archive: ' + name)
    suffix = Path(pieces[-1]).suffix.lower()
    if (suffix in ('.gba', '.gb', '.gbc', '.nds', '.3ds', '.iso', '.rom', '.sav',
                   '.srm', '.dsv', '.dmp', '.kst', '.khst', '.log') or
            re.fullmatch(r'\.(sgm|ss|state)[0-9]*', suffix)):
        raise ValueError('Game dumps, saves and diagnostics are forbidden: ' + name)
    mode = (info.external_attr >> 16) & 0xf000
    if mode not in (0, stat.S_IFREG, stat.S_IFDIR) or info.external_attr & 0x400:
        raise ValueError('Archive links and special files are forbidden: ' + name)
    if info.flag_bits & 1 or info.compress_type not in (zipfile.ZIP_STORED, zipfile.ZIP_DEFLATED):
        raise ValueError('Unsupported archive encoding: ' + name)
    return name


def inspect_archive(path, runtime_only=False):
    """Validate bounds, Windows paths, case aliases and CRC before extraction."""
    if not 0 < path.stat().st_size <= MAX_ARCHIVE:
        raise ValueError('Archive size is outside the release-store limits')
    files, seen, expanded = {}, set(), 0
    with zipfile.ZipFile(path) as archive:
        if len(archive.infolist()) > MAX_ENTRIES:
            raise ValueError('Too many archive members')
        for info in archive.infolist():
            name = safe_name(info)
            key = name.rstrip('/').casefold()
            if key in seen:
                raise ValueError('Case-insensitive duplicate archive path: ' + name)
            seen.add(key)
            if not 0 <= info.file_size <= MAX_FILE:
                raise ValueError('Archive member exceeds the per-file limit')
            expanded += info.file_size
            if expanded > MAX_EXPANDED:
                raise ValueError('Archive exceeds the expanded size limit')
            if runtime_only:
                allowed = (name in REQUIRED_RUNTIME or
                           name.startswith('runtime/assets/') or
                           (name.startswith('licenses/') and name.rstrip('/').count('/') == 1) or
                           name in ('runtime/', 'runtime/assets/', 'licenses/'))
                if not allowed:
                    raise ValueError('Unexpected runtime input member: ' + name)
                if name.startswith('runtime/assets/') and Path(name).suffix.lower() in ('.exe', '.dll'):
                    raise ValueError('Executable assets are not supported: ' + name)
            if not info.is_dir():
                if not info.file_size:
                    raise ValueError('Empty archive member: ' + name)
                files[name] = info
        if runtime_only and (not REQUIRED_RUNTIME.issubset(files) or
                not any(n.startswith('runtime/assets/') for n in files) or
                not any(n.startswith('licenses/') for n in files)):
            raise ValueError('Runtime input requires host, guest, SDL, assets and notices')
        if archive.testzip() is not None:
            raise ValueError('Archive CRC verification failed')
    return files


def extract_verified(path, destination, members):
    destination.mkdir(parents=True, exist_ok=False)
    with zipfile.ZipFile(path) as archive:
        for name in members:
            target = destination.joinpath(*name.split('/'))
            if not target.resolve().is_relative_to(destination.resolve()):
                raise ValueError('Extraction escaped the staging folder')
            target.parent.mkdir(parents=True, exist_ok=True)
            with archive.open(name) as source, target.open('xb') as output:
                shutil.copyfileobj(source, output)


def file_map(folder):
    result = {}
    for path in sorted(folder.rglob('*')):
        if path.is_symlink():
            raise ValueError('A staged file became a link')
        if path.is_file():
            result[path.relative_to(folder).as_posix()] = dict(size=path.stat().st_size, sha256=digest(path))
    return result


def hosted_environment():
    if (os.name != 'nt' or os.environ.get('GITHUB_ACTIONS') != 'true' or
            os.environ.get('RUNNER_ENVIRONMENT') != 'github-hosted' or
            os.environ.get('RUNNER_OS') != 'Windows'):
        raise RuntimeError('Packaging is restricted to GitHub-hosted Windows runners')
    if os.environ.get('GITHUB_REPOSITORY') != REPOSITORY:
        raise RuntimeError('This packager only accepts ' + REPOSITORY)
    if (os.environ.get('GITHUB_EVENT_NAME') != 'workflow_dispatch' or
            os.environ.get('GITHUB_REF') != 'refs/heads/main' or
            os.environ.get('GITHUB_SERVER_URL') != 'https://github.com'):
        raise RuntimeError('Dispatch this packaging workflow from this repository main branch')
    if not re.fullmatch(r'[0-9a-f]{40}', os.environ.get('GITHUB_SHA', '')):
        raise RuntimeError('Missing GitHub source commit identity')
    if not os.environ.get('GH_TOKEN'):
        raise RuntimeError('The runner default GitHub token is required')


def download_runtime(folder):
    version = launcher_build.VERSION
    tag = 'v' + version
    name = f'{launcher_build.RELEASE}-{version}-runtime.zip'
    environment = dict(os.environ, GH_HOST='github.com', GH_PROMPT_DISABLED='1')
    environment.pop('GH_DEBUG', None)
    metadata = subprocess.run(['gh', 'release', 'view', tag, '--repo', REPOSITORY,
        '--json', 'tagName,isDraft,assets'], check=True, capture_output=True,
        text=True, env=environment, timeout=60)
    release = json.loads(metadata.stdout)
    if release.get('tagName') != tag or release.get('isDraft') is not True:
        raise ValueError('Runtime input must come from the exact current-version draft release')
    assets = [asset for asset in release.get('assets', []) if asset.get('name') == name]
    if len(assets) != 1 or not 0 < assets[0].get('size', 0) <= MAX_ARCHIVE:
        raise ValueError('Missing, duplicate or oversized fixed runtime release asset')
    folder.mkdir(parents=True, exist_ok=False)
    subprocess.run(['gh', 'release', 'download', tag, '--repo', REPOSITORY,
        '--pattern', name, '--dir', str(folder)], check=True, env=environment, timeout=180)
    if {p.name for p in folder.iterdir()} != {name}:
        raise ValueError('Download produced unexpected files')
    return folder / name, dict(tag=tag, asset=name, size=assets[0]['size'])


def verify_install(executable, install, release_files, archive_sha256, environment):
    profile = install / 'profile'
    previous = install / 'versions/0.10.1'
    sentinels = {
        profile / 'data/story.sav': b'SYNTHETIC STORY SAVE\x00\xff\r\n',
        profile / 'practice-data/practice.sav': b'SYNTHETIC PRACTICE SAVE\x00\x01',
        profile / 'controller-settings.txt': b'SYNTHETIC SETTINGS = preserve bytes\r\n',
        previous / 'previous-version-sentinel.txt': b'SYNTHETIC RETAINED 0.10.1\r\n',
    }
    for path, contents in sentinels.items():
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(contents)
    (install / 'active-version.txt').write_text('0.10.1\n', encoding='ascii')
    before_profile, before_previous = file_map(profile), file_map(previous)
    installed_before = None
    checks = []
    for attempt in (1, 2):
        marker = install.parent / f'verify-install-{attempt}.txt'
        subprocess.run([str(executable), '--verify-install', str(install), str(marker)],
                       check=True, env=environment, timeout=120)
        if marker.read_text().strip() != launcher_build.VERSION or (
                install / 'active-version.txt').read_text().strip() != launcher_build.VERSION:
            raise ValueError('Launcher did not activate the expected embedded version')
        installed = file_map(install / 'versions' / launcher_build.VERSION)
        stamp = installed.pop('.archive-sha256', None)
        stamp_path = install / 'versions' / launcher_build.VERSION / '.archive-sha256'
        if not stamp or stamp_path.read_text().strip() != archive_sha256 or installed != release_files:
            raise ValueError('Installed release differs from the exact checked package')
        if file_map(profile) != before_profile or file_map(previous) != before_previous:
            raise ValueError('Upgrade changed the synthetic profile or prior immutable version')
        if installed_before is not None and installed != installed_before:
            raise ValueError('Second verify-install changed the installed version')
        installed_before = installed
        checks.append(dict(attempt=attempt,
                           operation='upgrade from synthetic 0.10.1' if attempt == 1 else 'idempotent launcher relaunch',
                           activated=launcher_build.VERSION,
                           profile_byte_identical=True, previous_version_retained=True,
                           installed_payload_exact=True))
    return dict(synthetic_only=True, profile_files=before_profile, previous_files=before_previous, attempts=checks)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime-sha256', required=True)
    args = parser.parse_args()
    expected = checked_sha256(args.runtime_sha256)
    hosted_environment()
    package_root = DEVELOPMENT / 'packaging'
    package_root.mkdir(parents=True, exist_ok=False)
    environment = {k: v for k, v in os.environ.items() if k not in ('GH_TOKEN', 'GITHUB_TOKEN')}
    temporary = DEVELOPMENT / 'temp/packaging'
    temporary.mkdir(parents=True, exist_ok=True)
    environment.update(TEMP=str(temporary), TMP=str(temporary))
    archive, source_release = download_runtime(package_root / 'download')
    if digest(archive) != expected:
        raise ValueError('Runtime input SHA256 does not match the approved value')
    members = inspect_archive(archive, runtime_only=True)
    stage = DEVELOPMENT / ('staging/cloud-runtime-' + launcher_build.VERSION)
    extract_verified(archive, stage, members)
    input_files = file_map(stage)
    output = package_root / 'artifacts'
    subprocess.run([sys.executable, str(ROOT / 'launcher/build.py'), '--runtime-source',
                    str(stage / 'runtime'), '--output', str(output)],
                   check=True, env=environment, cwd=ROOT, timeout=600)
    if file_map(stage) != input_files or digest(archive) != expected:
        raise ValueError('Runtime source changed during packaging')
    release = output / f'{launcher_build.RELEASE}-{launcher_build.VERSION}.zip'
    release_members = inspect_archive(release)
    required = launcher_build.archive_files(stage / 'runtime')
    if set(release_members) != set(required):
        raise ValueError('Release file map does not match the effective builder inputs')
    release_files = {}
    with zipfile.ZipFile(release) as package:
        for name in release_members:
            data = package.read(name)
            release_files[name] = dict(size=len(data), sha256=hashlib.sha256(data).hexdigest())
            if release_files[name]['sha256'] != digest(required[name]):
                raise ValueError('Packaged member differs from its exact input: ' + name)
    # All source runtime bytes, including already canonical branding, must stay
    # identical. The builder's icon helper is independently checked as well.
    for name in input_files:
        if name.startswith('runtime/') and release_files.get(name) != input_files[name]:
            raise ValueError('Packaging changed a verified runtime input: ' + name)
    source_host = stage / 'runtime/rekhcom64.exe'
    stamped_host = launcher_build.INTERMEDIATE / launcher_build.STAMPED_NAME
    if pe_sections(source_host) != pe_sections(stamped_host):
        raise ValueError('Host code/data sections changed during icon staging')
    source_resources, stamped_resources = resources(source_host), resources(stamped_host)
    if {k: v for k, v in source_resources.items() if k[0] not in (3, 14)} != {
            k: v for k, v in stamped_resources.items() if k[0] not in (3, 14)}:
        raise ValueError('Host non-icon resources changed during staging')
    checksum = output / (release.name + '.sha256')
    release_sha = digest(release)
    if checksum.read_text(encoding='ascii') != release_sha + '  ' + release.name + '\n':
        raise ValueError('Release checksum sidecar is inconsistent')
    executable = output / f'Kingdom Hearts Chain of Memories - Zwebti Mix {launcher_build.VERSION}.exe'
    (output / launcher_build.EXE_NAME).rename(executable)
    installation = verify_install(executable, package_root / 'isolated-install', release_files,
                                  release_sha, environment)
    expected_outputs = {executable.name, release.name, checksum.name}
    if {p.name for p in output.iterdir()} != expected_outputs:
        raise ValueError('Unexpected packaging output')
    manifest = dict(project=PROJECT, repository=REPOSITORY, version=launcher_build.VERSION,
        source_commit=os.environ['GITHUB_SHA'], workflow_run=os.environ.get('GITHUB_RUN_ID'),
        runtime_source=dict(**source_release, sha256=expected, files=input_files),
        outputs=file_map(output), packaged_payload=release_files,
        host=dict(source_sha256=digest(source_host), packaged_sha256=digest(stamped_host),
                  exact_input_bytes=True, code_data_sections_preserved=True,
                  non_icon_resources_preserved=True),
        validation=dict(zip_crc=True, safe_paths=True, game_dumps_and_saves_excluded=True,
                        exact_runtime_hashes=True, installation=installation),
        scope='Hosted packaging and isolated synthetic installation; native combat was validated separately')
    (output / 'packaging-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    print('Packaged and verified ' + PROJECT + ' ' + launcher_build.VERSION)
    print('Runtime SHA256: ' + expected)
    print('Release SHA256: ' + release_sha)


if __name__ == '__main__':
    main()
