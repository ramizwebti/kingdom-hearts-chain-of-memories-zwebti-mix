#!/usr/bin/env python3
"""Execute production Shadow dispatch and callback ownership with real x32 layouts.

The function bodies are extracted unchanged from the live adapter. The real
list-pool implementation is linked; only the clock, random roll, control switch
and the underlying Shadow AI callback are fixtures. Native gameplay coverage
separately verifies the authored attack animation and damage.
"""
import errno
import os
from pathlib import Path
import re
import subprocess
import tempfile
from test_action_collision import run_elf32

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    match = re.search(r"^(?:static\s+)?(?:int|u8|void)\s+" + name + r"\([^;]*?\)\s*\{", source, re.M)
    if not match: raise ValueError("Function not found: " + name)
    depth, cursor = 1, match.end()
    while depth:
        depth += (source[cursor] == "{") - (source[cursor] == "}")
        cursor += 1
    return source[match.start():cursor]


HARNESS = r'''
#include <stddef.h>
#include "battle_work.h"
#include "enemy_types.h"
#include "enemy_ids.h"
#include "btl_collision.h"
#include "port/port_action_fx.h"
#define PORT_ENV_ONCE(out,getter,name) ((out)=(const char*)0)
static BtlWork sBattle;
BtlWork* gBtlWork = &sBattle;
u32 gFrameCounter;
static BtlObj sActors[4];
static EmyWork sWork;
static BtlObj* sSource;
static BtlObj* sSeenSource;
static int sEnabled = 1;
static unsigned sThreatRemaining, sThreatTotal;
static uintptr_t sThreatKey;
static int32_t sThreatX;
void PortActionFxThreat(uintptr_t key, int32_t x, int32_t y, int32_t groundZ,
                        int32_t headZ, unsigned remaining, unsigned total, int left) {
    (void)y; (void)groundZ; (void)headZ; (void)left;
    sThreatKey=key; sThreatX=x; sThreatRemaining=remaining; sThreatTotal=total;
}
static u32 sLastActionShadowAttackFrame;
int PortActionCombatRules(void) { return sEnabled; }
static int ActionShadowTraceEnabled(void) { return 0; }
void PortLog(const char* fmt, ...) { (void)fmt; }
u16 GetRandom(void) { return 0; }
void GetEnemyTargetPosition(BtlObj* actor, s32* x, s32* y, s32* z) {
    (void)actor; if (x) *x = 0; if (y) *y = 0; if (z) *z = 0;
}
BtlObj* BtlSetAttackSource(BtlObj* source) {
    BtlObj* previous = sSource; sSource = source; return previous;
}
u8 task_emy_00_1(EmyWork* work) {
    (void)work; sSeenSource = sSource; sSource = &sActors[3]; return 1;
}
__STATES__
__FUNCTIONS__

#define CHECK(test, code) do { if (!(test)) return (code); } while (0)
static int run_tests(void) {
    unsigned i;
    sBattle.phase = BTL_PHASE_IDLE;
    sBattle.enemyCount = 4;
    ListPoolInit(&sBattle.pool);
    for (i = 0; i < 4; i++) {
        sActors[i].kind = ENEMY_SHADOW;
        sActors[i].hp = 30;
        sActors[i].cardInterval = 1;
        sActors[i].attackRangeX = sActors[i].attackRangeY = 32;
        ListNodeInit(&sActors[i].node, &sBattle.pool, &sActors[i]);
        ListPoolAppend(&sActors[i].node, &sBattle.pool);
    }
    CHECK(PortActionShadowIndependent(&sActors[0]), 1);
    CHECK(!PortActionShadowIndependent(0), 2);
    gFrameCounter = 100;
    RequestEnemyCardUse(&sActors[0]);
    CHECK(sActors[0].flags & BTLOBJ_FLAG_CARD_ACTION_PENDING, 3);
    CHECK(sBattle.actor4 == 0, 4);
    RequestEnemyCardUse(&sActors[1]);
    CHECK(!sActors[1].flags && sBattle.actor4 == 0, 5);
    gFrameCounter += 12;
    RequestEnemyCardUse(&sActors[1]);
    CHECK(sActors[1].flags & BTLOBJ_FLAG_CARD_ACTION_PENDING, 6);
    gFrameCounter += 12;
    RequestEnemyCardUse(&sActors[2]);
    CHECK(sActors[2].flags & BTLOBJ_FLAG_CARD_ACTION_PENDING, 7);
    gFrameCounter += 12;
    RequestEnemyCardUse(&sActors[3]);
    CHECK(!sActors[3].flags && sBattle.actor4 == 0, 8);
    sActors[0].flags = BTLOBJ_FLAG_IN_CARD_ACTION;
    RequestEnemyCardUse(&sActors[3]);
    CHECK(!sActors[3].flags, 9); /* An attacking enemy also occupies the budget. */
    sActors[0].hp = 0;
    RequestEnemyCardUse(&sActors[3]);
    CHECK(sActors[3].flags & BTLOBJ_FLAG_CARD_ACTION_PENDING, 10);

    for (i = 0; i < 4; i++) { sActors[i].flags = 0; sActors[i].hp = 30; }
    gFrameCounter += 12;
    TryEnemyCardUse(&sActors[0]);
    CHECK(sActors[0].flags & BTLOBJ_FLAG_CARD_ACTION_PENDING, 11);
    CHECK(sBattle.actor4 == 0, 12);
    gFrameCounter += 12;
    sActors[1].x = 400 * 256;
    TryEnemyCardUse(&sActors[1]);
    CHECK(!sActors[1].flags && sBattle.actor4 == 0, 13); /* Range remains authored. */
    sActors[1].x = 0;
    sActors[1].badStatus = BAD_STATUS_STOP;
    RequestEnemyCardUse(&sActors[1]);
    CHECK(!sActors[1].flags, 14);
    sActors[1].badStatus = BAD_STATUS_NONE;
    sActors[1].flags = BTLOBJ_FLAG_DAMAGE_PENDING;
    RequestEnemyCardUse(&sActors[1]);
    CHECK(sActors[1].flags == BTLOBJ_FLAG_DAMAGE_PENDING, 15);
    sActors[1].flags = 0;
    sActors[1].hp = 0;
    RequestEnemyCardUse(&sActors[1]);
    CHECK(!sActors[1].flags, 16);
    sActors[1].hp = 30;
    sBattle.phase = BTL_PHASE_END;
    sBattle.actor4 = &sActors[1];
    RequestEnemyCardUse(&sActors[1]);
    CHECK(!sActors[1].flags && sBattle.actor4 == 0, 17);
    sBattle.phase = BTL_PHASE_IDLE;

    sBattle.flags = BTL_FLAG_BOSS_BATTLE;
    CHECK(!PortActionShadowIndependent(&sActors[1]), 18);
    sBattle.flags = BTL_FLAG_HUM_BATTLE;
    CHECK(!PortActionShadowIndependent(&sActors[1]), 19);
    sBattle.flags = BTL_FLAG_TUTORIAL;
    CHECK(!PortActionShadowIndependent(&sActors[1]), 20);
    sBattle.flags = 0;
    sActors[1].kind = ENEMY_RED_NOCTURNE;
    CHECK(!PortActionShadowIndependent(&sActors[1]), 21);
    RequestEnemyCardUse(&sActors[1]);
    CHECK(sBattle.actor4 == &sActors[1] && !sActors[1].flags, 22);
    sActors[1].kind = ENEMY_SHADOW;
    sEnabled = 0;
    RequestEnemyCardUse(&sActors[1]);
    CHECK(sBattle.actor4 == &sActors[1] && !sActors[1].flags, 23);

    sEnabled = 1;
    sWork.actor.kind = ENEMY_SHADOW;
    sWork.actor.hp = 30;
    sSource = &sActors[0];
    CHECK(UpdateActionShadow(&sWork) == 1, 24);
    CHECK(sSeenSource == &sWork.actor && sSource == &sActors[0], 25);
    sEnabled = 0;
    CHECK(UpdateActionShadow(&sWork) == 1, 26);
    CHECK(sSeenSource == &sActors[0], 27); /* Classic callbacks retain legacy context. */
    sEnabled = 1;
    sWork.actor.flags = BTLOBJ_FLAG_CARD_ACTION_PENDING | BTLOBJ_FLAG_DAMAGE_PENDING;
    UpdateActionShadow(&sWork);
    CHECK(sWork.actor.flags == BTLOBJ_FLAG_DAMAGE_PENDING, 28);
    sWork.actor.flags = BTLOBJ_FLAG_CARD_ACTION_PENDING | BTLOBJ_FLAG_FREEZE_PENDING;
    UpdateActionShadow(&sWork);
    CHECK(sWork.actor.flags == BTLOBJ_FLAG_FREEZE_PENDING, 29);
    sWork.actor.flags = BTLOBJ_FLAG_IN_CARD_ACTION;
    sWork.actor.hp = 30;
    sWork.state = EMY00_STATE_LUNGE;
    sWork.stateTimer = 1;
    sWork.actor.x = 1024;
    UpdateActionShadowThreat(&sWork,1);
    CHECK(sThreatKey==(uintptr_t)&sWork.actor && sThreatX==1024,30);
    CHECK(sThreatRemaining==30 && sThreatTotal==31,31);
    sWork.stateTimer = 30; UpdateActionShadowThreat(&sWork,1);
    CHECK(sThreatRemaining==1,32);
    sWork.stateTimer = 31; UpdateActionShadowThreat(&sWork,1);
    CHECK(!sThreatRemaining,33); /* Active hits have no stale warning. */
    sWork.state = EMY00_STATE_JUMP_LUNGE; sWork.stateTimer=1;
    UpdateActionShadowThreat(&sWork,1);
    CHECK(sThreatRemaining==13 && sThreatTotal==14,34);
    sWork.stateTimer=14; UpdateActionShadowThreat(&sWork,1);
    CHECK(!sThreatRemaining,35);
    sWork.stateTimer=1; sWork.actor.flags|=BTLOBJ_FLAG_DAMAGE_PENDING;
    UpdateActionShadowThreat(&sWork,1); CHECK(!sThreatRemaining,36);
    sWork.actor.flags=BTLOBJ_FLAG_IN_CARD_ACTION; sWork.actor.badStatus=BAD_STATUS_STOP;
    UpdateActionShadowThreat(&sWork,1); CHECK(!sThreatRemaining,37);
    sWork.actor.badStatus=BAD_STATUS_NONE; sWork.actor.hp=0;
    UpdateActionShadowThreat(&sWork,1); CHECK(!sThreatRemaining,38);
    sWork.actor.hp=30; UpdateActionShadowThreat(&sWork,0); CHECK(!sThreatRemaining,39);
    sWork.state=EMY00_STATE_IDLE; UpdateActionShadowThreat(&sWork,1);
    CHECK(!sThreatRemaining,40); /* Pending AI requests do not warn. */
    sWork.state=EMY00_STATE_LUNGE; sWork.actor.flags=BTLOBJ_FLAG_CARD_ACTION_PENDING;
    UpdateActionShadowThreat(&sWork,1); CHECK(!sThreatRemaining,41);
    return 0;
}
void _start(void) {
    int code = run_tests();
    __asm__ volatile ("int $0x80" : : "a"(1), "b"(code));
    __builtin_unreachable();
}
'''


