"""Pure file/cache regressions; no compiler, executable or game is launched."""
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from build_cache import (cache_hit, content_key, copy_if_changed, file_sha256,
                         record_cache, write_if_changed)


class BuildCacheTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix='kh-com-cache-')
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.source = self.root / 'source.bin'
        self.source.write_bytes(b'source one')
        self.output = self.root / 'output.bin'
        self.manifest = self.root / 'cache.json'

    def key(self, **kwargs):
        return content_key([self.source], recipe=['fake-tool', '-O2'], **kwargs)

    def test_second_identical_build_invokes_no_tool(self):
        invocations = []

        def build():
            key = self.key()
            if cache_hit(self.manifest, key, [self.output]):
                return
            invocations.append('fake-compile')
            self.output.write_bytes(b'artifact:' + self.source.read_bytes())
            if self.key() == key:
                record_cache(self.manifest, key, [self.output])

        build()
        output_mtime = self.output.stat().st_mtime_ns
        manifest_mtime = self.manifest.stat().st_mtime_ns
        build()
        self.assertEqual(invocations, ['fake-compile'])
        self.assertEqual(self.output.stat().st_mtime_ns, output_mtime)
        self.assertEqual(self.manifest.stat().st_mtime_ns, manifest_mtime)
        self.source.write_bytes(b'source two')
        build()
        self.assertEqual(len(invocations), 2)

    def test_same_size_and_mtime_changes_still_invalidate(self):
        original = self.source.stat()
        key = self.key()
        self.source.write_bytes(b'source two')
        os.utime(self.source, ns=(original.st_atime_ns, original.st_mtime_ns))
        self.assertNotEqual(self.key(), key)

    def test_mtime_only_change_does_not_invalidate(self):
        key = self.key()
        original = self.source.stat()
        os.utime(self.source, ns=(original.st_atime_ns, original.st_mtime_ns + 5000000))
        self.assertEqual(self.key(), key)

    def test_explicit_trees_track_nested_additions_removals_and_bytes(self):
        tree = self.root / 'sdk'
        (tree / 'nested').mkdir(parents=True)
        first = tree / 'nested' / 'header.bin'
        first.write_bytes(b'a')
        key = self.key(trees=[tree])
        first.write_bytes(b'b')
        modified = self.key(trees=[tree])
        self.assertNotEqual(modified, key)
        second = tree / 'added.bin'
        second.write_bytes(b'new header')
        added = self.key(trees=[tree])
        self.assertNotEqual(added, modified)
        second.unlink()
        self.assertEqual(self.key(trees=[tree]), modified)
        first.unlink()
        self.assertNotEqual(self.key(trees=[tree]), modified)

    def test_order_independent_files_and_environment_with_ordered_recipe(self):
        other = self.root / 'other.bin'
        other.write_bytes(b'other')
        first = content_key([self.source, other], recipe=['cc', '-O2'],
                            environment={'INCLUDE': 'one', 'LIB': 'two'})
        second = content_key([other, self.source], recipe=['cc', '-O2'],
                             environment={'LIB': 'two', 'INCLUDE': 'one'})
        self.assertEqual(first, second)
        self.assertNotEqual(first, content_key([self.source, other], recipe=['-O2', 'cc'],
                                              environment={'INCLUDE': 'one', 'LIB': 'two'}))
        self.assertNotEqual(first, content_key([self.source, other], recipe=['cc', '-O2'],
                                              environment={'INCLUDE': 'changed', 'LIB': 'two'}))

    def test_environment_not_implicitly_captured(self):
        key = self.key()
        with patch.dict(os.environ, {'KHCOM_FAKE_SECRET': 'never-captured'}):
            self.assertEqual(self.key(), key)

    def test_missing_inputs_and_trees_are_errors(self):
        with self.assertRaises(FileNotFoundError):
            content_key([self.root / 'missing.bin'])
        with self.assertRaises(FileNotFoundError):
            content_key(trees=[self.root / 'missing-sdk'])

    def test_output_bytes_missing_path_and_wrong_key_all_miss(self):
        self.output.write_bytes(b'one')
        key = self.key()
        record_cache(self.manifest, key, [self.output])
        self.assertTrue(cache_hit(self.manifest, key, [self.output]))
        self.assertFalse(cache_hit(self.manifest, '0' * 64, [self.output]))
        other = self.root / 'other.bin'
        other.write_bytes(b'one')
        self.assertFalse(cache_hit(self.manifest, key, [other]))
        self.assertFalse(cache_hit(self.manifest, key, []))
        stat = self.output.stat()
        self.output.write_bytes(b'two')
        os.utime(self.output, ns=(stat.st_atime_ns, stat.st_mtime_ns))
        self.assertFalse(cache_hit(self.manifest, key, [self.output]))
        self.output.unlink()
        self.assertFalse(cache_hit(self.manifest, key, [self.output]))

    def test_multiple_outputs_all_required(self):
        self.output.write_bytes(b'host')
        second = self.root / 'resource.bin'
        second.write_bytes(b'resource')
        key = self.key()
        record_cache(self.manifest, key, [self.output, second])
        self.assertTrue(cache_hit(self.manifest, key, [second, self.output]))
        second.write_bytes(b'broken')
        self.assertFalse(cache_hit(self.manifest, key, [self.output, second]))

    def test_corrupt_manifest_fails_closed(self):
        self.output.write_bytes(b'one')
        for payload in (b'{broken', b'null', b'[]', b'{}', b'\xff',
                        json.dumps({'schema': 999, 'key': self.key(), 'outputs': []}).encode()):
            self.manifest.write_bytes(payload)
            self.assertFalse(cache_hit(self.manifest, self.key(), [self.output]))

    def test_failed_build_or_missing_output_cannot_replace_record(self):
        self.output.write_bytes(b'old valid artifact')
        key = self.key()
        record_cache(self.manifest, key, [self.output])
        previous = self.manifest.read_bytes()
        with self.assertRaises(RuntimeError):
            # A tool exception stops the caller before record_cache.
            raise RuntimeError('mock tool failed')
        self.assertEqual(self.manifest.read_bytes(), previous)
        with self.assertRaises(FileNotFoundError):
            record_cache(self.manifest, key, [self.root / 'missing.bin'])
        self.assertEqual(self.manifest.read_bytes(), previous)

    def test_inputs_changing_during_mock_compile_are_not_cached(self):
        before = self.key()
        self.output.write_bytes(b'uncertain output')
        self.source.write_bytes(b'source two')
        if self.key() == before:
            record_cache(self.manifest, before, [self.output])
        self.assertFalse(self.manifest.exists())

    def test_atomic_manifest_failure_keeps_previous_bytes(self):
        self.manifest.write_bytes(b'previous valid record')
        self.output.write_bytes(b'one')
        with patch('build_cache.os.replace', side_effect=OSError('mock interrupted replace')):
            with self.assertRaises(OSError):
                record_cache(self.manifest, self.key(), [self.output])
        self.assertEqual(self.manifest.read_bytes(), b'previous valid record')
        self.assertEqual(list(self.root.glob('*.tmp')), [])

    def test_copy_and_write_unchanged_preserve_existing_file(self):
        destination = self.root / 'nested' / 'copy.bin'
        self.assertTrue(copy_if_changed(self.source, destination))
        self.assertEqual(file_sha256(self.source), file_sha256(destination))
        stat = destination.stat()
        self.assertFalse(copy_if_changed(self.source, destination))
        self.assertEqual(destination.stat().st_mtime_ns, stat.st_mtime_ns)
        self.assertTrue(write_if_changed(destination, 'new bytes'))
        stat = destination.stat()
        self.assertFalse(write_if_changed(destination, b'new bytes'))
        self.assertEqual(destination.stat().st_mtime_ns, stat.st_mtime_ns)


if __name__ == '__main__':
    unittest.main()
