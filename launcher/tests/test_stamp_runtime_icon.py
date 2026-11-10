"""Icon-stamping control flow, using only .bin fixtures and fake resource APIs."""
import ctypes
import importlib.util
import os
from pathlib import Path
import shutil
import struct
import tempfile
import unittest
from unittest.mock import patch


SPEC = importlib.util.spec_from_file_location(
    'stamp_runtime_icon', Path(__file__).resolve().parents[1] / 'stamp_runtime_icon.py')
stamp = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(stamp)


class FakeResourceApi:
    def __init__(self, initial, *, alter_metadata=False):
        self.initial = dict(initial)
        self.current = dict(initial)
        self.temporary = None
        self.alter_metadata = alter_metadata
        self.calls = []

    def BeginUpdateResourceW(self, path, delete_existing):
        self.temporary = Path(path).resolve()
        if self.temporary.suffix != '.bin':
            raise AssertionError('Tests may not write executable files')
        self.calls.append('begin')
        return 1

    def UpdateResourceW(self, handle, kind, name, language, data, size):
        key = (kind.value, name.value, language)
        if data is None:
            self.current.pop(key, None)
        else:
            self.current[key] = ctypes.string_at(data, size)
        self.calls.append(('update', key))
        return True

    def EndUpdateResourceW(self, handle, discard):
        self.calls.append(('end', discard))
        if not discard:
            if self.alter_metadata:
                self.current[(16, 1, 0)] = b'changed version metadata'
            self.temporary.write_bytes(self.temporary.read_bytes() + b'|resource-edit')
        return True

    def read(self, path):
        if self.temporary is not None and Path(path).resolve() == self.temporary:
            return dict(self.current)
        return dict(self.initial)


class StampIconTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix='icon-unit-')
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.source = self.root / 'source.bin'
        self.destination = self.root / 'destination.bin'
        self.source.write_bytes(b'PE-header-A|unchanged-code-and-data')
        self.icon = self.root / 'heart.ico'
        self.payloads = [b'frame16', b'frame32-with-alpha']
        header = struct.pack('<HHH', 0, 1, 2)
        offset = 6 + 2 * 16
        entries = b''
        for size, payload in zip((16, 32), self.payloads):
            entries += struct.pack('<BBBBHHII', size, size, 0, 0, 1, 32, len(payload), offset)
            offset += len(payload)
        self.icon.write_bytes(header + entries + b''.join(self.payloads))
        group = struct.pack('<HHH', 0, 1, 2)
        group += struct.pack('<BBBBHHIH', 16, 16, 0, 0, 1, 32, len(self.payloads[0]), 1)
        group += struct.pack('<BBBBHHIH', 32, 32, 0, 0, 1, 32, len(self.payloads[1]), 2)
        self.canonical = {
            (3, 1, 0): self.payloads[0],
            (3, 2, 0): self.payloads[1],
            (14, 1, 0): group,
            (16, 1, 0): b'unchanged version metadata',
            (24, 1, 0): b'unchanged manifest',
        }
        self.sections = {'.text': (4, b'code'), '.data': (4, b'data')}
        self.real_copy = shutil.copy2
        self.real_mkstemp = tempfile.mkstemp

    def copy_fixture(self, source, destination):
        self.assertEqual(Path(source).suffix, '.bin')
        self.assertEqual(Path(destination).suffix, '.bin')
        return self.real_copy(source, destination)

    def temporary_fixture(self, **kwargs):
        return self.real_mkstemp(prefix='icon-fixture-', suffix='.bin', dir=kwargs['dir'])

    def assert_verified(self, report):
        self.assertEqual(report['source_sha256'], stamp.sha256(self.source))
        self.assertEqual(report['stamped_sha256'], stamp.sha256(self.destination))
        self.assertEqual(report['icon_sha256'], stamp.sha256(self.icon))
        self.assertEqual(report['icon_frames'], 2)
        for key in ('non_icon_resources_preserved',
                    'icon_resources_reextracted_and_verified', 'original_unchanged'):
            self.assertTrue(report[key], key)

    def run_no_edit(self):
        with patch.object(stamp, 'resources', return_value=dict(self.canonical)), \
             patch.object(stamp, 'pe_sections', return_value=self.sections), \
             patch.object(stamp, 'api', side_effect=AssertionError('Unexpected resource edit')), \
             patch.object(stamp.tempfile, 'mkstemp', side_effect=AssertionError('Unexpected temp file')), \
             patch.object(stamp.shutil, 'copy2', side_effect=self.copy_fixture) as copy:
            report = stamp.stamp_icon(self.source, self.destination, self.icon)
        self.assert_verified(report)
        self.assertFalse(report['resource_edit_performed'])
        return report, copy.call_count

    def test_exact_destination_is_untouched_without_resource_edit_or_temp_file(self):
        self.destination.write_bytes(self.source.read_bytes())
        os.utime(self.destination, ns=(1_600_000_000_000_000_000, 1_600_000_000_000_000_000))
        before = self.destination.stat().st_mtime_ns
        report, copies = self.run_no_edit()
        self.assertFalse(report['changed'])
        self.assertTrue(report['reused'])
        self.assertEqual(copies, 0)
        self.assertEqual(self.destination.stat().st_mtime_ns, before)

    def test_missing_destination_copies_only_the_verified_source(self):
        report, copies = self.run_no_edit()
        self.assertTrue(report['changed'])
        self.assertFalse(report['reused'])
        self.assertEqual(copies, 1)
        self.assertEqual(self.destination.read_bytes(), self.source.read_bytes())

    def test_matching_sections_do_not_hide_different_pe_headers(self):
        self.destination.write_bytes(b'PE-header-B|unchanged-code-and-data')
        report, copies = self.run_no_edit()
        self.assertTrue(report['changed'])
        self.assertFalse(report['reused'])
        self.assertEqual(copies, 1)
        self.assertEqual(self.destination.read_bytes(), self.source.read_bytes())

    def test_copy_corruption_is_rejected(self):
        def corrupt_copy(source, destination):
            Path(destination).write_bytes(b'corrupt copy')
        with patch.object(stamp, 'resources', return_value=dict(self.canonical)), \
             patch.object(stamp, 'pe_sections', return_value=self.sections), \
             patch.object(stamp, 'api', side_effect=AssertionError('Unexpected resource edit')), \
             patch.object(stamp.tempfile, 'mkstemp', side_effect=AssertionError('Unexpected temp file')), \
             patch.object(stamp.shutil, 'copy2', side_effect=corrupt_copy):
            with self.assertRaisesRegex(RuntimeError, 'Copied runtime'):
                stamp.stamp_icon(self.source, self.destination, self.icon)

    def run_edit(self, initial, *, alter_metadata=False, alter_sections=False):
        api = FakeResourceApi(initial, alter_metadata=alter_metadata)
        original = self.source.read_bytes()

        def sections(path):
            if alter_sections and api.temporary is not None and Path(path).resolve() == api.temporary:
                return {'.text': (4, b'evil')}
            return self.sections

        with patch.object(stamp, 'resources', side_effect=api.read), \
             patch.object(stamp, 'pe_sections', side_effect=sections), \
             patch.object(stamp, 'api', return_value=api), \
             patch.object(stamp.tempfile, 'mkstemp', side_effect=self.temporary_fixture), \
             patch.object(stamp.shutil, 'copy2', side_effect=self.copy_fixture):
            report = stamp.stamp_icon(self.source, self.destination, self.icon)
        self.assertEqual(self.source.read_bytes(), original)
        self.assertEqual(api.current, self.canonical)
        self.assertEqual(list(self.root.glob('icon-fixture-*')), [])
        self.assert_verified(report)
        self.assertTrue(report['changed'])
        self.assertFalse(report['reused'])
        self.assertTrue(report['resource_edit_performed'])
        self.assertIn('begin', api.calls)
        return report

    def test_changed_icon_uses_the_verified_resource_edit_path(self):
        initial = dict(self.canonical)
        initial[(3, 1, 0)] = b'old icon frame'
        self.run_edit(initial)

    def test_extra_icon_group_is_not_a_noop(self):
        initial = dict(self.canonical)
        initial[(14, 2, 0)] = initial[(14, 1, 0)]
        self.run_edit(initial)

    def test_nonneutral_icon_language_is_not_a_canonical_noop(self):
        initial = {(kind, name, 1033 if kind in (3, 14) else language): payload
                   for (kind, name, language), payload in self.canonical.items()}
        self.run_edit(initial)

    def test_changed_icon_still_rejects_nonicon_metadata_changes(self):
        initial = dict(self.canonical)
        initial[(3, 1, 0)] = b'old icon frame'
        self.destination.write_bytes(b'previous destination')
        with self.assertRaisesRegex(RuntimeError, 'non-icon resource'):
            self.run_edit(initial, alter_metadata=True)
        self.assertEqual(self.destination.read_bytes(), b'previous destination')
        self.assertEqual(list(self.root.glob('icon-fixture-*')), [])

    def test_changed_icon_still_rejects_code_changes(self):
        initial = dict(self.canonical)
        initial[(3, 1, 0)] = b'old icon frame'
        self.destination.write_bytes(b'previous destination')
        with self.assertRaisesRegex(RuntimeError, 'Code/data sections'):
            self.run_edit(initial, alter_sections=True)
        self.assertEqual(self.destination.read_bytes(), b'previous destination')
        self.assertEqual(list(self.root.glob('icon-fixture-*')), [])


if __name__ == '__main__':
    unittest.main(verbosity=2)
