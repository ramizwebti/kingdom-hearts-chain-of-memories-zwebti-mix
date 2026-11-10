/**
 * smn_tink.c
 * Tinker Bell Summon
 */

#include "task_descriptors.h"
#include "smn.h"
#include "anim.h"
#include "sprites_evt.h"
#include "sprites_smn.h"
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
#include "card_label_data.h"
#include "btl.h"

static const AnimDef sSmnTinkAnimDefs[3] = {
    { gTinkF00Frames, gTinkF00Anims, gTinkF00Tiles, 1 },
    { gTinkF00Frames, gTinkF00Anims, gTinkF00Tiles, 2 },
    { gTinkF00Frames, gTinkF00Anims, gTinkF00Tiles, 3 },
};

TaskDesc gTaskDescSmnTink = {
    "task_smn_tink",
    (TaskInitFunc)task_smn_tink_0,
    (TaskUpdateFunc)task_smn_tink_1,
    (TaskDrawFunc)task_smn_tink_2,
    (TaskDestroyFunc)task_smn_tink_3,
    sizeof(SmnTinkWork),
};

TaskDesc gTaskDescSmnTinkeff = {
    "task_smn_tinkeff",
    (TaskInitFunc)task_smn_tinkeff_0,
    (TaskUpdateFunc)task_smn_tinkeff_1,
    (TaskDrawFunc)task_smn_tinkeff_2,
    (TaskDestroyFunc)task_smn_tinkeff_3,
    sizeof(SmnTinkeffWork),
};

enum SmnTinkState {
    SMN_TINK_STATE_APPEAR,
    SMN_TINK_STATE_VANISH,
    SMN_TINK_STATE_HOVER,
    SMN_TINK_STATE_ORBIT,
    SMN_TINK_STATE_LOOP_THE_LOOP
};

void task_smn_tink_0(SmnTinkWork* work, SmnArgs* args) {
    BtlObj* body;
    BtlObj* actor;
    s32 spawnX, spawnY, spawnZ;
#ifdef PLATFORM_PC
    int independent;
#endif
    s32 healRate;

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
    independent=PortActionAssistSpawnOrigin(args->mainSide,&spawnX,&spawnY,&spawnZ);
#endif
    body->x = spawnX;
    body->y = spawnY;
    body->z = spawnZ - 0x3000;
#ifdef VERSION_EU
    body->groundZ = 0;
#else
    body->groundZ = spawnZ;
#endif

    if (actor->flags & BTLOBJ_FLAG_FACING_LEFT) {
        body->flags = (BTLOBJ_FLAG_FACING_LEFT | BTLOBJ_FLAG_SMALL_SHADOW);
    } else {
        body->flags = BTLOBJ_FLAG_SMALL_SHADOW;
    }

    work->variant = args->variant;
#ifdef PLATFORM_PC
    work->tiles = PortActionAssistSpawnTiles(args->mainSide, work->tiles);
#endif
    work->palette = LoadObjPalette(gTinkPalette, sizeof(gTinkPalette));
    AnimInit(&work->anim, NULL, NULL);
    AnimChangeWithDef(sSmnTinkAnimDefs, &work->anim, 0, ANIM_FLAG_LOOP, work->tiles);
    work->state = SMN_TINK_STATE_APPEAR;
    work->stateTimer = 0;
    work->steps = 0;
    work->scale = 10;
    work->animating = FALSE;
    work->unk_150 = 0;
    work->flyAngle = 0;
    work->frameCount = 0;

    if (work->mainSide) {
        work->actor = gBtlWork->actor;
    } else {
        work->actor = gRikuBtlWork->actor;
    }

    m4aSongNumStart(SONG_VO_SR_SUMMON00);
    work->healHp = work->actor->hp << 8;

    switch (args->variant) {
    case SUMMON_LEVEL_SINGLE:
        healRate = 0x4C;
        work->healFrames = 0xB4;
        break;
    case SUMMON_LEVEL_PAIR:
        healRate = 0x99;
        work->healFrames = 0x12C;
        break;
    case SUMMON_LEVEL_TRIPLE:
    default:
        healRate = 0x100;
        work->healFrames = 0x1A4;
        break;
    }

    if (work->actor->btl->hcEffect == HC_EFFECT_SUMMON_BOOST) {
        healRate = 332 * healRate >> 8;
    }

    work->healTarget = work->actor->maxHp * healRate + work->healHp;

    if (work->healTarget > work->actor->maxHp << 8
#ifdef PLATFORM_PC
        && !independent
#endif
    ) {
        work->healTarget = work->actor->maxHp << 8;
    }

    TaskPoolInit(&work->tasks, 15);
    TaskCreate(&work->tasks, &gTaskDescBtlShadow, body);
}

