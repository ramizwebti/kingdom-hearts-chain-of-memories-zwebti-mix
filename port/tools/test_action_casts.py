#!/usr/bin/env python3
"""Exercise production spell cases with real actors and authored animation timing.

Only effect/audio/render sinks are stubbed. The case bodies, action pose helper,
state reset and animation progression are extracted unchanged from game code.
An active effect deliberately outlives the action to verify early recovery.
"""
import ast
import errno
import os
from pathlib import Path
import re
import subprocess
import tempfile

from test_action_collision import run_elf32

ROOT = Path(__file__).resolve().parents[2]
SPELLS = ('CURE', 'STOP', 'AERO', 'THUNDER', 'GRAVITY', 'FIRE', 'BLIZZARD')
NEXT = ('STOP', 'ITEM', 'HOLY', 'GRAVITY', 'HOMING_BLIZZARA', 'MEGA_FLARE', 'SONIC_BLADE')

HARNESS = r'''
#include "btl.h"
#include "battle_work.h"
#include "battle_actor_types.h"
#include "battle_ids.h"
#include "btl_effect.h"
#include "card_ids.h"
#include "card_label_data.h"
#include "fade.h"
#include "songs.h"
#include "anim.h"
#include <stddef.h>
static BtlWork sBattle;
BtlWork* gBtlWork = &sBattle;
static BtlSoraWork sSora;
static int sActionIndependent, sActiveEffect, sEffects, sEffectTick;
static int sReleasePose, sTick, sFades, sPops, sRecoverTick;
__STATES__
#define FocusBtlSoraCameraOnTarget(...) ((void)0)
#define FocusBtlSoraCameraOnBgFx(...) ((void)0)
#define FocusBtlSoraCamera(...) ((void)0)
#define m4aSongNumStart(...) ((void)0)
#define FadeStartOut(...) (++sFades)
#define FadeStartIn(...) (--sFades)
#define FadeToAmount(...) (++sFades)
#define FadeToOriginal(...) (--sFades)
#define SetBattleZoom(...) ((void)0)
#define CreateBtlPopTask(...) (++sPops)
#define ClearBtlObjActionFlags(actor) ((actor)->flags = 0)
#define TraceActionCast(work,phase) ((void)0)
u8 BgFxIsActive(void) { return sActiveEffect; }
static void effect(void) {
    ++sEffects; sActiveEffect = 1; sEffectTick = sTick;
    sReleasePose = sSora.anim.frame;
}
#define BgFxStartCure(...) effect()
#define BgFxStartStop(...) effect()
#define BgFxStartAero(...) effect()
#define BgFxStartThunder(...) effect()
#define BgFxStartWideThunder(...) effect()
#define BgFxStartGravity(...) effect()
#define BgFxStartFire(...) effect()
#define BgFxStartBlizzard(...) effect()
__FRAMES__
static void SetBtlSoraAnimation(BtlSoraWork* work, u16 index, u16 flags) {
    AnimState* anim = &work->anim;
    anim->frame = anim->timer = 0;
    anim->flags = flags;
    anim->animId = index;
    anim->frames = frames42;
    anim->frameCount = sizeof(frames42) / sizeof(frames42[0]);
    if (index >= 43 && index <= 45) {
        anim->frames = frames43;
        anim->frameCount = sizeof(frames43) / sizeof(frames43[0]);
    }
    if (index == 1 || index == 46) {
        anim->frames = framesIdle;
        anim->frameCount = 1;
    }
}
static void SetBtlSoraDirAnimation(BtlSoraWork* work, u16 index, u16 flags) {
    SetBtlSoraAnimation(work, 1, flags);
}
void* AnimGetGfx(AnimState* anim) { (void)anim; return NULL; }
__FUNCTIONS__
static void step(BtlSoraWork* work, int actionMotion) {
    BtlObj* act = &work->actor;
    BtlObj* enemy;
    s16 timer;
    s32 attack, hit, targetZ;
    switch (work->state) {
__CASES__
    }
    if (work->state == BTL_SORA_STATE_IDLE || work->state == BTL_SORA_STATE_RECOVER ||
        work->state == BTL_SORA_STATE_AIRBORNE) {
        if (sRecoverTick < 0) sRecoverTick = sTick;
    }
    AnimUpdate(&work->anim);
}
static void reset(u16 state, int independent) {
    sSora.actor.btl = &sBattle;
    sSora.actor.x = sSora.actor.y = sSora.actor.z = sSora.actor.groundZ = 0;
    sSora.actor.flags = 0;
    sBattle.flags = 0;
    sBattle.actor2 = NULL;
    sBattle.hcEffect = 0;
    sSora.actor.hp = 30; sSora.actor.maxHp = 500;
    sSora.variant[0] = SPELL_TIER_BASE;
    sActionIndependent = independent;
    sActiveEffect = sEffects = sPops = sFades = 0;
    sEffectTick = sReleasePose = sRecoverTick = -1;
    SetBtlSoraState(&sSora, state);
    SetBtlSoraAnimation(&sSora, 1, 1);
}
#define CHECK(test, code) do { if (!(test)) return (code); } while (0)
static int run_tests(void) {
    static const u16 spells[] = {__SPELL_IDS__};
    static const s16 boosts[] = {0, HC_EFFECT_MAGIC_BOOST, HC_EFFECT_CURE_BOOST};
    static const s16 heals[3][3] = {{50,150,300},{65,195,390},{75,225,450}};
    unsigned i, variant, boost;
    int nativeTick;
    for (i=0; i<sizeof(spells)/sizeof(spells[0]); ++i) {
        /* Action effects still run after the caster is free at tick20. */
        reset(spells[i], 1);
        for (sTick=0; sTick<21; ++sTick) step(&sSora, 1);
        CHECK(sEffects == 1 && sEffectTick == 14, 1+i);
        CHECK(sRecoverTick == 20 && sSora.state == BTL_SORA_STATE_IDLE, 11+i);
        CHECK(sActiveEffect && !sFades, 21+i);
        CHECK(sReleasePose == ((i <= 3) ? 4 : 2), 31+i);
        CHECK(sPops == (i == 0 ? 1 : 0), 41+i);
        /* Merely enabling the mode cannot accelerate an original card cast. */
        reset(spells[i], 0);
        nativeTick = spells[i] == BTL_SORA_STATE_STOP ? 25 : 27;
        for (sTick=0; sTick<50; ++sTick) step(&sSora, 1);
        CHECK(sEffects == 1 && sEffectTick == nativeTick && sRecoverTick < 0, 51+i);
        sActiveEffect = 0; step(&sSora, 1);
        CHECK(sRecoverTick == 50 && !sFades, 61+i);
        /* Classic mode is unchanged even if an old independent flag survives. */
        reset(spells[i], 1);
        for (sTick=0; sTick<50; ++sTick) step(&sSora, 0);
        CHECK(sEffects == 1 && sEffectTick == nativeTick && sRecoverTick < 0, 71+i);
        sActiveEffect = 0; step(&sSora, 0);
        CHECK(sRecoverTick == 50 && !sFades, 81+i);
        /* Interruption before release cannot launch an effect or heal. */
        reset(spells[i], 1);
        for (sTick=0; sTick<14; ++sTick) step(&sSora, 1);
        SetBtlSoraState(&sSora, BTL_SORA_STATE_IDLE);
        for (;sTick<40; ++sTick) step(&sSora, 1);
        CHECK(!sEffects && !sPops && sSora.actor.hp == 30 && !sFades, 91+i);
        /* Air casting releases immediately on its own clock, keeps altitude,
         * and returns control to the normal airborne state. */
        reset(spells[i], 1);
        sSora.actor.z = -32*256;
        sBattle.flags |= BTL_FLAG_PLAYER_AIRBORNE;
        for (sTick=0; sTick<21; ++sTick) step(&sSora, 1);
        CHECK(sEffects == 1 && sEffectTick == 14 && sRecoverTick == 20, 111+i);
        CHECK(sSora.state == BTL_SORA_STATE_AIRBORNE && sSora.actor.z == -32*256, 121+i);
        /* A classic airborne spell retains the original wait for landing. */
        reset(spells[i], 0);
        sSora.actor.z = -32*256;
        sBattle.flags |= BTL_FLAG_PLAYER_AIRBORNE;
        for (sTick=0; sTick<40; ++sTick) step(&sSora, 1);
        CHECK(!sEffects && !sSora.stateTimer, 131+i);
    }
    /* The actual Cure case retains tier/boost amounts and heals only once. */
    for (boost=0; boost<3; ++boost) for (variant=0; variant<3; ++variant) {
        reset(BTL_SORA_STATE_CURE, 1);
        sBattle.hcEffect = boosts[boost]; sSora.variant[0] = variant;
        for (sTick=0; sTick<14; ++sTick) step(&sSora, 1);
        CHECK(sSora.actor.hp == 30 && !sPops, 101);
        step(&sSora, 1); ++sTick;
        CHECK(sSora.actor.hp == 30+heals[boost][variant] && sPops == 1, 102);
        for (;sTick<60; ++sTick) step(&sSora, 1);
        CHECK(sSora.actor.hp == 30+heals[boost][variant] && sPops == 1, 103);
    }
    reset(BTL_SORA_STATE_CURE, 1); sSora.actor.hp = 480;
    for (sTick=0; sTick<60; ++sTick) step(&sSora, 1);
    CHECK(sSora.actor.hp == 500 && sPops == 1, 104);
    /* The overhead release preserves every charge pose before its release. */
    reset(BTL_SORA_STATE_CURE, 1);
    for (sTick=0; sTick<=14; ++sTick) {
        step(&sSora, 1);
        if (sTick==0) CHECK(sSora.anim.frame==0, 105);
        if (sTick==4) CHECK(sSora.anim.frame==1, 106);
        if (sTick==7) CHECK(sSora.anim.frame==2, 107);
        if (sTick==10) CHECK(sSora.anim.frame==3, 108);
        if (sTick==14) CHECK(sSora.anim.frame==4, 109);
    }
    return 0;
}
void _start(void) {
    int code = run_tests();
    __asm__ volatile ("int $0x80" : : "a"(1), "b"(code));
    __builtin_unreachable();
}
'''


