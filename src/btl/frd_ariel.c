/**
 * frd_ariel.c
 * Ariel Battle Ally
 */

#include "task_descriptors.h"
#include "frd.h"
#include "sprites_frd.h"
#include "world_types.h"
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

static const AnimDef sFrdArielAnimDefs[3] = {
    { gFrdArielFrames, gFrdArielAnims, gFrdArielTiles, 0 },
    { gFrdArielFrames, gFrdArielAnims, gFrdArielTiles, 1 },
    { gFrdArielFrames, gFrdArielAnims, gFrdArielTiles, 2 },
};

TaskDesc gTaskDescFrdAriel = {
    "task_frd_ariel",
    (TaskInitFunc)task_frd_ariel_0,
    (TaskUpdateFunc)task_frd_ariel_1,
    (TaskDrawFunc)task_frd_ariel_2,
    (TaskDestroyFunc)task_frd_ariel_3,
    sizeof(FrdArielWork),
};

enum FrdArielState {
    FRD_ARIEL_STATE_ENTER,
    FRD_ARIEL_STATE_ACCELERATE,
    FRD_ARIEL_STATE_CHARGE
};

void task_frd_ariel_0(FrdArielWork* work, FrdArgs* args) {
    BtlObj* body;

    body = &work->body;
    m4aSongNumStart(SONG_VO_SR_SUMMON10);

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
    work->state = FRD_ARIEL_STATE_ENTER;
    work->stateTimer = 0;

    if (work->actor->flags & BTLOBJ_FLAG_FACING_LEFT) {
        body->x = (gBtlWork->xMax + 0x30) << 8;
        body->flags = BTLOBJ_FLAG_FACING_LEFT;
    } else {
        body->x = (gBtlWork->xMin - 0x30) << 8;
        body->flags = 0;
    }

    body->y = work->actor->y;
    body->groundZ = 0;
    work->hoverZ = -0x1000;
    body->z = -0x1000;
#ifdef PLATFORM_PC
    work->tiles = PortActionAssistSpawnTiles(args->mainSide, work->tiles);
#endif
    work->palette = LoadObjPalette(gArielPalette, sizeof(gArielPalette));
    AnimInit(&work->anim, NULL, NULL);
    AnimChangeWithDef(sFrdArielAnimDefs, &work->anim, 1, 0, work->tiles);
    TaskPoolInit(&work->tasks, 1);
    TaskCreate(&work->tasks, &gTaskDescBtlShadow, body);

    switch (args->variant) {
    case SUMMON_LEVEL_SINGLE:
        work->passSpeed = 0x500;
        work->passesLeft = 0;
        break;
    case SUMMON_LEVEL_PAIR:
        work->passSpeed = 0x800;
        work->passesLeft = 1;
        break;
    case SUMMON_LEVEL_TRIPLE:
    default:
        work->passSpeed = 0xC00;
        work->passesLeft = 4;
        break;
    }
}

u8 task_frd_ariel_1(FrdArielWork* work) {
    BtlObj* body;
    BtlWork* owner;
    s32 pixelX;

    body = &work->body;

    if (gGameState.world != WORLD_ATLANTICA
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

    ApplyBattleBounds(&body->x, &body->y, &body->z, &body->groundZ);

    switch (work->state) {
    case FRD_ARIEL_STATE_ENTER:
        if (body->flags & BTLOBJ_FLAG_FACING_LEFT) {
            pixelX = gBtlWork->xMax - 0x30;
        } else {
            pixelX = gBtlWork->xMin + 0x30;
        }

        body->x += ((pixelX << 8) - body->x) >> 3;

        if (work->stateTimer > 20) {
            work->stateTimer = 0;
            work->state = FRD_ARIEL_STATE_ACCELERATE;
        } else {
            work->stateTimer++;
        }

        break;
    case FRD_ARIEL_STATE_ACCELERATE:
        if (work->stateTimer == 0) {
            work->speed = 0;
            work->steps = 12;
            AnimChangeWithDef(sFrdArielAnimDefs, &work->anim, 2, 0, work->tiles);
        }

        switch (AnimGetFrame(&work->anim)) {
        case 0:
        case 1:
        case 2:
            break;
        case 3:
        default:
            if (work->steps > 0) {
                ApproachValue(&work->speed, work->passSpeed, work->steps);
                work->steps--;
            }

            if (body->flags & BTLOBJ_FLAG_FACING_LEFT) {
                body->x -= work->speed;
            } else {
                body->x += work->speed;
            }

            break;
        }

        if (work->steps <= 0 && AnimIsFinished(&work->anim)) {
            work->stateTimer = 0;
            work->state = FRD_ARIEL_STATE_CHARGE;
        } else {
            work->stateTimer++;
        }

        break;
    case FRD_ARIEL_STATE_CHARGE:
        AnimChangeWithDef(sFrdArielAnimDefs, &work->anim, 0, ANIM_FLAG_LOOP, work->tiles);

        if (body->flags & BTLOBJ_FLAG_FACING_LEFT
                ? ApplyAttackBox(0x77, body->x, body->y, body->z, 0x10, 0x10, 0x10)
                : ApplyAttackBox(0x77, body->x, body->y, body->z, 0x10, 0x10, 0x10)) {
            m4aSongNumStart(SONG_BTL_AR_PUNCHHIT);
        }

        if (body->flags & BTLOBJ_FLAG_FACING_LEFT) {
            body->x -= work->passSpeed;

            if (body->x < (gBtlWork->xMin - 0x30) << 8) {
                if (work->passesLeft == 0) {
                    return 0;
                }

                work->passesLeft--;
                body->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
                work->stateTimer = 0;
                body->y = work->actor->y;
                MakeOpponentsHittable();
            }
        } else {
            body->x += work->passSpeed;

            if (body->x > (gBtlWork->xMax + 0x30) << 8) {
                if (work->passesLeft == 0) {
                    return 0;
                }

                work->passesLeft--;
                body->flags |= BTLOBJ_FLAG_FACING_LEFT;
                work->stateTimer = 0;
                body->y = work->actor->y;
                MakeOpponentsHittable();
            }
        }

        body->z = work->hoverZ + (SIN((u16)work->stateTimer * 8) << 3);
        body->y += (work->actor->y - body->y) >> 4;

        if (work->stateTimer == 20) {
            m4aSongNumStart(SONG_VO_AR_ATTACK00);
        }

        work->stateTimer++;
        break;
    }

    AnimUpdate(&work->anim);
    TaskPoolUpdate(&work->tasks);
    return 1;
}

void task_frd_ariel_2(FrdArielWork* work) {
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

void task_frd_ariel_3(FrdArielWork* work) {
    BtlWork* owner;

    owner = work->mainSide ? gBtlWork : gRikuBtlWork;
    owner->flags &= ~BTL_FLAG_SUMMON_ACTIVE;
    ReleaseObjPalette(work->palette);
    TaskPoolDestroy(&work->tasks);
}
