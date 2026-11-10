/**
 * frd_goofy.c
 * Goofy Battle Ally
 */

#include "task_descriptors.h"
#include "system_state.h"
#include "frd.h"
#include "sprites_frd.h"
#include "songs.h"
#include "frd_tasks.h"
#include "anim.h"
#include "battle_actor.h"
#include "battle_actor_types.h"
#include "battle_work.h"
#include "btl_collision.h"
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
#include "btl.h"

static const AnimDef sFrdGoofyAnimDefs[5] = {
    { gGoofy16Frames, gGoofy16Anims, gGoofy16Tiles, 0 },
    { gGoofy16Frames, gGoofy16Anims, gGoofy16Tiles, 1 },
    { gGoofy14Frames, gGoofy14Anims, gGoofy14Tiles, 0 },
    { gGoofy05Frames, gGoofy05Anims, gGoofy05Tiles, 1 },
    { gGoofy05Frames, gGoofy05Anims, gGoofy05Tiles, 2 },
};

TaskDesc gTaskDescFrdGoofy = {
    "task_frd_goofy",
    (TaskInitFunc)task_frd_goofy_0,
    (TaskUpdateFunc)task_frd_goofy_1,
    (TaskDrawFunc)task_frd_goofy_2,
    (TaskDestroyFunc)task_frd_goofy_3,
    sizeof(FrdGoofyWork),
};

