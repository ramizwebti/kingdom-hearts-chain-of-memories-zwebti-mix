/**
 * smn_simba.c
 * Simba Summon
 */

#include "task_descriptors.h"
#include "smn.h"
#include "anim.h"
#include "sprites_smn.h"
#include "btl_api.h"
#include "fade.h"
#include "songs.h"
#include "smn_tasks.h"
#include "battle_actor.h"
#include "battle_actor_types.h"
#include "battle_work.h"
#include "btl_collision.h"
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
#include "btl.h"

static const AnimDef sSmnSimbaAnimDef = { gShinba10Frames, gShinba10Anims, gShinba10Tiles, 0 };

TaskDesc gTaskDescSmnSimba = {
    "task_smn_simba",
    (TaskInitFunc)task_smn_simba_0,
    (TaskUpdateFunc)task_smn_simba_1,
    (TaskDrawFunc)task_smn_simba_2,
    (TaskDestroyFunc)task_smn_simba_3,
    sizeof(SmnSimbaWork),
};

enum SmnSimbaState {
    SMN_SIMBA_STATE_APPEAR,
    SMN_SIMBA_STATE_ROAR,
    SMN_SIMBA_STATE_VANISH
};

void task_smn_simba_0(SmnSimbaWork* work, SmnArgs* args) {
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
    work->palette = LoadObjPalette(gShinbaPalette, sizeof(gShinbaPalette));
    AnimInit(&work->anim, NULL, NULL);
    AnimChangeWithDef(&sSmnSimbaAnimDef, &work->anim, 0, 0, work->tiles);
    work->state = SMN_SIMBA_STATE_APPEAR;
    work->stateTimer = 0;
    work->steps = 0;
    work->unk_14C = 0;
    work->scale = 10;
    work->animating = FALSE;
    TaskPoolInit(&work->tasks, 2);
    TaskCreate(&work->tasks, &gTaskDescBtlShadow, body);
}

u8 task_smn_simba_1(SmnSimbaWork* work) {
    BtlObj* body;
    BtlWork* owner;

    body = &work->body;
    owner = work->mainSide ? gBtlWork : gRikuBtlWork;

    if (owner->flags & BTL_FLAG_DISMISS_SUMMONS) {
        // fakematch
        do {
            return 0;
        } while (0);
    }

    BtlMapFollowPosition(body->x, body->y, body->z);

    switch (work->state) {
    case SMN_SIMBA_STATE_APPEAR:
        if (work->stateTimer == 0) {
            work->steps = 30;
            BgFxStartSummon(body->x, body->y, body->z);
            m4aSongNumStart(SONG_EF_SUMMON_UP);
        }

        ApproachValue(&work->scale, Q_8_8(1), work->steps);

        if (work->steps > 0) {
            STEP_STATE(work);
        } else {
            work->state = SMN_SIMBA_STATE_ROAR;
            work->stateTimer = 0;
            work->animating = TRUE;
        }

        break;
    case SMN_SIMBA_STATE_VANISH:
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
    case SMN_SIMBA_STATE_ROAR:
        switch (work->stateTimer) {
        case 0:
            if (body->flags & BTLOBJ_FLAG_FACING_LEFT) {
                SetBattleZoom(30, Q_8_8(1.2), body->x - 0x1400,
                              body->y + body->z - 0x1400);
            } else {
                SetBattleZoom(30, Q_8_8(1.2), body->x + 0x1400,
                              body->y + body->z - 0x1400);
            }

            break;
        case 50:
            switch (work->variant) {
            case SUMMON_LEVEL_SINGLE:
                m4aSongNumStart(SONG_BTL_SIMBA_ROA0);
                break;
            case SUMMON_LEVEL_PAIR:
                m4aSongNumStart(SONG_BTL_SIMBA_ROA1);
                break;
            case SUMMON_LEVEL_TRIPLE:
            default:
                m4aSongNumStart(SONG_BTL_SIMBA_ROA2);
                break;
            }

            BtlMapStartShake();
            FadeFromAmount(FADE_MODE_GREEN, 8, 20);
            SetBattleZoom(30, Q_8_8(0.8), 0x10000, 0x15E00);

            if (body->flags & BTLOBJ_FLAG_FACING_LEFT) {
                BgFxStartShockwave(body->x - 0x1400, body->y + body->z - 0x1400, TRUE);
            } else {
                BgFxStartShockwave(body->x + 0x1400, body->y + body->z - 0x1400, FALSE);
            }

            switch (work->variant) {
            case SUMMON_LEVEL_SINGLE:
                if (body->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    ApplyAttackBox(0x99, body->x - 0x8000, body->y, body->z,
                                  0x80, 0x100, 0x100);
                } else {
                    ApplyAttackBox(0x99, body->x + 0x8000, body->y, body->z,
                                  0x80, 0x100, 0x100);
                }

                break;
            case SUMMON_LEVEL_PAIR:
                if (body->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    ApplyAttackBox(0x9A, body->x - 0x8000, body->y, body->z,
                                  0x80, 0x100, 0x100);
                } else {
                    ApplyAttackBox(0x9A, body->x + 0x8000, body->y, body->z,
                                  0x80, 0x100, 0x100);
                }

                break;
            case SUMMON_LEVEL_TRIPLE:
            default:
                if (body->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    ApplyAttackBox(0x9B, body->x - 0x8000, body->y, body->z,
                                  0x80, 0x100, 0x100);
                } else {
                    ApplyAttackBox(0x9B, body->x + 0x8000, body->y, body->z,
                                  0x80, 0x100, 0x100);
                }

                break;
            }

            break;
        }

        if (AnimIsFinished(&work->anim)) {
            SetBattleZoom(8, Q_8_8(1), gBtlWork->x2, gBtlWork->y2);
            work->state = SMN_SIMBA_STATE_VANISH;
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

void task_smn_simba_2(SmnSimbaWork* work) {
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

void task_smn_simba_3(SmnSimbaWork* work) {
    BtlWork* owner;

    owner = work->mainSide ? gBtlWork : gRikuBtlWork;
    owner->flags &= ~BTL_FLAG_SUMMON_ACTIVE;
    ReleaseObjPalette(work->palette);
    TaskPoolDestroy(&work->tasks);
}