void SmnTinkSpawnSparkle(SmnTinkWork* work) {
    if (work->frameCount % 3 == 0) {
        TaskCreate(&work->tasks, &gTaskDescSmnTinkeff, &work->body);
    }
}

u8 task_smn_tink_1(SmnTinkWork* work) {
    BtlObj* body;
    BtlObj* actor;
    s32 x;
    s32 y;
    s32 z;
    s32 delta;
    s32 timer;

    body = &work->body;

    if ((work->mainSide ? gBtlWork->flags : gRikuBtlWork->flags) & BTL_FLAG_DISMISS_SUMMONS) {
        return 0;
    }

    if (work->healFrames <= 0) {
        if (work->state != SMN_TINK_STATE_VANISH) {
            work->state = SMN_TINK_STATE_VANISH;
            work->stateTimer = 0;
        }
    } else {
#ifdef PLATFORM_PC
        s32 previousHeal=work->healHp>>8;
#endif
        ApproachValue(&work->healHp, work->healTarget, work->healFrames);
#ifdef PLATFORM_PC
        if (PortActionAssistOwnsNative(work)) {
            unsigned hpBefore=(u16)work->actor->hp;
            /* Healing is additive while Sora fights. Native takeover instead
             * assigns a precomputed HP path, which would erase fresh damage. */
            if (work->actor->hp>0) {
                work->actor->hp+=(work->healHp>>8)-previousHeal;
                if (work->actor->hp>work->actor->maxHp) work->actor->hp=work->actor->maxHp;
            }
            PortActionAssistHealed(hpBefore,(u16)work->actor->hp);
        } else
#endif
        work->actor->hp = work->healHp >> 8;
        work->healFrames--;
    }

    switch (work->state) {
    case SMN_TINK_STATE_APPEAR:
        if (work->stateTimer == 0) {
            work->steps = 30;
            BgFxStartSummon(body->x, body->y, body->z);
            m4aSongNumStart(SONG_EF_SUMMON_UP);
        }

        ApproachValue(&work->scale, Q_8_8(1), work->steps);

        if (work->steps <= 0) {
            work->state = SMN_TINK_STATE_HOVER;
            work->stateTimer = 0;
            work->animating = TRUE;
            m4aSongNumStart(SONG_EF_TINK_LOOP);
        } else {
            STEP_STATE(work);
        }

        break;
    case SMN_TINK_STATE_VANISH:
        if (work->stateTimer == 0) {
            work->steps = 20;
            m4aSongNumStop(SONG_EF_TINK_LOOP);
            BgFxStartSummon(body->x, body->y, body->z);
            m4aSongNumStart(SONG_EF_SUMMON_DOWN);
        }

        ApproachValue(&work->scale, Q_8_8(0.1), work->steps);

        if (work->steps <= 0) {
            return 0;
        }

        STEP_STATE(work);
        break;
    case SMN_TINK_STATE_HOVER:
        SmnTinkSpawnSparkle(work);

        if (work->stateTimer == 0) {
            AnimChangeWithDef(sSmnTinkAnimDefs, &work->anim, 0, ANIM_FLAG_LOOP, work->tiles);
            work->hoverZ = body->z;
            work->steps = 30;
        } else {
            body->z += (work->hoverZ + SIN(work->stateTimer) * 12
                             - body->z) >> 2;
        }

        if (work->actor->x < body->x) {
            body->flags |= BTLOBJ_FLAG_FACING_LEFT;
        } else {
            body->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
        }

        if (work->steps-- <= 0) {
            work->stateTimer = 0;
            work->state = SMN_TINK_STATE_ORBIT;
        } else {
            work->stateTimer++;
        }

        break;
    case SMN_TINK_STATE_ORBIT:
        SmnTinkSpawnSparkle(work);

        if (work->stateTimer == 0) {
            AnimChangeWithDef(sSmnTinkAnimDefs, &work->anim, 1, ANIM_FLAG_LOOP, work->tiles);
        }

        actor = work->actor;
        x = actor->x + SIN((u16)work->stateTimer * 4) * 32;
        y = actor->y + COS((u16)work->stateTimer * 4) * -16;
        z = (actor->z - 0x1E00) + SIN((u16)work->stateTimer * 2) * 16;

        if (x < body->x) {
            body->flags |= BTLOBJ_FLAG_FACING_LEFT;
        } else {
            body->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
        }

        delta = (x - body->x) >> 3;

        if (delta > 0x400) {
            delta = 0x400;
        } else if (delta < -0x400) {
            delta = -0x400;
        }

        body->x += delta;
        delta = (y - body->y) >> 3;

        if (delta > 0x200) {
            delta = 0x200;
        } else if (delta < -0x200) {
            delta = -0x200;
        }

        body->y += delta;
        body->z += (z - body->z) >> 3;
        timer = work->stateTimer % 60;

        if (timer == 0) {
            switch (GetRandom() % 3) {
            case 0:
                work->speed = 0x280;
                work->state = SMN_TINK_STATE_LOOP_THE_LOOP;
                work->stateTimer = timer;
                break;
            case 1:
                work->state = SMN_TINK_STATE_HOVER;
                work->stateTimer = timer;
                break;
            case 2:
            default:
                work->stateTimer++;
                break;
            }
        } else {
            work->stateTimer++;
        }

        break;
    case SMN_TINK_STATE_LOOP_THE_LOOP:
        SmnTinkSpawnSparkle(work);

        if (work->stateTimer == 0) {
            AnimChangeWithDef(sSmnTinkAnimDefs, &work->anim, 2, 0, work->tiles);

            if (body->flags & BTLOBJ_FLAG_FACING_LEFT) {
                work->flyAngle = 0xC0;
            } else {
                work->flyAngle = 0x40;
            }
        }

        body->x += gSineTable[(u8)work->flyAngle] * work->speed >> 8;
        body->z += -gSineTable[(u8)work->flyAngle + 0x40] * work->speed >> 8;

        if (body->flags & BTLOBJ_FLAG_FACING_LEFT) {
            work->flyAngle += 7;
        } else {
            work->flyAngle -= 7;
        }

        if (AnimIsFinished(&work->anim)) {
            AnimChangeWithDef(sSmnTinkAnimDefs, &work->anim, 1, ANIM_FLAG_LOOP, work->tiles);
            work->state = SMN_TINK_STATE_ORBIT;
            work->stateTimer = 1;
        } else {
            work->stateTimer++;
        }

        break;
    }

    ClampBattlePosition(&body->x, &body->y, -16, 0);

    if (work->animating) {
        AnimUpdate(&work->anim);
    }

    TaskPoolUpdate(&work->tasks);
    work->frameCount++;
    return 1;
}

