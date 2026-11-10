#!/usr/bin/env python3
"""Syntax-check the action adapter without linking or running game content.

Uses the project's real generated headers when available. With
--declarations-only it emits extern declarations from the public manifests
into a temporary directory; it neither fabricates nor reads ROM asset bytes.
This checks C types and syntax, not executable behavior or compatibility.

A compiler with a real 32-bit C runtime/header sysroot is required, for example
llvm-mingw's i686-w64-mingw32-clang. System GCC needs libc6-dev-i386 or equivalent.

Examples:
  python port/tools/check_action_sources.py --region us
  python port/tools/check_action_sources.py --cc /path/to/i686-w64-mingw32-clang
  python port/tools/check_action_sources.py --declarations-only --region eu
"""
import argparse
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_SOURCES = [
    'src/btl/btl_sora.c', 'src/btl/battle_runtime.c',
    'src/taskpool.c', 'src/btl/btl_collision.c', 'src/btl/btl_effect.c',
]


class DeclaredSize:
    """Only len() is needed by emit_header; contains no tile data."""
    def __init__(self, size):
        self.size = size

    def __len__(self):
        return self.size


def declarations(destination, region):
    sys.path.insert(0, str(ROOT / 'tools'))
    import assetgen
    count = 0
    for manifest in assetgen.load_manifests():
        members = manifest.members(region)
        sheets = {}
        for entries in members.values():
            for entry in entries:
                if entry.get('format') != 'sprite_sheet' or entry.get('tiles'):
                    continue
                # Match Gen.sheet_block's original record positions using
                # public piece/frame counts, without decoding cartridge data.
                frames = manifest.frames(entry, region)
                animations = manifest.animations(entry, region)
                records = sum(2 + 6 * len(f['pieces']) + (2 if f['pieces'] else 0) for f in frames)
                records += sum(6 + 4 * len(a['frames']) for a in animations)
                tile_size = entry[region]['size'] - records
                if tile_size < 0:
                    raise ValueError(f"negative tile extent: {manifest.group}/{entry['name']}")
                sheets[entry['name']] = (None, None, DeclaredSize(tile_size))
        assetgen.emit_header(manifest, region, members,
                             destination / f'{manifest.group}.h', sheets)
        count += 1
    return count


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default='gcc')
    parser.add_argument('--region', choices=('us', 'eu', 'jp'), default='us')
    parser.add_argument('--compiler-arg', action='append', default=[],
                        help='extra argument, e.g. --compiler-arg=--sysroot=/path')
    parser.add_argument('--source', action='append', default=[],
                        help='repository-relative translation unit; repeat to replace defaults')
    parser.add_argument('--declarations-only', action='store_true',
                        help='generate extern declarations from public manifests, without a ROM')
    parser.add_argument('--keep-headers', type=Path,
                        help='keep declaration headers here for inspection, outside real generated assets')
    args = parser.parse_args()
    cc = shutil.which(args.cc)
    if not cc:
        parser.error(f'compiler not found: {args.cc}')
    sources = [ROOT / s for s in (args.source or DEFAULT_SOURCES)]
    for source in sources:
        if not source.is_file():
            parser.error(f'translation unit not found: {source}')

    with tempfile.TemporaryDirectory(prefix='rekhcom-action-syntax-') as temporary:
        temp = Path(temporary)
        # Probe the actual ABI and libc headers. Never silence sizeof assertions
        # or invent SDK headers just to get a passing syntax check.
        probe = temp / 'probe.c'
        probe.write_text('#include <stdint.h>\n#include <stdlib.h>\n'
                         'typedef char pointer_size_must_be_four[sizeof(void*) == 4 ? 1 : -1];\n')
        base = [cc, '-m32', *args.compiler_arg]
        result = subprocess.run([*base, '-fsyntax-only', str(probe)], capture_output=True, text=True)
        if result.returncode:
            print(result.stderr, file=sys.stderr)
            raise SystemExit('32-bit compiler/SDK prerequisite failed; no game checks were run. '
                             'Use a 32-bit MinGW compiler or provide a proper 32-bit libc sysroot.')

        generated = ROOT / 'build' / args.region / 'gen'
        if args.declarations_only or not generated.is_dir():
            generated = args.keep_headers.resolve() if args.keep_headers else temp / 'declarations'
            canonical = (ROOT / 'build' / args.region / 'gen').resolve()
            if generated == canonical or canonical in generated.parents:
                parser.error('--keep-headers must not overwrite real generated assets')
            generated.mkdir(parents=True, exist_ok=True)
            count = declarations(generated, args.region)
            print(f'{args.region}: generated {count} public declaration headers (no ROM bytes)')
        else:
            print(f'{args.region}: using real generated headers')

        includes = [ROOT / 'include', *sorted(p for p in (ROOT / 'include').iterdir()
                                            if p.is_dir() and p.name != 'gba'),
                    generated, ROOT / 'port/include']
        flags = ['-fsyntax-only', '-std=gnu89', '-funsigned-char', '-fno-strict-aliasing',
                 '-fwrapv', '-mno-ms-bitfields', '-DPLATFORM_PC', f'-DVERSION_{args.region.upper()}',
                 '-Wno-implicit-function-declaration', '-Wno-int-conversion',
                 '-Wno-incompatible-pointer-types', '-Wno-pointer-to-int-cast',
                 '-Wno-int-to-pointer-cast', '-Wno-unknown-pragmas',
                 '-include', str(ROOT / 'port/include/port/game_prefix.h')]
        flags += [f'-I{p}' for p in includes]
        failed = 0
        for source in sources:
            result = subprocess.run([*base, *flags, str(source)], cwd=ROOT,
                                    capture_output=True, text=True)
            name = source.relative_to(ROOT)
            print(f'{"FAIL" if result.returncode else "PASS"} {name}')
            if result.returncode:
                failed += 1
                print(result.stderr, file=sys.stderr)
        if failed:
            raise SystemExit(f'{failed}/{len(sources)} translation units failed syntax checks')
        print(f'All {len(sources)} translation units passed; no link or gameplay test performed.')


if __name__ == '__main__':
    main()
