"""ROM-free validation fixtures for the hosted-only packaging input boundary."""
import importlib.util
from pathlib import Path
import stat
import sys
import tempfile
import unittest
import zipfile

LAUNCHER = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(LAUNCHER))
spec = importlib.util.spec_from_file_location('package_windows_ci', LAUNCHER / 'package_windows_ci.py')
package = importlib.util.module_from_spec(spec)
spec.loader.exec_module(package)


class RuntimeArchiveTests(unittest.TestCase):
    def setUp(self):
        temporary = package.DEVELOPMENT / 'temp/package-validation'
        temporary.mkdir(parents=True, exist_ok=True)
        self.directory = tempfile.TemporaryDirectory(dir=temporary)
        self.root = Path(self.directory.name)

    def tearDown(self):
        self.directory.cleanup()

    def archive(self, extras=(), omit=()):
        path = self.root / 'runtime.zip'
        names = sorted(package.REQUIRED_RUNTIME | {'runtime/assets/icon.bmp', 'licenses/NOTICE.txt'})
        with zipfile.ZipFile(path, 'w', compression=zipfile.ZIP_STORED) as output:
            for name in names:
                if name not in omit:
                    output.writestr(name, ('SYNTHETIC ' + name).encode())
            for name in extras:
                output.writestr(name, b'SYNTHETIC EXTRA')
        return path

    def test_exact_hash_syntax(self):
        self.assertEqual(package.checked_sha256('A' * 64), 'a' * 64)
        for invalid in ('', 'a' * 63, 'a' * 65, 'g' * 64, ' a' * 32, 'a' * 64 + '\n'):
            with self.subTest(invalid=invalid), self.assertRaises(ValueError):
                package.checked_sha256(invalid)

    def test_valid_runtime_extracts_exact_bytes(self):
        archive = self.archive()
        members = package.inspect_archive(archive, runtime_only=True)
        destination = self.root / 'staging'
        package.extract_verified(archive, destination, members)
        self.assertEqual(set(package.file_map(destination)), set(members))
        with zipfile.ZipFile(archive) as source:
            for name in members:
                self.assertEqual((destination / name).read_bytes(), source.read(name))

    def test_missing_required_runtime_or_notices(self):
        for name in sorted(package.REQUIRED_RUNTIME | {'runtime/assets/icon.bmp', 'licenses/NOTICE.txt'}):
            with self.subTest(omitted=name), self.assertRaises(ValueError):
                package.inspect_archive(self.archive(omit=(name,)), runtime_only=True)

    def test_unsafe_private_and_unexpected_paths(self):
        names = ('../escape.txt', '/absolute.txt', 'C:/absolute.txt', 'runtime\\escape.txt',
                 'runtime/assets/../escape.txt', 'runtime/assets/CON.txt', 'runtime/assets/tail.',
                 'runtime/assets/tail ', 'runtime/assets/a:stream', 'runtime/assets//gap.txt',
                 'runtime/profile/settings.txt', 'runtime/assets/game.gba', 'runtime/assets/game.nds',
                 'runtime/assets/player.sav', 'runtime/assets/slot.state12', 'runtime/assets/debug.log',
                 'runtime/assets/helper.exe', 'runtime/assets/other.dll', 'unexpected.txt',
                 'licenses/nested/NOTICE.txt', 'runtime/assets/icon.bmp')
        for name in names:
            with self.subTest(name=name), self.assertRaises(ValueError):
                package.inspect_archive(self.archive(extras=(name,)), runtime_only=True)

    def test_windows_case_alias_is_rejected(self):
        with self.assertRaises(ValueError):
            package.inspect_archive(self.archive(extras=('runtime/assets/ICON.BMP',)), runtime_only=True)

    def test_special_and_encrypted_members_are_rejected(self):
        for kind in (stat.S_IFLNK, stat.S_IFIFO):
            info = zipfile.ZipInfo('runtime/assets/link')
            info.external_attr = kind << 16
            with self.subTest(kind=kind), self.assertRaises(ValueError):
                package.safe_name(info)
        info = zipfile.ZipInfo('runtime/assets/reparse')
        info.external_attr = 0x400
        with self.assertRaises(ValueError):
            package.safe_name(info)
        info = zipfile.ZipInfo('runtime/assets/encrypted')
        info.flag_bits = 1
        with self.assertRaises(ValueError):
            package.safe_name(info)

    def test_corrupt_member_fails_crc_before_extraction(self):
        archive = self.archive()
        data = archive.read_bytes()
        archive.write_bytes(data.replace(b'SYNTHETIC runtime/guest-us.bin', b'CORRUPTED runtime/guest-us.bin', 1))
        with self.assertRaises((ValueError, zipfile.BadZipFile)):
            package.inspect_archive(archive, runtime_only=True)


if __name__ == '__main__':
    unittest.main()
