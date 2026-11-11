#!/usr/bin/env python3
"""Check production companion phase gates without ROM assets or a game host.

Compiles the actual gate statements extracted from ActionAssistUpdate; task
execution and presentation predicates are test inputs. In particular, Blitz's
airborne pose 7 still owns its hitbox because native code rewinds it to pose 6.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_OUT = ROOT / 'build/Kingdom-Hearts-Chain-of-Memories-Zwebti-Mix-Development/native/out'

HARNESS = r'''
#include <stdio.h>
__STATES__
enum { MOVE_BLITZ=1, MOVE_SONIC_BLADE, MOVE_ARS_ARCANUM };
typedef struct { int z,groundZ; } Body;
typedef struct { unsigned frame; } Anim;
typedef struct { Body actor;Anim anim;unsigned state,stateTimer,steps; } Sora;
typedef struct { const void* nativeDesc;void* native;unsigned ownsEffect,paused,move; } Assist;
typedef struct { unsigned nativeMove,comboStep; } Intent;
static char gTaskDescFrdGoofy,gTaskDescFrdBeast,gTaskDescFrdAladdin,gTaskDescFrdAriel,sOther;
static unsigned sActionIndependent=1,gFrameCounter,sEffect,sFull,sQuick,sRecovery,sTicks;
static Sora sora;
static Sora* sActionWork=&sora;
static Intent sActionCurrent;
static int BgFxIsActive(void) { return sEffect; }
static int ActionFullNative(void) { return sFull; }
static int ActionQuickMagic(void) { return sQuick; }
static int ActionNativeRecoveryReady(Sora* work) { (void)work;return sRecovery; }
static unsigned ActionAnimationTicks(Sora* work) { (void)work;return sTicks; }
static unsigned AnimGetFrame(Anim* anim) { return anim->frame; }
static int ActionTraceEnabled(void) { return 0; }
static void PortLog(const char* fmt,...) { (void)fmt; }
static int phase_gate(Assist* work) {
__GATE__
    return paused;
}
#define CHECK(test) do { ++checks;if(!(test)) { fprintf(stderr,"failed phase check %u at line %u\n",checks,__LINE__);return 1; } } while(0)
int main(void) {
    unsigned checks=0,i;
    const void* physical[]={&gTaskDescFrdGoofy,&gTaskDescFrdBeast,&gTaskDescFrdAladdin,&gTaskDescFrdAriel};
    Assist assist={&gTaskDescFrdAriel,&sOther,0,0,0};
    sActionCurrent.comboStep=1;sora.state=BTL_SORA_STATE_COMBO;
    sora.steps=0;CHECK(!phase_gate(&assist)); /* startup can coexist */
    sora.steps=1;CHECK(phase_gate(&assist)); /* global reset cannot reopen normal */
    sora.steps=2;CHECK(!phase_gate(&assist)); /* recovery can coexist */
    sFull=1;sActionCurrent.nativeMove=MOVE_BLITZ;sora.state=BTL_SORA_STATE_BLITZ_STRIKE;
    for(i=0;i<4;++i) {
        assist.nativeDesc=physical[i];sora.actor.z=-256;sora.actor.groundZ=0;
        sora.anim.frame=5;CHECK(!phase_gate(&assist));
        sora.anim.frame=6;CHECK(phase_gate(&assist));
        sora.anim.frame=7;CHECK(phase_gate(&assist)); /* next tick rewinds to 6 */
        sora.actor.z=0;CHECK(!phase_gate(&assist)); /* grounded 7 no longer rewinds */
        sora.anim.frame=8;CHECK(!phase_gate(&assist));
    }
    assist.nativeDesc=&sOther;CHECK(phase_gate(&assist));
    assist.nativeDesc=&gTaskDescFrdAriel;sActionCurrent.nativeMove=MOVE_SONIC_BLADE;
    sora.state=BTL_SORA_STATE_SONIC_BLADE;
    sTicks=21;CHECK(!phase_gate(&assist));sTicks=22;CHECK(phase_gate(&assist));
    sTicks=64;CHECK(phase_gate(&assist));sTicks=65;CHECK(!phase_gate(&assist));
    sFull=0;sQuick=1;sora.stateTimer=13;CHECK(!phase_gate(&assist));
    sora.stateTimer=14;CHECK(phase_gate(&assist));
    sQuick=0;sActionCurrent.comboStep=0;sEffect=1;CHECK(!phase_gate(&assist));
    assist.nativeDesc=&sOther;CHECK(phase_gate(&assist));
    assist.ownsEffect=1;CHECK(!phase_gate(&assist));
    printf("PASS: %u production companion phase boundary checks\n",checks);
    return 0;
}
'''


def source_harness():
    source = (ROOT / 'src/btl/btl_sora.c').read_text()
    enum_start = source.rfind('enum', 0, source.index('BTL_SORA_STATE_ENTER,'))
    states = source[enum_start:source.index('};', enum_start) + 2]
    update = source.index('static u8 ActionAssistUpdate(')
    start = source.index('    int effectBefore=', update)
    end = source.index('    work->paused=(u8)paused;', start)
    gate = source[start:end]
    if not re.search(r'case MOVE_BLITZ:', gate):
        raise ValueError('production companion gate not found')
    return HARNESS.replace('__STATES__', states).replace('__GATE__', gate)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default=os.environ.get('CC', 'cc'))
    parser.add_argument('--out', type=Path, default=DEFAULT_OUT)
    args = parser.parse_args()
    out = args.out.resolve()
    if not out.is_relative_to((ROOT / 'build').resolve()):
        parser.error('--out must stay in this checkout build directory')
    out.mkdir(parents=True, exist_ok=True)
    source = out / 'adapter-assist-phases.c'
    binary = out / ('adapter-assist-phases.exe' if os.name == 'nt' else 'adapter-assist-phases')
    source.write_text(source_harness())
    subprocess.run([args.compiler, '-std=c99', '-O2', str(source), '-o', str(binary)], check=True)
    return subprocess.run([str(binary)], check=False).returncode


if __name__ == '__main__':
    raise SystemExit(main())
