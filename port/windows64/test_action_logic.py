"""Compile/run focused action regressions in the stable COM development folder."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'port/windows64'))
from build_cache import content_key, cache_hit, record_cache


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--only', help='comma-separated test names')
    args = parser.parse_args()
    out = ROOT / 'build/Kingdom-Hearts-Chain-of-Memories-Zwebti-Mix-Development/native/out'
    out.mkdir(parents=True, exist_ok=True)
    # The existing renderer fixture saves its atlas under a relative build/.
    (out / 'build').mkdir(exist_ok=True)
    toolchain = next(p for p in sorted((ROOT.parent/'toolchain').glob('llvm-mingw-*')) if p.is_dir())
    sdk = next(p for p in (ROOT.parent/'toolchain').glob('SDL3-*') if p.is_dir()) / 'x86_64-w64-mingw32'
    compiler = toolchain / 'bin/clang.exe'
    common = ['port/src/port_action.c', 'port/src/port_action_routes.c', 'port/src/port_action_catalog.c']
    tests = {
        'routes': (['port/tools/test_action_routes.c', *common], []),
        'invariants': (['port/tools/test_action_combat.c', *common], []),
        'pressure': (['port/tools/test_action_pressure.c'], []),
        'feedback': (['port/tools/test_action_feedback.c', 'port/src/port_action_feedback.c'], []),
        'effects': (['port/tools/test_action_fx.c', 'port/src/port_action_fx.c'], ['-DPORT_ACTION_FX_NO_RENDER']),
        'effects-render': (['port/tools/test_action_fx.c', 'port/src/port_action_fx.c'], ['-DPORT_ACTION_FX_RENDER_TEST']),
    }
    wanted = args.only.split(',') if args.only else list(tests)
    if set(wanted)-set(tests): parser.error('Unknown test name')
    environment = dict(os.environ)
    temporary = out.parents[1] / 'temp'
    temporary.mkdir(exist_ok=True)
    environment.update(TEMP=str(temporary), TMP=str(temporary))
    results = []
    for name in wanted:
        sources, defines = tests[name]
        exe = out / ('action-' + name + '.exe')
        flags = ['--target=x86_64-w64-mingw32', '-std=c11', '-O2', '-Wall', '-Wextra',
                 '-I'+str(ROOT/'port/include'), '-I'+str(ROOT/'include'),
                 '-I'+str(ROOT/'include/card'), '-I'+str(sdk/'include')]
        command = [str(compiler), *flags, *defines, *(str(ROOT/p) for p in sources),
                   '-L'+str(sdk/'lib'), '-lSDL3', '-o', str(exe)]
        inputs = [compiler, Path(__file__), *(ROOT/p for p in sources)]
        key = content_key(inputs, trees=[ROOT/'port/include/port', ROOT/'include/card', sdk/'include', sdk/'lib'],
                          recipe={'command': command})
        manifest = out / ('action-' + name + '-cache.json')
        if not cache_hit(manifest, key, [exe]):
            subprocess.run(command, cwd=ROOT, env=environment, check=True)
            after = content_key(inputs, trees=[ROOT/'port/include/port', ROOT/'include/card', sdk/'include', sdk/'lib'],
                                recipe={'command': command})
            if after != key: raise RuntimeError('Inputs changed during compilation; wait for edits to finish')
            record_cache(manifest, key, [exe])
        result = subprocess.run([str(exe)], cwd=out, env=environment, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
        print(result.stdout, end='', flush=True)
        results.append(dict(name=name, passed=result.returncode==0, exit=result.returncode))
        if result.returncode: raise RuntimeError(name+' failed with exit '+str(result.returncode))
    (out/'action-logic-report.json').write_text(json.dumps(dict(passed=True, tests=results), indent=2)+'\n')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
