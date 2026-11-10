/**
 * frd_aladdin.c
 * Aladdin Battle Ally
 */

#include "task_descriptors.h"
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
#include "engine_math.h"
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
#include "btl.h"
#ifdef PLATFORM_PC
#include "port/port_action_game.h"
#endif

static const AnimDef sFrdAladdinAnimDefs[3] = {
    { gAladdin10Frames, gAladdin10Anims, gAladdin10Tiles, 2 },
    { gAladdin10Frames, gAladdin10Anims, gAladdin10Tiles, 0 },
    { gAladdin10Frames, gAladdin10Anims, gAladdin10Tiles, 1 },
};

TaskDesc gTaskDescFrdAladdin = {
    "task_frd_aladdin",
    (TaskInitFunc)task_frd_aladdin_0,
    (TaskUpdateFunc)task_frd_aladdin_1,
    (TaskDrawFunc)task_frd_aladdin_2,
    (TaskDestroyFunc)task_frd_aladdin_3,
    sizeof(FrdAladdinWork),
};

u8 FrdAladdinApplyGravity(FrdAladdinWork* work) {
    BtlObj* body;

    body = &work->body;

    if (ApplyBattleBounds(&body->x, &body->y, &body->z, &body->groundZ)) {
        body->z += work->vz;
        work->vz = -0x200;
    } else {
        body->z += work->vz;
        work->vz += 0x33;
    }

    if (body->z > body->groundZ) {
        body->z = body->groundZ;
        work->vz = 0;
        return TRUE;
    }

    return FALSE;
}

enum FrdAladdinState {
    FRD_ALADDIN_STATE_ENTER,
    FRD_ALADDIN_STATE_LAND,
    FRD_ALADDIN_STATE_LEAVE,
    FRD_ALADDIN_STATE_ATTACK
};

void task_frd_aladdin_0(FrdAladdinWork* work, FrdArgs* args) {
    BtlObj* body;

    body = &work->body;
    m4aSongNumStart(SONG_VO_SR_SUMMON11);

    if (args->mainSide) {
        work->mainSide = TRUE;
        gBtlWork->flags |= BTL_FLAG_SUMMON_ACTIVE;
        work->actor = gBtlWork->actor;
        work->tiles = gBtlWork->tiles2;
    } else {
        work->mainSide = args->mainSide;
        gRikuBtlWork->flags |= BTL_FLAG_SUMMON_ACTIVE;
        work->actor = gRikuBtlWork->actor;
        work->tiles = gBtlWork->tiles2;
    }

    work->variant = args->variant;
    work->state = FRD_ALADDIN_STATE_ENTER;
    work->stateTimer = 0;
    work->steps = 0;
    work->vz = 0;

    if (work->actor->flags & BTLOBJ_FLAG_FACING_LEFT) {
        work->targetX = work->actor->x - 0x3000;
        body->x = (gBtlWork->xMax + 0x30) << 8;
        body->flags = BTLOBJ_FLAG_FACING_LEFT;
    } else {
        work->targetX = work->actor->x + 0x3000;
        body->x = (gBtlWork->xMin - 0x30) << 8;
        body->flags = 0;
    }

    body->y = work->actor->y;
    body->z = -0x5000;
    body->groundZ = 0;
#ifdef PLATFORM_PC
    work->tiles = PortActionAssistSpawnTiles(args->mainSide, work->tiles);
#endif
    work->palette = LoadObjPalette(gAladdinPalette, sizeof(gAladdinPalette));
    AnimInit(&work->anim, NULL, NULL);
    AnimChangeWithDef(sFrdAladdinAnimDefs, &work->anim, 0, 0, work->tiles);
    TaskPoolInit(&work->tasks, 1);
    TaskCreate(&work->tasks, &gTaskDescBtlShadow, body);

    switch (work->variant) {
    case SUMMON_LEVEL_SINGLE:
        work->duration = 0x78;
        break;
    case SUMMON_LEVEL_PAIR:
        work->duration = 0xF0;
        break;
    case SUMMON_LEVEL_TRIPLE:
    default:
        work->duration = 0x1E0;
        break;
    }
}

