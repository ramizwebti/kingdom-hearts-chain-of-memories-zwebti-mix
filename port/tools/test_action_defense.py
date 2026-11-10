#!/usr/bin/env python3
"""Exercise the unchanged live perfect-dodge observer with real 32-bit actors.

The controller reward is linked from its production source; only render/log
sinks and the battle-active predicate are stubbed. Geometry, timing, actor
flags, reward deduplication and counter dispatch execute actual game code.
"""
import errno
import os
from pathlib import Path
import subprocess
import tempfile
from test_action_collision import run_elf32
from test_action_cardfree import function

ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "btl.h"
#include "battle_work.h"
#include "battle_actor_types.h"
#include "port/port_action.h"
#include "port/port_action_fx.h"
#include <stddef.h>
static BtlSoraWork sSora;
static BtlSoraWork* sActionWork = &sSora;
static PortActionState sActionState;
static int sActive = 1, sEffects;
unsigned gFrameCounter;
int PortActionCombatActive(void) { return sActive; }
static int ActionTraceEnabled(void) { return 0; }
void PortLog(const char* fmt, ...) { (void)fmt; }
void PortActionFxCue(int32_t x, int32_t y, int32_t z, unsigned cue,
                     int facingLeft, unsigned variant) {
    (void)x; (void)y; (void)z; (void)facingLeft;
    if (cue == PORT_ACTION_FX_GUARD && variant == 1) sEffects++;
}
void* memset(void* ptr, int value, size_t n) {
    unsigned char* p = ptr;
    while (n--) *p++ = (unsigned char)value;
    return ptr;
}
__STATES__
__OBSERVER__
#define CHECK(test, code) do { if (!(test)) return (code); } while (0)
static void overlap(void) { PortActionObserveEnemyAttack(0,0,-2048,4,4,4); }
static void roll(void) {
    sSora.state = BTL_SORA_STATE_DODGE;
    sSora.stateTimer = 2;
    sSora.actor.flags = BTLOBJ_FLAG_HIT_LOCKED;
    sSora.actor.badStatus = BAD_STATUS_NONE;
    sSora.actor.hp = 80;
    sSora.actor.invincibleTimer = 0;
    sSora.actor.delayedDamage = 0;
    PortActionBeginDodge(&sActionState);
}
static int run_tests(void) {
    unsigned i;
    PortActionInput input = {0};
    PortActionIntent intent;
    PortActionReset(&sActionState);
    sActionState.mp = 10;
    sSora.actor.height = 24;
    sSora.actor.radiusX = 6;
    sSora.actor.radiusY = 4;
    roll();
    sActive = 0; overlap(); CHECK(!sEffects, 1);
    sActive = 1; sSora.state = BTL_SORA_STATE_IDLE; overlap(); CHECK(!sEffects, 2);
    roll(); sSora.stateTimer = 1; overlap(); CHECK(!sEffects, 3);
    sSora.stateTimer = 8; overlap(); CHECK(!sEffects, 4);
    roll(); sSora.actor.flags = 0; overlap(); CHECK(!sEffects, 5);
    roll(); sSora.actor.hp = 0; overlap(); CHECK(!sEffects, 6);
    roll(); sSora.actor.invincibleTimer = 1; overlap(); CHECK(!sEffects, 7);
    roll(); sSora.actor.delayedDamage = 2; overlap(); CHECK(!sEffects, 8);
    roll(); sSora.actor.flags |= BTLOBJ_FLAG_DAMAGE_PENDING; overlap(); CHECK(!sEffects, 9);
    roll(); sSora.actor.flags |= BTLOBJ_FLAG_HURT; overlap(); CHECK(!sEffects, 10);
    roll(); sSora.actor.flags |= BTLOBJ_FLAG_INTANGIBLE; overlap(); CHECK(!sEffects, 11);
    roll(); sSora.actor.flags |= BTLOBJ_FLAG_UNHITTABLE; overlap(); CHECK(!sEffects, 12);
    roll(); sSora.actor.badStatus = BAD_STATUS_STOP; overlap(); CHECK(!sEffects, 13);
    roll(); PortActionObserveEnemyAttack(10000,0,-2048,4,4,4); CHECK(!sEffects, 14);
    PortActionObserveEnemyAttack(-10000,0,-2048,4,4,4); CHECK(!sEffects, 15);
    PortActionObserveEnemyAttack(0,10000,-2048,4,4,4); CHECK(!sEffects, 16);
    PortActionObserveEnemyAttack(0,-10000,-2048,4,4,4); CHECK(!sEffects, 17);
    PortActionObserveEnemyAttack(0,0,10000,4,4,4); CHECK(!sEffects, 18);
    PortActionObserveEnemyAttack(0,0,-10000,4,4,4); CHECK(!sEffects, 19);
    PortActionObserveEnemyAttack(0,0,-2048,-1,4,4); CHECK(!sEffects, 20);
    CHECK(sActionState.mp == 10, 21);
    overlap();
    CHECK(sEffects == 1 && sActionState.mp == 20, 22);
    CHECK(sActionState.counterFrames == PORT_ACTION_COUNTER_FRAMES, 23);
    for (i=0;i<20;i++) overlap();
    CHECK(sEffects == 1 && sActionState.mp == 20, 24);
    input.pressed = PORT_ACTION_ATTACK; input.canAct = 1;
    PortActionTick(&sActionState, &input);
    CHECK(PortActionPeek(&sActionState, &intent) && intent.command == PORT_ACTION_GROUND_3, 25);
    CHECK(PortActionCommit(&sActionState, &intent), 26);
    CHECK(!sActionState.counterFrames, 27);
    roll(); sSora.stateTimer = 7; overlap();
    CHECK(sEffects == 2 && sActionState.mp == 30, 28);
    CHECK(sSora.actor.flags == BTLOBJ_FLAG_HIT_LOCKED && sSora.actor.hp == 80, 29);
    return 0;
}
void _start(void) {
    int code = run_tests();
    __asm__ volatile ("int $0x80" : : "a"(1), "b"(code));
    __builtin_unreachable();
}
'''

def main():
    game = (ROOT / 'src/btl/btl_sora.c').read_text()
    observer = function(game, 'PortActionObserveEnemyAttack')
    enum_start = game.rfind('enum', 0, game.index('BTL_SORA_STATE_ENTER,'))
    states = game[enum_start:game.index('};', enum_start)+2]
    includes = [ROOT / 'include', ROOT / 'port/include']
    includes += sorted(p for p in (ROOT/'include').iterdir() if p.is_dir() and p.name != 'gba')
    with tempfile.TemporaryDirectory(prefix='rekhcom-defense-') as temporary:
        folder = Path(temporary)
        source, exe = folder/'defense.c', folder/'defense'
        source.write_text(HARNESS.replace('__OBSERVER__', observer).replace('__STATES__', states))
        (folder/'string.h').write_text('#include <stddef.h>\nvoid* memset(void*, int, size_t);\n')
        cmd = [os.environ.get('CC','gcc'), '-m32', '-O2', '-std=gnu89', '-ffreestanding',
               '-fno-builtin', '-fno-stack-protector', '-fno-pie', '-ffunction-sections',
               '-fdata-sections', '-DPLATFORM_PC', '-DVERSION_US', '-nostdlib', '-static',
               '-no-pie', '-Wl,--gc-sections']
        cmd += ['-I'+str(folder)] + ['-I'+str(p) for p in includes]
        subprocess.run(cmd+[str(source),str(ROOT/'port/src/port_action.c'),'-o',str(exe)], check=True)
        try: code = subprocess.run([str(exe)], check=False).returncode
        except OSError as e:
            if e.errno != errno.ENOEXEC: raise
            code = run_elf32(exe)
        if code: raise SystemExit('FAIL defense check '+str(code))
    print('PASS: 29 actual dodge timing, hitbox, invulnerability, reward and counter checks')

if __name__ == '__main__': main()
