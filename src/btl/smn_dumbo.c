/**
 * smn_dumbo.c
 * Dumbo Summon
 */

#include "task_descriptors.h"
#include "smn.h"
#include "anim.h"
#include "sprites_smn.h"
#include "btl_api.h"
#include "songs.h"
#include "smn_tasks.h"
#include "battle_actor.h"
#include "battle_actor_types.h"
#include "battle_work.h"
#include "btl_effect.h"
#include "engine_math.h"
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

static const AnimDef sSmnDumboAnimDefs[3] = {
    { gSmnDumboFrames, gSmnDumboAnims, gSmnDumboTiles, 0 },
    { gSmnDumboFrames, gSmnDumboAnims, gSmnDumboTiles, 1 },
    { gSmnDumboFrames, gSmnDumboAnims, gSmnDumboTiles, 2 },
};

TaskDesc gTaskDescSmnDumbo = {
    "task_smn_dumbo",
    (TaskInitFunc)task_smn_dumbo_0,
    (TaskUpdateFunc)task_smn_dumbo_1,
    (TaskDrawFunc)task_smn_dumbo_2,
    (TaskDestroyFunc)task_smn_dumbo_3,
    sizeof(SmnDumboWork),
};

enum SmnDumboState {
    SMN_DUMBO_STATE_APPEAR,
    SMN_DUMBO_STATE_VANISH,
    SMN_DUMBO_STATE_SPLASH_WINDUP,
    SMN_DUMBO_STATE_SPLASH,
    SMN_DUMBO_STATE_SPLASH_END
};

void task_smn_dumbo_0(SmnDumboWork* work, SmnArgs* args) {
    BtlObj* body;
    BtlObj* actor;
    s32 spawnX, spawnY, spawnZ;

    body = &work->body;

    if (args->mainSide) {
        work->mainSide = TRUE;
        gBtlWork->flags |= BTL_FLAG_SUMMON_ACTIVE;
        actor = gBtlWork->actor;
        work->tiles = gBtlWork->tiles;
    } else {
        work->mainSide = args->mainSide;
        gRikuBtlWork->flags |= BTL_FLAG_SUMMON_ACTIVE;
        actor = gRikuBtlWork->actor;
        work->tiles = gRikuBtlWork->tiles;
    }

    spawnX=actor->originX;spawnY=actor->originY;spawnZ=actor->originZ;
#ifdef PLATFORM_PC
    PortActionAssistSpawnOrigin(args->mainSide,&spawnX,&spawnY,&spawnZ);
#endif
    body->x = spawnX;
    body->y = spawnY;
    body->z = spawnZ;
    body->groundZ = spawnZ;

    if (actor->flags & BTLOBJ_FLAG_FACING_LEFT) {
        body->flags = (BTLOBJ_FLAG_FACING_LEFT | BTLOBJ_FLAG_LARGE_SHADOW);
    } else {
        body->flags = BTLOBJ_FLAG_LARGE_SHADOW;
    }

    work->variant = args->variant;
#ifdef PLATFORM_PC
    work->tiles = PortActionAssistSpawnTiles(args->mainSide, work->tiles);
#endif
    work->palette = LoadObjPalette(gDamboPalette, sizeof(gDamboPalette));
    AnimInit(&work->anim, NULL, NULL);
    AnimChangeWithDef(sSmnDumboAnimDefs, &work->anim, 0, 0, work->tiles);
    work->state = SMN_DUMBO_STATE_APPEAR;
    work->stateTimer = 0;
    work->steps = 0;
    work->unk_14C = 0;
    work->scale = 10;
    work->animating = FALSE;
    TaskPoolInit(&work->tasks, 1);
    TaskCreate(&work->tasks, &gTaskDescBtlShadow, body);
}

