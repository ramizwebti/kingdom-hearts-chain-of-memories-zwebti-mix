/**
 * smn_bambi.c
 * Bambi Summon
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
#include "btl_collision.h"
#include "btl_effect.h"
#include "engine_math.h"
#include "listpool.h"
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
#include "battle.h"

static const AnimDef sSmnBambiAnimDef = { gBanb00Frames, gBanb00Anims, gBanb00Tiles, 0 };

TaskDesc gTaskDescSmnBambi = {
    "task_smn_bambi",
    (TaskInitFunc)task_smn_bambi_0,
    (TaskUpdateFunc)task_smn_bambi_1,
    (TaskDrawFunc)task_smn_bambi_2,
    (TaskDestroyFunc)task_smn_bambi_3,
    sizeof(SmnBambiWork),
};

void SmnBambiPickHopTarget(SmnBambiWork* work) {
    work->targetX = work->body.x + gSineTable[work->angle] * 80;
    work->targetY = work->body.y + -gSineTable[work->angle + 0x40] * 40;
    work->angle += GetRandom() % 0x21 + 0x20;

    if (work->targetX - work->body.x > 0) {
        work->body.flags |= BTLOBJ_FLAG_FACING_LEFT;
    } else {
        work->body.flags &= ~BTLOBJ_FLAG_FACING_LEFT;
    }
}

enum SmnBambiState {
    SMN_BAMBI_STATE_APPEAR,
    SMN_BAMBI_STATE_HOP,
    SMN_BAMBI_STATE_STOMP,
    SMN_BAMBI_STATE_VANISH
};

void task_smn_bambi_0(SmnBambiWork* work, SmnArgs* args) {
    BtlObj* body;
    BtlObj* actor;
    s32 spawnX, spawnY, spawnZ;

    body = &work->body;

    if (args->mainSide) {
        work->mainSide = TRUE;
        gBtlWork->flags |= BTL_FLAG_SUMMON_ACTIVE;
        actor = gBtlWork->actor;
        work->tiles = gBtlWork->tiles2;
    } else {
        work->mainSide = args->mainSide;
        gRikuBtlWork->flags |= BTL_FLAG_SUMMON_ACTIVE;
        actor = gRikuBtlWork->actor;
        work->tiles = gBtlWork->tiles2;
    }

    spawnX=actor->originX;spawnY=actor->originY;spawnZ=actor->originZ;
#ifdef PLATFORM_PC
    PortActionAssistSpawnOrigin(args->mainSide,&spawnX,&spawnY,&spawnZ);
#endif
    body->x = (gBtlWork->xMin
                    + GetRandom() % (gBtlWork->xMax - gBtlWork->xMin + 1)) << 8;
    body->y = (gBtlWork->yMin
                    + GetRandom() % (gBtlWork->yMax - gBtlWork->yMin + 1)) << 8;
    body->z = spawnZ;
    body->groundZ = spawnZ;

    if (actor->flags & BTLOBJ_FLAG_FACING_LEFT) {
        body->flags = 0;
        work->angle = 0xC0;
    } else {
        body->flags = BTLOBJ_FLAG_FACING_LEFT;
        work->angle = 0x40;
    }

    work->variant = args->variant;
#ifdef PLATFORM_PC
    work->tiles = PortActionAssistSpawnTiles(args->mainSide, work->tiles);
#endif
    work->palette = LoadObjPalette(gBanbPalette, sizeof(gBanbPalette));
    work->vz = 0;
    AnimInit(&work->anim, NULL, NULL);
    AnimChangeWithDef(&sSmnBambiAnimDef, &work->anim, 0, 0, work->tiles);
    work->state = SMN_BAMBI_STATE_APPEAR;
    work->stateTimer = 0;
    work->steps = 0;
    work->hopTimer = 0;
    work->unk_150 = 0;
    work->scale = 10;
    work->animating = FALSE;
    work->unk_160 = 0;
    work->target = NULL;
    work->targetIndex = 0;
    m4aSongNumStart(SONG_VO_SR_SUMMON00);
    TaskPoolInit(&work->tasks, 2);
    TaskCreate(&work->tasks, &gTaskDescBtlShadow, body);
}

u8 SmnBambiApplyGravity(SmnBambiWork* work) {
    BtlObj* body;

    body = &work->body;
    body->groundZ = 0;
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

BtlObj* SmnBambiNextTarget(SmnBambiWork* work) {
    BtlObj* list[10];
    BtlObj* obj;
    s16 count;

    if (gBtlWork->flags & BTL_FLAG_VS_BATTLE) {
        if (work->mainSide) {
            obj = gRikuBtlWork->actor;
        } else {
            obj = gBtlWork->actor;
        }

        if (obj->hp <= 0) {
            return NULL;
        }

        return obj;
    }

    count = 0;
    obj = ListPoolFirst(&gBtlWork->pool);

    while (obj != NULL) {
        if (!(obj->flags & BTLOBJ_FLAG_UNHITTABLE)) {
            list[count] = obj;
            count++;

            if (count > 9) {
                break;
            }
        }

        obj = ListPoolNext(&obj->node);
    }

    if (count == 0) {
        return NULL;
    }

    obj = list[work->targetIndex % count];
    work->targetIndex++;
    return obj;
}

u8 task_smn_bambi_1(SmnBambiWork* work) {
    BtlObj* body;
    BtlWork* owner;
    SmnPrizeArgs args;

    body = &work->body;
    owner = work->mainSide ? gBtlWork : gRikuBtlWork;

    if (owner->flags & BTL_FLAG_DISMISS_SUMMONS) {
        return 0;
    }

    switch (work->state) {
    case SMN_BAMBI_STATE_APPEAR:
        if (work->mainSide) {
            BtlMapFollowPosition(body->x, body->y, body->z);
        }

        if (work->stateTimer == 0) {
            work->steps = 30;
            BgFxStartSummon(body->x, body->y, body->z);
            m4aSongNumStart(SONG_EF_SUMMON_UP);
        }

        ApproachValue(&work->scale, Q_8_8(1), work->steps);

        if (work->steps > 0) {
            STEP_STATE(work);
        } else {
            if (work->variant == SUMMON_LEVEL_SLEIGHT) {
                work->state = SMN_BAMBI_STATE_STOMP;
            } else {
                work->state = SMN_BAMBI_STATE_HOP;
            }

            work->stateTimer = 0;
            work->animating = TRUE;
        }

        break;
    case SMN_BAMBI_STATE_VANISH:
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
    case SMN_BAMBI_STATE_STOMP:
        if (work->stateTimer == 0) {
            AnimStart(&work->anim, 0, 0);
            work->vz = -0x480;
            m4aSongNumStart(SONG_BTL_BBI_JUMP);
            work->target = SmnBambiNextTarget(work);
            work->steps = 0;
        }

        if (work->vz > 0) {
            MakeOpponentsHittable();

            if (ApplyAttackBox(0x76, body->x, body->y, body->z - 0x400, 8, 8, 2) != ATTACK_RESULT_NONE) {
                BgFxStartFriendHit(body->x, body->y, body->z);
                AnimStart(&work->anim, 0, 0);
                work->vz = -0x400;
                m4aSongNumStart(SONG_BTL_BBI_JUMP);
                m4aSongNumStart(SONG_BTL_BANBIHIT);
                work->target = SmnBambiNextTarget(work);
                work->steps++;
            }
        }

        if (SmnBambiApplyGravity(work)) {
            AnimStart(&work->anim, 0, 0);
            work->vz = -0x480;
            m4aSongNumStart(SONG_BTL_BBI_JUMP);
            work->target = SmnBambiNextTarget(work);
            work->steps++;
        }

        if ((work->vz > 0 && work->steps > 7) || work->target == NULL) {
            work->state = SMN_BAMBI_STATE_VANISH;
            work->stateTimer = 0;
        } else {
            if (body->x < work->target->x) {
                body->flags |= BTLOBJ_FLAG_FACING_LEFT;
            } else {
                body->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
            }

            body->x += (work->target->x - body->x) >> 4;
            body->y += (work->target->y - body->y) >> 4;
            ClampBattlePosition(&body->x, &body->y, -16, 0);
            work->stateTimer++;
        }

        break;
    case SMN_BAMBI_STATE_HOP:
        if (work->hopTimer == 0) {
            AnimStart(&work->anim, 0, 0);
            SmnBambiPickHopTarget(work);
            work->steps = 30;
        }

        if (work->variant == SUMMON_LEVEL_TRIPLE) {
            ApplyAttackBox(0x75, body->x, body->y, body->z, 8, 8, 8);
        } else {
            ApplyAttackBox(0x74, body->x, body->y, body->z, 8, 8, 8);
        }

        if (work->hopTimer > 4) {
            if (work->hopTimer == 5) {
                work->vz = -0x300;
                m4aSongNumStart(SONG_BTL_BBI_JUMP);
            }

            if (work->steps > 0) {
                ApproachValueHalfSteps(&body->x, work->targetX, work->steps);
                ApproachValueHalfSteps(&body->y, work->targetY, work->steps);
                work->steps--;
            }
        }

        SmnBambiApplyGravity(work);
        ClampBattlePosition(&body->x, &body->y, -16, 0);

        if (AnimIsFinished(&work->anim)) {
            work->hopTimer = 0;
            work->stateTimer++;
            args.x = body->x;
            args.y = body->y;
            args.z = body->z;

            if (work->variant == SUMMON_LEVEL_SINGLE) {
                args.kind = BTL_PRIZE_HP_3;
            } else {
                args.kind = BTL_PRIZE_HP_10;
            }

            args.noTimeout = 0;
            TaskCreate(&gBtlWork->taskPools[0], &gTaskDescBtlPrize, &args);
        } else {
            work->hopTimer++;
        }

        if (work->stateTimer > 4) {
            work->state = SMN_BAMBI_STATE_VANISH;
            work->stateTimer = 0;
        }

        break;
    }

    if (work->animating) {
        AnimUpdate(&work->anim);
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

void task_smn_bambi_2(SmnBambiWork* work) {
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

void task_smn_bambi_3(SmnBambiWork* work) {
    BtlWork* owner;

    owner = work->mainSide ? gBtlWork : gRikuBtlWork;
    owner->flags &= ~BTL_FLAG_SUMMON_ACTIVE;
    ReleaseObjPalette(work->palette);
    TaskPoolDestroy(&work->tasks);
}
