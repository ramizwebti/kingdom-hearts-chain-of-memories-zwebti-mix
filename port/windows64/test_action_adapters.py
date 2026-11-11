"""Run ROM-free production action adapters with verified Win32 actor layouts.

Hosted CI uses --compiler msvc from an installed Visual C++ x86 environment.
The default portable MinGW mode is retained for explicitly authorized local use.
"""
import argparse
import json
import os
from pathlib import Path
import re
import runpy
import shutil
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'port/tools'))

MSVC_COMPAT = r'''
/* Test-runner compatibility only. /Zp4 supplies the ARM aggregate alignment
 * normally requested by GNU aligned(4) typedefs. Production declarations and
 * their size assertions are retained; critical offsets are asserted below. */
#if !defined(_MSC_VER) || !defined(_M_IX86)
#error These adapter checks require the installed MSVC x86 C compiler.
#endif
#define __attribute__(spec)
#define __UINTPTR_TYPE__ unsigned int
'''

WIN32_ABI = r'''
#include <stddef.h>
typedef char AdapterPointerSize[(sizeof(void*)==4)?1:-1];
typedef char AdapterIntegerSize[(sizeof(int)==4)?1:-1];
'''

ACTOR_ABI = r'''
#include <stddef.h>
/* Retain the production headers' full size assertions and verify fields used
 * across collision, task and target adapters at their cartridge offsets. */
typedef char AdapterActorFlags[(offsetof(BtlObj,flags)==0x34)?1:-1];
typedef char AdapterActorCollider[(offsetof(BtlObj,collider)==0x40)?1:-1];
typedef char AdapterActorNode[(offsetof(BtlObj,node)==0xB8)?1:-1];
typedef char AdapterActorBattle[(offsetof(BtlObj,btl)==0xE4)?1:-1];
typedef char AdapterBattleFlags[(offsetof(BtlWork,flags)==0x68)?1:-1];
typedef char AdapterBattleActor[(offsetof(BtlWork,actor)==0x7C)?1:-1];
'''


