/**
 * frd_beast.c
 * Beast Battle Ally
 */

#include "task_descriptors.h"
#include "system_state.h"
#include "frd.h"
#include "sprites_frd.h"
#include "world_types.h"
#include "btl_api.h"
#include "songs.h"
#include "frd_tasks.h"
#include "anim.h"
#include "battle_actor.h"
#include "battle_actor_types.h"
#include "battle_work.h"
#include "btl_collision.h"
#include "game_state.h"
#include "m4a_song.h"
#include "obj.h"
#include "obj_api.h"
#include "taskpool.h"
#include "types.h"
#include <stddef.h>
#include "sprite_palettes.h"
#ifdef PLATFORM_PC
#include "port/port_action_assist.h"
#endif
#include "battle_ids.h"
#include "engine_math.h"
#include "btl.h"
#ifdef PLATFORM_PC
#include "port/port_action_game.h"
#endif

static const AnimDef sFrdBeastAnimDefs[2] = {
    { gFelosiaslangeFrames, gFelosiaslangeAnims, gFelosiaslangeTiles, 0 },
    { gFelosiaslangeFrames, gFelosiaslangeAnims, gFelosiaslangeTiles, 1 },
};

u8 FrdBeastApplyGravity(FrdBeastWork* work) {
    BtlObj* body;

    body = &work->body;
    ApplyBattleBounds(&body->x, &body->y, &body->z, &body->groundZ);
    body->z += work->vz;
    work->vz += 0x33;

    if (body->z > body->groundZ) {
        body->z = body->groundZ;
        work->vz = 0;
        return TRUE;
    }

    return FALSE;
}

enum FrdBeastState {
    FRD_BEAST_STATE_CHARGE = 1,
    FRD_BEAST_STATE_POUNCE
};

void task_frd_beast_0(FrdBeastWork* work, FrdArgs* args) {
    BtlObj* body;
    BtlObj* target;

    body = &work->body;

#ifdef VERSION_EU
    if (gLanguage == LANGUAGE_ENGLISH) {
        m4aSongNumStart(SONG_VO_SR_SUMMON09);
    } else {
        m4aSongNumStart(SONG_VO_SR_SUMMON00);
    }
#else
    m4aSongNumStart(SONG_VO_SR_SUMMON09);
#endif

    if (args->mainSide) {
        work->mainSide = TRUE;
        gBtlWork->flags |= BTL_FLAG_SUMMON_ACTIVE;
        work->actor = gBtlWork->actor;
        work->tiles = gBtlWork->tiles2;
        target = gBtlWork->actor2;
    } else {
        work->mainSide = args->mainSide;
        gRikuBtlWork->flags |= BTL_FLAG_SUMMON_ACTIVE;
        work->actor = gRikuBtlWork->actor;
        work->tiles = gBtlWork->tiles2;
        target = gRikuBtlWork->actor2;
    }

    work->variant = args->variant;
    work->stateTimer = 0;
    work->vz = 0;

    if (target != NULL) {
        work->targetX = target->x;
        work->targetY = target->y;
    } else {
        work->targetX = 0x10000;
        work->targetY = work->actor->y;
    }

    if (work->actor->flags & BTLOBJ_FLAG_FACING_LEFT) {
        body->x = (gBtlWork->xMax + 0x30) << 8;
        body->flags = (BTLOBJ_FLAG_FACING_LEFT | BTLOBJ_FLAG_LARGE_SHADOW);
    } else {
        body->x = (gBtlWork->xMin - 0x30) << 8;
        body->flags = BTLOBJ_FLAG_LARGE_SHADOW;
    }

    body->y = work->targetY;
    body->z = 0;
    body->groundZ = 0;

    switch (work->variant) {
    case SUMMON_LEVEL_SINGLE:
        work->state = FRD_BEAST_STATE_CHARGE;
        work->attack = 0xA0;
        break;
    case SUMMON_LEVEL_PAIR:
        work->state = FRD_BEAST_STATE_CHARGE;
        work->attack = 0xA1;
        break;
    case SUMMON_LEVEL_TRIPLE:
    default:
        work->state = FRD_BEAST_STATE_POUNCE;
        work->attack = 0xA1;
        break;
    }

#ifdef PLATFORM_PC
    work->tiles = PortActionAssistSpawnTiles(args->mainSide, work->tiles);
#endif
    work->palette = LoadObjPalette(gBeastPalette, sizeof(gBeastPalette));
    AnimInit(&work->anim, NULL, NULL);
    AnimChangeWithDef(sFrdBeastAnimDefs, &work->anim, 0, 0, work->tiles);
    TaskPoolInit(&work->tasks, 1);
    TaskCreate(&work->tasks, &gTaskDescBtlShadow, body);
}