def frames(source, name):
    match = re.search(r'symbol: '+name+r'\n[^\n]*\n\s*frames: (\[.*\])', source)
    return ast.literal_eval(match.group(1))


def function(source, name):
    match = re.search(r'(?:static )?(?:void\*|void|u8) '+name+r'\([^;{}]*\)\s*\{', source)
    if not match:
        raise ValueError(name)
    cursor, depth = match.end(), 1
    while depth:
        depth += (source[cursor] == '{') - (source[cursor] == '}')
        cursor += 1
    return source[match.start():cursor]


def main():
    source = (ROOT/'src/btl/btl_sora.c').read_text()
    engine = (ROOT/'src/engine.c').read_text()
    manifest = (ROOT/'config/assets/sprites_fld.yaml').read_text()
    start = source.rfind('enum', 0, source.index('BTL_SORA_STATE_ENTER,'))
    states = source[start:source.index('};', start)+2]
    cases = []
    for spell, after in zip(SPELLS, NEXT):
        begin = source.index('    case BTL_SORA_STATE_'+spell+':')
        end = source.index('    case BTL_SORA_STATE_'+after+':', begin)
        cases.append(source[begin:end])
    helpers = '\n'.join((function(source, 'SetBtlSoraState'),
                         function(source, 'ActionAdvanceCastPose'),
                         function(engine, 'AnimUpdate'),
                         function(engine, 'AnimIsFinished'),
                         function(engine, 'AnimSetFrame')))
    frame_data = []
    for index, name in ((42, 'gSor1ll08Anim0'), (43, 'gSor1ll09Anim0')):
        values = ','.join('{%d,%d}' % tuple(frame) for frame in frames(manifest, name))
        frame_data.append('static AnimFrame frames%d[] = {%s};' % (index, values))
    frame_data.append('static AnimFrame framesIdle[] = {{0,1000}};')
    harness = HARNESS.replace('__STATES__', states).replace('__CASES__', '\n'.join(cases))
    harness = harness.replace('__FRAMES__', '\n'.join(frame_data)).replace('__FUNCTIONS__', helpers)
    harness = harness.replace('__SPELL_IDS__', ','.join('BTL_SORA_STATE_'+x for x in SPELLS))
    includes = [ROOT/'include', ROOT/'port/include']
    includes += sorted(p for p in (ROOT/'include').iterdir() if p.is_dir() and p.name != 'gba')
    with tempfile.TemporaryDirectory(prefix='rekhcom-casts-') as temporary:
        folder = Path(temporary)
        runner, exe = folder/'casts.c', folder/'casts'
        runner.write_text(harness)
        cmd = [os.environ.get('CC', 'gcc'), '-m32', '-O2', '-std=gnu89', '-ffreestanding',
               '-fno-builtin', '-fno-stack-protector', '-fno-pie', '-ffunction-sections',
               '-fdata-sections', '-DPLATFORM_PC', '-DVERSION_US', '-nostdlib', '-static',
               '-no-pie', '-Wl,--gc-sections']
        cmd += ['-I'+str(p) for p in includes]
        subprocess.run(cmd+[str(runner), '-o', str(exe)], check=True)
        try:
            status = subprocess.run([str(exe)], check=False).returncode
        except OSError as error:
            if error.errno != errno.ENOEXEC:
                raise
            status = run_elf32(exe)
        if status:
            raise SystemExit('FAIL production cast timeline check '+str(status))
    print('PASS: seven production spell timelines, native-mode guards, authored release poses, active-effect recovery, interruption and nine Cure tier/boost combinations')


if __name__ == '__main__':
    main()