void task_smn_tink_2(SmnTinkWork* work) {
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

void task_smn_tink_3(SmnTinkWork* work) {
    BtlWork* owner;

    owner = work->mainSide ? gBtlWork : gRikuBtlWork;
    owner->flags &= ~BTL_FLAG_SUMMON_ACTIVE;
    m4aSongNumStop(SONG_EF_TINK_LOOP);
    ReleaseObjPalette(work->palette);
    TaskPoolDestroy(&work->tasks);
}

static inline s32 GetTinkEffectOffset() {
    return ((u16)(GetRandom() % 9) << 8) - 0x400;
}

void task_smn_tinkeff_0(SmnTinkeffWork* work, BtlObj* args) {
    work->x = args->x + GetTinkEffectOffset();
    work->y = args->y + GetTinkEffectOffset();
    work->z = args->z;
    work->vz = (u16)(GetRandom() % 0xE8) + 0x4C;
    work->tiles = LoadObjTiles(gSmnTinkEffTiles, sizeof(gSmnTinkEffTiles));
    work->palette = LoadObjPalette(gBStatesPalette, sizeof(gBStatesPalette));
    AnimInit(&work->anim, gSmnTinkEffAnims, gSmnTinkEffFrames);

    switch ((u16)(GetRandom() % 3)) {
    case 0:
        AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
        break;
    case 1:
        AnimStart(&work->anim, 1, ANIM_FLAG_LOOP);
        break;
    case 2:
        AnimStart(&work->anim, 2, ANIM_FLAG_LOOP);
        break;
    }
}

u8 task_smn_tinkeff_1(SmnTinkeffWork* work) {
    work->z += work->vz;

    if (AnimIsFinished(&work->anim)) {
        return 0;
    }

    AnimUpdate(&work->anim);
    return 1;
}

void task_smn_tinkeff_2(SmnTinkeffWork* work) {
    void* gfx;
    s16 sx;
    s16 sy;

    gfx = AnimGetGfx(&work->anim);
    WorldToScreen(&sx, &sy, work->x, work->y, work->z);
    DrawSprite(sx, sy, gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2),
               -4100 - ((work->y >> 8) * 4));
}

void task_smn_tinkeff_3(SmnTinkeffWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}