u8 task_frd_beast_1(FrdBeastWork* work) {
    BtlObj* body;
    BtlWork* owner;

    body = &work->body;

    if (gGameState.world != WORLD_HOLLOW_BASTION
#ifdef PLATFORM_PC
        && !(work->mainSide && PortActionCombatRules())
#endif
    ) {
        return 0;
    }

    owner = work->mainSide ? gBtlWork : gRikuBtlWork;

    if (owner->flags & BTL_FLAG_DISMISS_SUMMONS) {
        return 0;
    }

    switch (work->state) {
    case FRD_BEAST_STATE_POUNCE:
        if (work->stateTimer == 0) {
            m4aSongNumStart(SONG_VO_BE_ATTACK00);
        }

        if (work->anim.timer == 0 && AnimGetFrame(&work->anim) == 2) {
            work->vz = -0x400;
            m4aSongNumStart(SONG_BTL_BE_ATT02);
        }

        if (body->z < body->groundZ) {
            body->x += (work->targetX - body->x) >> 4;
            body->y += (work->targetY - body->y) >> 4;
        }

        if (work->vz > 0) {
            if (gBtlWork->battleId == BATTLE_DRAGON_MALEFICENT) {
                ApplyAttackBox(0xA3, body->x, body->y, body->z - 0x1800, 0x28, 0x14, 0x10);
            } else {
                ApplyAttackBox(0xA2, body->x, body->y, body->z - 0x1800, 0x28, 0x14, 0x10);
            }
        }

        if (FrdBeastApplyGravity(work) && AnimIsFinished(&work->anim)) {
            work->state = FRD_BEAST_STATE_CHARGE;
            work->attack = 0xA1;
            work->stateTimer = 0;
            BtlMapStartShake();
            break;
        }

        work->stateTimer++;
        break;
    case FRD_BEAST_STATE_CHARGE:
        if (work->stateTimer == 0) {
            AnimChangeWithDef(sFrdBeastAnimDefs, &work->anim, 1, ANIM_FLAG_LOOP, work->tiles);

            if (work->variant != SUMMON_LEVEL_TRIPLE) {
                m4aSongNumStart(SONG_VO_BE_ATTACK00);
            }
        }

        if (body->flags & BTLOBJ_FLAG_FACING_LEFT) {
            body->x -= 0x380;

            if (body->x < (gBtlWork->xMin - 0x28) << 8) {
                return 0;
            }
        } else {
            body->x += 0x380;

            if (body->x > (gBtlWork->xMax + 0x28) << 8) {
                return 0;
            }
        }

        if (ApplyAttackBox(work->attack, body->x, body->y, body->z - 0x1800, 0x28, 0x14, 0x10)) {
            m4aSongNumStart(SONG_BTL_BE_ATT01);
        }

        FrdBeastApplyGravity(work);
        work->stateTimer++;
        break;
    }

    AnimUpdate(&work->anim);
    TaskPoolUpdate(&work->tasks);
    return 1;
}

void task_frd_beast_2(FrdBeastWork* work) {
    BtlObj* body;
    void* gfx;
    u16 flags;
    s16 sx;
    s16 sy;
    ObjAffine* affine;
    s32 sclX;
    s32 sclY;

    body = &work->body;
    gfx = AnimGetGfx(&work->anim);
    flags = GetBattleSpritePriorityFlags(body->y);

    if (body->flags & BTLOBJ_FLAG_FACING_LEFT) {
        sclY = gBtlWork->scale;
        sclX = sclY;
    } else if (gBtlWork->scale == Q_8_8(1)) {
        sclY = gBtlWork->scale;
        sclX = sclY;
        flags |= SPRITE_FLAG_HFLIP;
    } else {
        sclX = -gBtlWork->scale;
        sclY = gBtlWork->scale;
    }

    WorldToScreen(&sx, &sy, body->x, body->y, body->z);

    if (gBtlWork->scale == Q_8_8(1)) {
        affine = NULL;
    } else if (gBtlWork->scale <= 255) {
        affine = AllocObjAffine(0, sclX, sclY, FALSE);
    } else {
        affine = AllocObjAffine(0, sclX, sclY, TRUE);
    }

    DrawSprite(sx, sy, gfx, work->tiles, work->palette, affine, flags,
               -4100 - ((body->y >> 8) * 4));
    body->shadowPriority = (-4100 - ((body->y >> 8) * 4)) | 2;
    TaskPoolDraw(&work->tasks);
}

void task_frd_beast_3(FrdBeastWork* work) {
    BtlWork* owner;

    owner = work->mainSide ? gBtlWork : gRikuBtlWork;
    owner->flags &= ~BTL_FLAG_SUMMON_ACTIVE;
    ReleaseObjPalette(work->palette);
    TaskPoolDestroy(&work->tasks);
}

TaskDesc gTaskDescFrdBeast = {
    "task_frd_beast",
    (TaskInitFunc)task_frd_beast_0,
    (TaskUpdateFunc)task_frd_beast_1,
    (TaskDrawFunc)task_frd_beast_2,
    (TaskDestroyFunc)task_frd_beast_3,
    sizeof(FrdBeastWork),
};
