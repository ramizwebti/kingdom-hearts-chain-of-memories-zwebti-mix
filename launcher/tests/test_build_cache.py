"""Launcher cache orchestration with .bin stand-ins; never run a build tool."""
from contextlib import ExitStack
import importlib.util
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
import zipfile


LAUNCHER = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(LAUNCHER))
SPEC = importlib.util.spec_from_file_location('launcher_builder', LAUNCHER / 'build.py')
builder = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(builder)


class LauncherBuildCacheTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix='launcher-cache-unit-')
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.development = self.root / 'Development'
        self.intermediate = self.development / 'launcher'
        self.output = self.intermediate / 'out'
        self.runtime = self.root / 'runtime-fixtures'
        self.runtime.mkdir()
        (self.root / 'launcher').mkdir()
        self.csharp = self.root / 'launcher/Launcher.cs'
        self.csharp.write_text('launcher source A', encoding='utf-8')
        (self.root / 'launcher/ReleaseStore.cs').write_text('release store source', encoding='utf-8')
        self.compiler = self.root / 'compiler.bin'
        self.compiler.write_bytes(b'compiler fixture')
        self.inputs = {}
        for archive_name, name, content in (
                ('runtime/rekhcom64.exe', 'host.bin', b'host A'),
                ('runtime/guest-us.bin', 'guest.bin', b'guest A'),
                ('runtime/SDL3.dll', 'sdl.bin', b'sdl A'),
                ('branding/master-logo.png', 'logo.bin', b'logo A'),
                ('README.md', 'README.md', b'help A')):
            path = self.runtime / name
            path.write_bytes(content)
            self.inputs[archive_name] = path
        self.stamps = 0
        self.compiles = 0
        self.child_environments = []
        self.on_compile = None
        self.stack = ExitStack()
        self.addCleanup(self.stack.close)
        for name, value in (
                ('ROOT', self.root), ('DEVELOPMENT', self.development),
                ('INTERMEDIATE', self.intermediate), ('COMPILER', self.compiler),
                ('EXE_NAME', 'launcher.bin'), ('STAMPED_NAME', 'stamped-host.bin')):
            self.stack.enter_context(patch.object(builder, name, value))
        self.stack.enter_context(patch.object(builder, 'archive_files',
                                             side_effect=lambda source: dict(self.inputs)))
        self.stack.enter_context(patch.object(builder, 'compiler_inputs',
                                             return_value=([self.compiler], [('compiler fixture', [])])))
        self.stack.enter_context(patch.object(builder, 'stamp_icon', side_effect=self.fake_stamp))
        self.stack.enter_context(patch.object(builder.subprocess, 'run', side_effect=self.fake_compiler))

    def tearDown(self):
        self.assertEqual(list(self.root.rglob('*.exe')), [], 'Tests must never write an EXE')

    def fake_stamp(self, source, destination, icon):
        self.assertEqual(Path(source).suffix, '.bin')
        self.assertEqual(Path(destination).suffix, '.bin')
        self.stamps += 1
        builder.cache.copy_if_changed(source, destination)

    def fake_compiler(self, command, *, check, cwd, env):
        self.assertTrue(check)
        self.assertEqual(cwd, self.root)
        output = Path(next(value[5:] for value in command if value.startswith('/out:')))
        self.assertEqual(output.suffix, '.bin')
        self.compiles += 1
        self.child_environments.append(dict(env))
        data = (builder.cache.file_sha256(self.intermediate / 'release.zip') +
                builder.cache.file_sha256(self.csharp)).encode('ascii')
        output.write_bytes(data)
        if self.on_compile:
            self.on_compile()

    def build(self, **kwargs):
        return builder.build(self.runtime, self.output, **kwargs)

    def test_exact_repeat_skips_stamp_compile_and_all_output_writes(self):
        original_environment = {name: os.environ.get(name) for name in ('TEMP', 'TMP')}
        self.assertFalse(self.build()['reused'])
        outputs = list(self.intermediate.rglob('*'))
        before = {path: path.stat().st_mtime_ns for path in outputs if path.is_file()}
        # Source mtimes do not invalidate content-identical inputs.
        os.utime(self.inputs['README.md'], ns=(1_500_000_000_000_000_000,) * 2)
        self.assertTrue(self.build()['reused'])
        self.assertEqual((self.stamps, self.compiles), (1, 1))
        self.assertEqual({path: path.stat().st_mtime_ns for path in before}, before)
        child = self.child_environments[0]
        self.assertEqual(child['TEMP'], str(self.development / 'temp'))
        self.assertEqual(child['TMP'], str(self.development / 'temp'))
        self.assertEqual({name: os.environ.get(name) for name in ('TEMP', 'TMP')}, original_environment)

    def test_same_size_same_mtime_input_change_invalidates_cache(self):
        first = self.build()
        source = self.inputs['README.md']
        before = source.stat()
        source.write_bytes(b'help B')
        os.utime(source, ns=(before.st_atime_ns, before.st_mtime_ns))
        second = self.build()
        self.assertFalse(second['reused'])
        self.assertNotEqual(first['release_sha256'], second['release_sha256'])
        self.assertEqual((self.stamps, self.compiles), (2, 2))

    def test_corrupt_output_is_not_accepted_by_size_or_mtime(self):
        self.build()
        output = self.output / 'launcher.bin'
        expected = output.read_bytes()
        before = output.stat()
        output.write_bytes(b'x' * len(expected))
        os.utime(output, ns=(before.st_atime_ns, before.st_mtime_ns))
        self.assertFalse(self.build()['reused'])
        self.assertEqual(output.read_bytes(), expected)
        self.assertEqual((self.stamps, self.compiles), (2, 2))

    def test_archive_path_rename_invalidates_even_when_source_files_are_identical(self):
        first = self.build()
        self.inputs['RENAMED-README.md'] = self.inputs.pop('README.md')
        second = self.build()
        self.assertFalse(second['reused'])
        self.assertNotEqual(first['release_sha256'], second['release_sha256'])
        self.assertEqual((self.stamps, self.compiles), (2, 2))

    def test_changed_compiler_bytes_invalidate_cache(self):
        self.build()
        before = self.compiler.stat()
        self.compiler.write_bytes(b'compiler changed')
        os.utime(self.compiler, ns=(before.st_atime_ns, before.st_mtime_ns))
        self.assertFalse(self.build()['reused'])
        self.assertEqual((self.stamps, self.compiles), (2, 2))

    def test_force_bypasses_verified_cache(self):
        self.build()
        self.assertFalse(self.build(force=True)['reused'])
        self.assertEqual((self.stamps, self.compiles), (2, 2))

    def test_changed_input_during_compilation_is_never_cached(self):
        self.on_compile = lambda: self.csharp.write_text('launcher source B', encoding='utf-8')
        with self.assertRaisesRegex(ValueError, 'inputs changed'):
            self.build()
        self.assertFalse((self.intermediate / 'launcher-build-cache.json').exists())

    def test_added_archive_member_during_compilation_is_never_cached(self):
        def add_member():
            path = self.runtime / 'extra.bin'
            path.write_bytes(b'added dependency')
            self.inputs['runtime/assets/extra.bin'] = path
        self.on_compile = add_member
        with self.assertRaisesRegex(ValueError, 'inputs changed'):
            self.build()
        self.assertFalse((self.intermediate / 'launcher-build-cache.json').exists())

    def test_personal_game_data_is_rejected_before_any_tool_or_cache_record(self):
        dump = self.runtime / 'not-a-real-cartridge.gba'
        dump.write_bytes(b'unit fixture only')
        self.inputs['runtime/not-a-real-cartridge.gba'] = dump
        with self.assertRaisesRegex(SystemExit, 'Personal game data'):
            self.build()
        self.assertEqual((self.stamps, self.compiles), (0, 0))
        self.assertFalse(self.intermediate.exists())

    def test_archive_metadata_and_payload_are_reproducible(self):
        first = builder.deterministic_archive(self.inputs)
        for path in self.inputs.values():
            os.utime(path, ns=(1_600_000_000_000_000_000,) * 2)
        second = builder.deterministic_archive(dict(reversed(list(self.inputs.items()))))
        self.assertEqual(first, second)
        import io
        with zipfile.ZipFile(io.BytesIO(first)) as archive:
            self.assertIsNone(archive.testzip())
            self.assertEqual(archive.namelist(), sorted(self.inputs))
            for info in archive.infolist():
                self.assertEqual(info.date_time, (1980, 1, 1, 0, 0, 0))
                self.assertEqual(info.create_system, 3)
                self.assertEqual(info.external_attr, 0o100644 << 16)


if __name__ == '__main__':
    unittest.main(verbosity=2)