def windows_collision_context():
    """Resolve PE reference tables without substituting collision geometry."""
    source=(ROOT/'src/engine.c').read_text()
    table=re.search(r'^const s16 gSineTable\[320\] = \{.*?^\};',
                    source,re.MULTILINE|re.DOTALL)
    if not table: raise RuntimeError('Production sine table was not found')
    values=table.group(0).split('{',1)[1].rsplit('}',1)[0]
    if len(re.findall(r'-?\d+',values))!=320:
        raise RuntimeError('Production sine table extent changed')
    # COFF .refptr entries can retain references from unused field-collider
    # sections. This fixture never calls field registration: leave its typed
    # context null so an accidental call fails rather than inventing a field.
    return ('\n#include "field_state.h"\nFieldState* gFieldState=0;\n'+
            table.group(0)+'\n')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler',choices=('mingw','msvc'),default='mingw')
    args=parser.parse_args()
    out=ROOT/'build/Kingdom-Hearts-Chain-of-Memories-Zwebti-Mix-Development/native/out'
    if args.compiler=='msvc': out=out/'ci/adapters'
    out.mkdir(parents=True,exist_ok=True)
    if args.compiler=='msvc':
        if os.environ.get('VSCMD_ARG_TGT_ARCH','').lower()!='x86':
            parser.error('MSVC mode requires vcvars32.bat (an x86 compiler environment)')
        compiler=shutil.which('cl.exe')
        if not compiler: parser.error('The installed MSVC compiler was not found')
        compat=out/'adapter-msvc-compat.h'
        compat.write_text(MSVC_COMPAT)
    else:
        toolchain=next(p for p in sorted((ROOT.parent/'toolchain').glob('llvm-mingw-*')) if p.is_dir())
        compiler=toolchain/'bin/clang.exe'
    source=(ROOT/'src/btl/btl_sora.c').read_text()
    target=runpy.run_path(str(ROOT/'port/tools/test_action_targeting.py'))
    collision=runpy.run_path(str(ROOT/'port/tools/test_action_collision.py'))
    assist=runpy.run_path(str(ROOT/'port/tools/test_action_assist_phases.py'))
    portrait=runpy.run_path(str(ROOT/'port/tools/test_action_portrait.py'))
    start=source.rfind('enum',0,source.index('BTL_SORA_STATE_ENTER,'))
    states=source[start:source.index('};',start)+2]
    names=['ActionFullNative','ActionQuickMagic','ActionTargetUsable','ActionQuickFireSpawnOffset',
           'ActionLiveMeleeTarget','ActionSelectMeleeTarget','PortActionLockMeleeTarget',
           'ActionApproachMeleeTarget','PortActionPlayerMeleeHit']
    functions=target['extract']((ROOT/'src/btl/btl_collision.c').read_text(),'CanAttackBoxHitBtlObj')+'\n'
    functions+='\n'.join(target['extract'](source,n) for n in names)
    harnesses={
        'target':(target['HARNESS'].replace('__STATES__',states).replace('__FUNCTIONS__',functions),
                  ['src/listpool.c','port/src/port_action.c','port/src/port_action_routes.c','port/src/port_action_catalog.c']),
        'collision':(collision['HARNESS'],['src/btl/btl_collision.c','src/listpool.c']),
        'assist-phases':(assist['source_harness'](),[]),
        'portrait':(portrait['source_harness'](),[]),
    }
    includes=[ROOT/'include',ROOT/'port/include']+sorted(p for p in (ROOT/'include').iterdir()
                   if p.is_dir() and p.name!='gba')
    environment=dict(os.environ)
    temporary=ROOT/'build/Kingdom-Hearts-Chain-of-Memories-Zwebti-Mix-Development/temp/ci-adapters'
    temporary.mkdir(parents=True,exist_ok=True)
    environment.update(TEMP=str(temporary),TMP=str(temporary))
    report=out/'action-adapters-report.json'
    report.write_text(json.dumps(dict(passed=False,status='started',compiler=args.compiler))+'\n')
    results=[]
    for name,(harness,units) in harnesses.items():
        if name=='collision': harness+=windows_collision_context()
        # The Linux fixtures' only platform code is their exit syscall. Keep
        # all production functions/assertions and use the normal Windows CRT.
        if name in ('target','collision'):
            harness,count=re.subn(r'void _start\(void\)\s*\{[^{}]*\}',
                'int main(void) { return run_tests(); }',harness)
            if count!=1: raise RuntimeError('Unexpected fixture entry point: '+name)
            harness+=ACTOR_ABI
        harness+=WIN32_ABI
        if args.compiler=='msvc' and name=='target':
            # Linux's freestanding fixture supplies memset. This CRT runner
            # uses the real library function rather than redefining an MSVC
            # intrinsic. No production body or assertion is rewritten.
            harness,count=re.subn(r'void\* memset\(void\* p,int v,size_t n\)\s*\{[^{}]*\}',
                '#include <string.h>\n',harness)
            if count!=1: raise RuntimeError('Unexpected freestanding memset fixture')
        c=out/('adapter-'+name+'.c');exe=out/('adapter-'+name+'.exe')
        c.write_text(harness)
        if args.compiler=='msvc':
            command=[str(compiler),'/nologo','/TC','/std:c11','/O2','/W3','/J','/Zp4',
                     '/Gy','/Gw','/DPLATFORM_PC','/DVERSION_US','/D_CRT_SECURE_NO_WARNINGS',
                     '/FI'+str(compat),*['/I'+str(p) for p in includes],str(c),
                     *[str(ROOT/p) for p in units],'/Fe'+str(exe),'/link',
                     '/OPT:REF','/INCREMENTAL:NO','/MACHINE:X86']
        else:
            command=[str(compiler),'--target=i686-w64-mingw32','-O2','-std=gnu89',
                     '-funsigned-char','-fno-strict-aliasing','-fwrapv','-mno-ms-bitfields',
                     '-ffunction-sections','-fdata-sections','-DPLATFORM_PC','-DVERSION_US',
                     '-Wno-implicit-function-declaration','-Wno-incompatible-pointer-types',
                     '-Wl,--gc-sections',*['-I'+str(p) for p in includes],str(c),
                     *[str(ROOT/p) for p in units],'-o',str(exe)]
        subprocess.run(command,cwd=out,env=environment,check=True)
        result=subprocess.run([str(exe)],cwd=out,env=environment,timeout=30)
        if result.returncode: raise RuntimeError(name+' failed at check '+str(result.returncode))
        results.append(dict(name=name,passed=True))
        print('PASS: production Win32 '+name+' adapter checks ('+args.compiler+')',flush=True)
    report.write_text(json.dumps(dict(passed=True,compiler=args.compiler,bits=32,tests=results),indent=2)+'\n')


if __name__=='__main__': main()