u8 FrdGoofyApplyGravity(FrdGoofyWork* work) {
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

enum FrdGoofyState {
    FRD_GOOFY_STATE_ENTER,
    FRD_GOOFY_STATE_LAND,
    FRD_GOOFY_STATE_ATTACK_END,
    FRD_GOOFY_STATE_LEAVE,
    FRD_GOOFY_STATE_CHARGE,
    FRD_GOOFY_STATE_TORNADO_WINDUP,
    FRD_GOOFY_STATE_TORNADO
};

void task_frd_goofy_0(FrdGoofyWork* work, FrdArgs* args) {
    BtlObj* body;

    body = &work->body;

#ifdef VERSION_EU
    if (gLanguage == LANGUAGE_FRENCH || gLanguage == LANGUAGE_ITALIAN) {
        m4aSongNumStart(SONG_VO_SR_SUMMON00);
    } else {
        m4aSongNumStart(SONG_VO_SR_SUMMON03);
    }
#else
    m4aSongNumStart(SONG_VO_SR_SUMMON03);
#endif

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
    work->state = FRD_GOOFY_STATE_ENTER;
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
    work->palette = LoadObjPalette(gGoofyPalette, sizeof(gGoofyPalette));
    AnimInit(&work->anim, NULL, NULL);
    AnimChangeWithDef(sFrdGoofyAnimDefs, &work->anim, 0, 0, work->tiles);
    TaskPoolInit(&work->tasks, 1);
    TaskCreate(&work->tasks, &gTaskDescBtlShadow, body);
}

u8 task_frd_goofy_1(FrdGoofyWork* work) {
    BtlObj* body;
    BtlWork* owner;
    s32 t;

    body = &work->body;
    owner = work->mainSide ? gBtlWork : gRikuBtlWork;

    if (owner->flags & BTL_FLAG_DISMISS_SUMMONS) {
        return 0;
    }

    switch (work->state) {
    case FRD_GOOFY_STATE_ENTER:
        if (work->stateTimer == 0) {
            AnimChangeWithDef(sFrdGoofyAnimDefs, &work->anim, 0, 0, work->tiles);
            work->stateTimer++;
        }

        body->x += (work->targetX - body->x) >> 4;
        ClampBattlePosition(&body->x, &body->y, -16, 0);

        if (FrdGoofyApplyGravity(work)) {
            work->state = FRD_GOOFY_STATE_LAND;
            work->stateTimer = 0;
            m4aSongNumStart(SONG_VO_GF_ATTACK00);
        }

        break;
    case FRD_GOOFY_STATE_LAND:
        if (work->stateTimer == 0) {
            AnimChangeWithDef(sFrdGoofyAnimDefs, &work->anim, 1, 0, work->tiles);
        }

        if (AnimIsFinished(&work->anim)) {
            switch (work->variant) {
            case SUMMON_LEVEL_SINGLE:
            case SUMMON_LEVEL_PAIR:
                work->state = FRD_GOOFY_STATE_CHARGE;
                break;
            case SUMMON_LEVEL_TRIPLE:
                work->state = FRD_GOOFY_STATE_TORNADO_WINDUP;
                break;
            }

            work->stateTimer = 0;
        } else {
            work->stateTimer++;
        }

        break;
    case FRD_GOOFY_STATE_ATTACK_END:
        if (work->stateTimer == 0) {
            AnimChangeWithDef(sFrdGoofyAnimDefs, &work->anim, 1, 0, work->tiles);
        }

        if (AnimIsFinished(&work->anim)) {
            work->state = FRD_GOOFY_STATE_LEAVE;
            work->stateTimer = 0;
        } else {
            work->stateTimer++;
        }

        break;
    case FRD_GOOFY_STATE_LEAVE:
        if (work->stateTimer == 0) {
            AnimChangeWithDef(sFrdGoofyAnimDefs, &work->anim, 0, 0, work->tiles);

            if (body->flags & BTLOBJ_FLAG_FACING_LEFT) {
                work->targetX = (gBtlWork->xMin - 0x40) << 8;
            } else {
                work->targetX = (gBtlWork->xMax + 0x40) << 8;
            }

            work->vz = -0x500;
            work->steps = 30;
        }

        ApproachValue(&body->x, work->targetX, work->steps);
        FrdGoofyApplyGravity(work);

        if (work->steps <= 0) {
            return 0;
        }

        STEP_STATE(work);
        break;
    case FRD_GOOFY_STATE_CHARGE:
        if (work->stateTimer == 0) {
            AnimChangeWithDef(sFrdGoofyAnimDefs, &work->anim, 2, 0, work->tiles);

            if (body->flags & BTLOBJ_FLAG_FACING_LEFT) {
                work->targetX = body->x - 0x8500;
            } else {
                work->targetX = body->x + 0x8500;
            }
        }

        if (work->stateTimer == 40) {
            work->targetY = work->actor->y;
        }

        if (work->stateTimer > 39) {
            body->x += (work->targetX - body->x) >> 4;
            body->y += (work->targetY - body->y) >> 4;

            if (body->flags & BTLOBJ_FLAG_FACING_LEFT
                    ? ApplyAttackBox(work->variant + 120, body->x - 0xF00, body->y, body->z, 0x1E, 0x0C, 0x30)
                    : ApplyAttackBox(work->variant + 120, body->x + 0xF00, body->y, body->z, 0x1E, 0x0C, 0x30)) {
                m4aSongNumStart(SONG_EF_GFHIT);
            }

            ClampBattlePosition(&body->x, &body->y, -16, 0);
        }

        FrdGoofyApplyGravity(work);

        if (AnimIsFinished(&work->anim)) {
            work->state = FRD_GOOFY_STATE_ATTACK_END;
            work->stateTimer = 0;
        } else {
            work->stateTimer++;
        }

        break;
    case FRD_GOOFY_STATE_TORNADO_WINDUP:
        if (work->stateTimer == 0) {
            AnimChangeWithDef(sFrdGoofyAnimDefs, &work->anim, 3, 0, work->tiles);
        }

        FrdGoofyApplyGravity(work);

        if (AnimIsFinished(&work->anim)) {
            work->state = FRD_GOOFY_STATE_TORNADO;
            work->stateTimer = 0;
        } else {
            work->stateTimer++;
        }

        break;
    case FRD_GOOFY_STATE_TORNADO:
        if (work->stateTimer == 0) {
            AnimChangeWithDef(sFrdGoofyAnimDefs, &work->anim, 4, ANIM_FLAG_LOOP, work->tiles);
            work->angle = GetRandom();
        }

        work->targetX = work->actor->x + (gSineTable[work->angle] << 6);
        work->targetY = work->actor->y - (gSineTable[work->angle + 0x40] << 5);
        body->x += (work->targetX - body->x) >> 3;
        body->y += (work->targetY - body->y) >> 3;
        ClampBattlePosition(&body->x, &body->y, -16, 0);
        work->angle += 4;

        if (ApplyAttackBox(0x7A, body->x, body->y, body->z, 0x23, 0x1C, 0x30)) {
            m4aSongNumStart(SONG_EF_GFHIT);
        }

        FrdGoofyApplyGravity(work);

        if (work->stateTimer > 179) {
            work->state = FRD_GOOFY_STATE_ATTACK_END;
            work->stateTimer = 0;
        }

        work->stateTimer++;
        break;
    }

    AnimUpdate(&work->anim);
    TaskPoolUpdate(&work->tasks);
    return 1;
}

void task_frd_goofy_2(FrdGoofyWork* work) {
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

void task_frd_goofy_3(FrdGoofyWork* work) {
    BtlWork* owner;

    owner = work->mainSide ? gBtlWork : gRikuBtlWork;
    owner->flags &= ~BTL_FLAG_SUMMON_ACTIVE;
    ReleaseObjPalette(work->palette);
    TaskPoolDestroy(&work->tasks);
}