def main():
    runtime = (ROOT / "src/btl/battle_runtime.c").read_text()
    shadow = (ROOT / "src/btl/emy_00.c").read_text()
    functions = "\n".join(function(runtime, name) for name in (
        "PortActionShadowIndependent", "RequestActionShadowAttack", "RequestEnemyCardUse", "TryEnemyCardUse"))
    functions += "\n" + function(shadow, "UpdateActionShadowThreat")
    functions += "\n" + function(shadow, "UpdateActionShadow")
    enum_start = shadow.index('enum Emy00State')
    states = shadow[enum_start:shadow.index('};',enum_start)+2]
    includes = [ROOT / "include", ROOT / "port/include"]
    includes += sorted(p for p in (ROOT / "include").iterdir() if p.is_dir() and p.name != "gba")
    with tempfile.TemporaryDirectory(prefix="rekhcom-shadows-") as folder:
        folder = Path(folder)
        source, executable = folder / "runner.c", folder / "runner"
        source.write_text(HARNESS.replace("__FUNCTIONS__", functions).replace("__STATES__", states))
        command = [os.environ.get("CC", "gcc"), "-m32", "-O2", "-std=gnu89",
                   "-ffreestanding", "-fno-builtin", "-fno-stack-protector", "-fno-pie",
                   "-DPLATFORM_PC", "-DVERSION_US", "-nostdlib", "-static", "-no-pie"]
        command += ["-I" + str(p) for p in includes]
        subprocess.run(command + [str(ROOT / "src/listpool.c"), str(source), "-o", str(executable)],
                       cwd=ROOT, check=True)
        try:
            status = subprocess.run([str(executable)], check=False).returncode
        except OSError as error:
            if error.errno != errno.ENOEXEC: raise
            status = run_elf32(executable)
        if status: raise SystemExit("Shadow dispatch regression failed at check " + str(status))
    print("PASS: 41 real Shadow dispatch, threat windup/cancel, concurrency and ownership checks")


if __name__ == "__main__":
    main()