u8 task_smn_dumbo_1(SmnDumboWork* work) {
    BtlObj* body;
    BtlWork* owner;

    body = &work->body;
    owner = work->mainSide ? gBtlWork : gRikuBtlWork;

    if (owner->flags & BTL_FLAG_DISMISS_SUMMONS) {
        return 0;
    }

    BtlMapFollowPosition(body->x, body->y, body->z);

    switch (work->state) {
    case SMN_DUMBO_STATE_APPEAR:
        if (work->stateTimer == 0) {
            work->steps = 30;
            BgFxStartSummon(body->x, body->y, body->z);
            m4aSongNumStart(SONG_EF_SUMMON_UP);
        }

        ApproachValue(&work->scale, Q_8_8(1), work->steps);

        if (work->steps > 0) {
            STEP_STATE(work);
        } else {
            work->state = SMN_DUMBO_STATE_SPLASH_WINDUP;
            work->stateTimer = 0;
            work->animating = TRUE;
        }

        break;
    case SMN_DUMBO_STATE_VANISH:
        if (work->stateTimer == 0) {
            work->steps = 30;
            BgFxStartSummon(body->x, body->y, body->z);
            m4aSongNumStart(SONG_EF_SUMMON_DOWN);
        }

        ApproachValue(&work->scale, Q_8_8(0.1), work->steps);

        if (work->steps <= 0) {
            return 0;
        }

        STEP_STATE(work);
        break;
    case SMN_DUMBO_STATE_SPLASH_WINDUP:
        if (work->stateTimer == 0) {
            AnimChangeWithDef(sSmnDumboAnimDefs, &work->anim, 0, 0, work->tiles);
        }

        if (AnimIsFinished(&work->anim)) {
            work->state = SMN_DUMBO_STATE_SPLASH;
            work->stateTimer = 0;
        } else {
            work->stateTimer++;
        }

        break;
    case SMN_DUMBO_STATE_SPLASH:
        if (work->stateTimer == 0) {
            AnimChangeWithDef(sSmnDumboAnimDefs, &work->anim, 1, ANIM_FLAG_LOOP, work->tiles);

            if (body->flags & BTLOBJ_FLAG_FACING_LEFT) {
                BgFxStartDumboSplash(work->variant, body->x - 0x1C00, body->y,
                              body->z - 0x1B00, FALSE, 0x9C);
            } else {
                BgFxStartDumboSplash(work->variant, body->x + 0x1C00, body->y,
                              body->z - 0x1B00, TRUE, 0x9C);
            }

            m4aSongNumStart(SONG_EF_DAMBO_SPLOOP);
        } else if (!BgFxIsActive()) {
            m4aSongNumStop(SONG_EF_DAMBO_SPLOOP);
            work->state = SMN_DUMBO_STATE_SPLASH_END;
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
    case SMN_DUMBO_STATE_SPLASH_END:
        if (work->stateTimer == 0) {
            AnimChangeWithDef(sSmnDumboAnimDefs, &work->anim, 2, 0, work->tiles);
        }

        if (AnimIsFinished(&work->anim)) {
            work->state = SMN_DUMBO_STATE_VANISH;
            work->stateTimer = 0;
        } else {
            work->stateTimer++;
        }

        break;
    }

    if (work->animating) {
        AnimUpdate(&work->anim);
    }

    ApplyBattleBounds(&body->x, &body->y, &body->z, &body->groundZ);

    if (body->z > body->groundZ) {
        body->z = body->groundZ;
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

void task_smn_dumbo_2(SmnDumboWork* work) {
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
    } else if (gBtlWork->scale == Q_8_8(1) && work->scale == gBtlWork->scale) {
        sclY = work->scale;
        sclX = sclY;
        flags |= SPRITE_FLAG_HFLIP;
    } else {
        sclX = -gBtlWork->scale;
        sclY = gBtlWork->scale;
    }

    sclX = sclX * work->scale >> 8;
    sclY = sclY * work->scale >> 8;

    WorldToScreen(&sx, &sy, body->x, body->y, body->z);

    if (sclX <= Q_8_8(1) && sclY <= Q_8_8(1)) {
        affine = AllocObjAffine(0, sclX, sclY, FALSE);
    } else {
        affine = AllocObjAffine(0, sclX, sclY, TRUE);
    }

    DrawSprite(sx, sy, gfx, work->tiles, work->palette, affine, flags,
               -4100 - ((body->y >> 8) * 4));
    body->shadowPriority = (-4100 - ((body->y >> 8) * 4)) | 2;
    TaskPoolDraw(&work->tasks);
}

void task_smn_dumbo_3(SmnDumboWork* work) {
    BtlWork* owner;

    owner = work->mainSide ? gBtlWork : gRikuBtlWork;
    owner->flags &= ~BTL_FLAG_SUMMON_ACTIVE;
    m4aSongNumStop(SONG_EF_DAMBO_SPLOOP);
    ReleaseObjPalette(work->palette);
    TaskPoolDestroy(&work->tasks);
}
