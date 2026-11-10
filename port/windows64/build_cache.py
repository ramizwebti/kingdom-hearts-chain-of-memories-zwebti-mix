"""Content-only cache primitives for this project's Windows build scripts.

Callers enumerate every input, include tree, toolchain/SDK tree, recipe and
relevant environment value explicitly. Nothing is inferred from the working
directory or the complete environment, and this module never runs a tool.

Compute the key before building, check it again after a successful build, then
record the outputs only if both keys match. A hit verifies every output's bytes;
an incomplete build, missing file or corrupt manifest is always a miss.
"""
import hashlib
import json
import os
from pathlib import Path
import shutil
import tempfile

SCHEMA = 1


def _identity(path):
    return os.path.normcase(str(Path(path).resolve()))


def file_sha256(path):
    """Hash a complete file, rejecting an observable change during the read."""
    path = Path(path)
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        before = os.fstat(stream.fileno())
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
        after = os.fstat(stream.fileno())
    if (before.st_size, before.st_mtime_ns, before.st_ctime_ns) != (
            after.st_size, after.st_mtime_ns, after.st_ctime_ns):
        raise ValueError('Build input changed while being hashed: ' + str(path))
    return digest.hexdigest()


def _files(paths):
    found = {}
    for value in paths:
        path = Path(value)
        if not path.is_file():
            raise FileNotFoundError('Missing build file: ' + str(path))
        found[_identity(path)] = path
    return [
        {'path': name, 'sha256': file_sha256(path)}
        for name, path in sorted(found.items())
    ]


def content_key(paths=(), *, trees=(), recipe=(), environment=None):
    """Return a SHA256 of explicit file bytes, tree membership and build recipe.

    Tree paths must exist. Added/deleted files invalidate the key. Directory
    symlinks are rejected so an untraversed dependency cannot silently match.
    File mtimes never determine validity. Only the supplied environment mapping
    is included; callers should pass relevant values, never their whole env.
    """
    tree_records = []
    for identity, path in sorted({_identity(p): Path(p) for p in trees}.items()):
        if not path.is_dir():
            raise FileNotFoundError('Missing build tree: ' + str(path))
        entries = []
        for child in sorted(path.rglob('*')):
            if child.is_dir() and child.is_symlink():
                raise ValueError('Build tree contains a directory symlink: ' + str(child))
            if child.is_file():
                entries.append({
                    'path': child.relative_to(path).as_posix(),
                    'resolved': _identity(child),
                    'sha256': file_sha256(child),
                })
            elif not child.is_dir():
                raise FileNotFoundError('Unreadable build tree entry: ' + str(child))
        tree_records.append({'root': identity, 'files': entries})
    payload = {
        'schema': SCHEMA,
        'files': _files(paths),
        'trees': tree_records,
        'recipe': recipe,
        'environment': {} if environment is None else dict(environment),
    }
    encoded = json.dumps(payload, sort_keys=True, separators=(',', ':'),
                         ensure_ascii=True, allow_nan=False).encode('utf-8')
    return hashlib.sha256(encoded).hexdigest()


def _output_records(outputs):
    paths = list(outputs)
    if not paths:
        raise ValueError('A build cache target must have at least one output')
    records = _files(paths)
    for item in records:
        item['size'] = Path(item['path']).stat().st_size
    return records


def cache_hit(manifest, key, outputs):
    """Fail closed unless key, exact output paths and all output hashes match."""
    try:
        record = json.loads(Path(manifest).read_text(encoding='utf-8'))
        return (record['schema'] == SCHEMA and record['key'] == key
                and record['outputs'] == _output_records(outputs))
    except (OSError, ValueError, TypeError, KeyError):
        return False


def record_cache(manifest, key, outputs):
    """Publish only complete output hashes; never call this after a failed tool."""
    if not isinstance(key, str) or len(key) != 64 or any(
            character not in '0123456789abcdef' for character in key):
        raise ValueError('Build cache key must be a SHA256 hex digest')
    record = {'schema': SCHEMA, 'key': key, 'outputs': _output_records(outputs)}
    write_if_changed(manifest, json.dumps(record, sort_keys=True, indent=2) + '\n')


def _temporary(destination):
    destination = Path(destination)
    destination.parent.mkdir(parents=True, exist_ok=True)
    descriptor, name = tempfile.mkstemp(prefix='.' + destination.name + '-',
                                      suffix='.tmp', dir=destination.parent)
    return descriptor, Path(name)


def write_if_changed(path, data, encoding='utf-8'):
    """Atomically write bytes/text only when their content differs."""
    path = Path(path)
    if isinstance(data, str):
        data = data.encode(encoding)
    if not isinstance(data, bytes):
        raise TypeError('Build output must be bytes or text')
    try:
        if path.read_bytes() == data:
            return False
    except FileNotFoundError:
        pass
    descriptor, temporary = _temporary(path)
    try:
        with os.fdopen(descriptor, 'wb') as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
        return True
    finally:
        temporary.unlink(missing_ok=True)


def copy_if_changed(source, destination):
    """Copy an existing artifact only when the destination bytes differ."""
    source, destination = Path(source), Path(destination)
    if not source.is_file():
        raise FileNotFoundError(source)
    source_digest = file_sha256(source)
    try:
        if source_digest == file_sha256(destination):
            return False
    except FileNotFoundError:
        pass
    descriptor, temporary = _temporary(destination)
    try:
        with os.fdopen(descriptor, 'wb') as target, source.open('rb') as origin:
            shutil.copyfileobj(origin, target)
            target.flush()
            os.fsync(target.fileno())
        if file_sha256(temporary) != source_digest:
            raise ValueError('Artifact changed while being copied: ' + str(source))
        shutil.copystat(source, temporary)
        os.replace(temporary, destination)
        return True
    finally:
        temporary.unlink(missing_ok=True)