u8 task_frd_aladdin_1(FrdAladdinWork* work) {
    BtlObj* body;
    s32 x;
    s32 y;
    s32 delta;

    body = &work->body;

    if (gGameState.world != WORLD_AGRABAH
#ifdef PLATFORM_PC
        && !(work->mainSide && PortActionCombatRules())
#endif
    ) {
        return 0;
    }

    if ((work->mainSide ? gBtlWork->flags : gRikuBtlWork->flags) & BTL_FLAG_DISMISS_SUMMONS) return 0;

    switch (work->state) {
    case FRD_ALADDIN_STATE_ENTER:
        if (work->stateTimer == 0) {
            AnimChangeWithDef(sFrdAladdinAnimDefs, &work->anim, 0, 0, work->tiles);
            work->stateTimer++;
        }

        body->x += (work->targetX - body->x) >> 4;
        ClampBattlePosition(&body->x, &body->y, -16, 0);

        if (FrdAladdinApplyGravity(work)) {
            work->state = FRD_ALADDIN_STATE_LAND;
            work->stateTimer = 0;
            m4aSongNumStart(SONG_VO_AD_ATTACK00);
        }

        break;
    case FRD_ALADDIN_STATE_LAND:
        if (work->stateTimer == 0) {
            AnimChangeWithDef(sFrdAladdinAnimDefs, &work->anim, 1, 0, work->tiles);
        }

        if (AnimIsFinished(&work->anim)) {
            work->state = FRD_ALADDIN_STATE_ATTACK;
            work->stateTimer = 0;
        } else {
            work->stateTimer++;
        }

        break;
    case FRD_ALADDIN_STATE_LEAVE:
        if (work->stateTimer == 0) {
            AnimChangeWithDef(sFrdAladdinAnimDefs, &work->anim, 0, 0, work->tiles);

            if (!(body->flags & BTLOBJ_FLAG_FACING_LEFT)) {
                work->targetX = (gBtlWork->xMin - 64) << 8;
            } else {
                work->targetX = (gBtlWork->xMax + 64) << 8;
            }

            work->vz = -0x500;
            work->steps = 30;
        }

        ApproachValue(&body->x, work->targetX, work->steps);
        FrdAladdinApplyGravity(work);

        if (work->steps <= 0) {
            return 0;
        }

        STEP_STATE(work);
        break;
    case FRD_ALADDIN_STATE_ATTACK:
        if (work->stateTimer == 0) {
            AnimChangeWithDef(sFrdAladdinAnimDefs, &work->anim, 2, ANIM_FLAG_LOOP, work->tiles);
        }

        SelectLockonTarget();

        if (work->actor->flags & BTLOBJ_FLAG_FACING_LEFT) {
            body->flags |= BTLOBJ_FLAG_FACING_LEFT;
            x = work->actor->x - 0x2800;
        } else {
            body->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
            x = work->actor->x + 0x2800;
        }

        y = work->actor->y;
        delta = (x - body->x) >> 3;

        if (delta < -0x400) {
            delta = -0x400;
        } else if (delta > 0x400) {
            delta = 0x400;
        }

        body->x += delta;
        delta = (y - body->y) >> 5;

        if (delta < -0x200) {
            delta = -0x200;
        } else if (delta > 0x200) {
            delta = 0x200;
        }

        body->y += delta;

        if (work->anim.timer == 0) {
            switch (AnimGetFrame(&work->anim)) {
            case 0:
            case 1:
            case 5:
            case 6:
                if ((body->flags & BTLOBJ_FLAG_FACING_LEFT) ? ApplyAttackBox(0x95, body->x - 0x1E00, body->y, body->z, 20, 20, 50) : ApplyAttackBox(0x95, body->x + 0x1E00, body->y, body->z, 20, 20, 50)) {
                    m4aSongNumStart(SONG_BTL_AD_SWORDHIT);
                }

                break;
            }
        }

        FrdAladdinApplyGravity(work);
        ClampBattlePosition(&body->x, &body->y, -16, 0);

        if (work->stateTimer > work->duration) {
            work->stateTimer = 0;
            work->state = FRD_ALADDIN_STATE_LEAVE;
        } else {
            work->stateTimer++;
        }

        break;
    }

    AnimUpdate(&work->anim);
    TaskPoolUpdate(&work->tasks);
    return 1;
}

void task_frd_aladdin_2(FrdAladdinWork* work) {
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

void task_frd_aladdin_3(FrdAladdinWork* work) {
    BtlWork* owner;

    owner = work->mainSide ? gBtlWork : gRikuBtlWork;
    owner->flags &= ~BTL_FLAG_SUMMON_ACTIVE;
    ReleaseObjPalette(work->palette);
    TaskPoolDestroy(&work->tasks);
}
