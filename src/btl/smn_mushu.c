/**
 * smn_mushu.c
 * Mushu Summon
 */

#include "task_descriptors.h"
#include "display.h"
#include "smn.h"
#include "anim.h"
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
#include "btl.h"

static const AnimDef sSmnMushuAnimDefs[4] = {
    { gMushu10Frames, gMushu10Anims, gMushu10Tiles, 0 },
    { gMushu10Frames, gMushu10Anims, gMushu10Tiles, 1 },
    { gMushu10Frames, gMushu10Anims, gMushu10Tiles, 2 },
    { gMushu10Frames, gMushu10Anims, gMushu10Tiles, 3 },
};

const AnimDef gSmnMushuEndAnimDef = { gMushu10Frames, gMushu10Anims, gMushu10Tiles, 4 };

TaskDesc gTaskDescSmnMushu = {
    "task_smn_mushu",
    (TaskInitFunc)task_smn_mushu_0,
    (TaskUpdateFunc)task_smn_mushu_1,
    (TaskDrawFunc)task_smn_mushu_2,
    (TaskDestroyFunc)task_smn_mushu_3,
    sizeof(SmnMushuWork),
};

enum SmnMushuState {
    SMN_MUSHU_STATE_APPEAR,
    SMN_MUSHU_STATE_VANISH,
    SMN_MUSHU_STATE_FIRE,
    SMN_MUSHU_STATE_FIRE_WINDUP
};

void task_smn_mushu_0(SmnMushuWork* work, SmnArgs* args) {
    BtlObj* body;
    BtlObj* actor;

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

    body->x = actor->x;
    body->y = actor->y;
    body->z = actor->z - 0x2200;
    body->groundZ = actor->groundZ;

    if (actor->flags & BTLOBJ_FLAG_FACING_LEFT) {
        body->flags = BTLOBJ_FLAG_FACING_LEFT;
    } else {
        body->flags = 0;
    }

    work->variant = args->variant;
#ifdef PLATFORM_PC
    work->tiles = PortActionAssistSpawnTiles(args->mainSide, work->tiles);
#endif
    work->palette = LoadObjPalette(gMushuPalette, sizeof(gMushuPalette));
    AnimInit(&work->anim, NULL, NULL);
    AnimChangeWithDef(sSmnMushuAnimDefs, &work->anim, 0, ANIM_FLAG_LOOP, work->tiles);
    work->state = SMN_MUSHU_STATE_APPEAR;
    work->stateTimer = 0;
    work->scaleSteps = 0;
    work->scale = 10;
    work->animating = FALSE;
    work->unk_150 = 0;

    if (work->mainSide) {
        work->actor = gBtlWork->actor;
    } else {
        work->actor = gRikuBtlWork->actor;
    }

    m4aSongNumStart(SONG_VO_SR_SUMMON00);
    TaskPoolInit(&work->tasks, 3);
}

u8 task_smn_mushu_1(SmnMushuWork* work) {
    BtlObj* body;
    BtlWork* owner;
    s32 prevX;
    s32 prevY;
    s32 prevZ;
    s32 x;
    s32 y;
    s32 z;
    s32 attack;
    u16 frame;
    u16 timer;

    body = &work->body;
    owner = work->mainSide ? gBtlWork : gRikuBtlWork;

    if (owner->flags & BTL_FLAG_DISMISS_SUMMONS) {
        return 0;
    }

    prevX = body->x;
    prevY = body->y;
    prevZ = body->z;
    body->x = work->actor->x;
    body->y = work->actor->y;
    body->z = work->actor->z - 0x2200;

    if (work->actor->flags & BTLOBJ_FLAG_FACING_LEFT) {
        body->flags |= BTLOBJ_FLAG_FACING_LEFT;
    } else {
        body->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
    }

    switch (work->state) {
    case SMN_MUSHU_STATE_APPEAR:
        if (work->stateTimer == 0) {
            work->scaleSteps = 30;
            BgFxStartSummon(body->x, body->y, body->z);
            m4aSongNumStart(SONG_EF_SUMMON_UP);
        }

        ApproachValue(&work->scale, Q_8_8(1), work->scaleSteps);

        if (work->scaleSteps > 0) {
            work->stateTimer++;
            work->scaleSteps--;
        } else {
            work->state = SMN_MUSHU_STATE_FIRE_WINDUP;
            work->stateTimer = 0;
            work->animating = TRUE;
        }

        break;
    case SMN_MUSHU_STATE_VANISH:
        if (work->stateTimer == 0) {
            work->scaleSteps = 30;
            BgFxStartSummon(body->x, body->y, body->z);
            m4aSongNumStart(SONG_EF_SUMMON_DOWN);
        }

        ApproachValue(&work->scale, Q_8_8(0.1), work->scaleSteps);

        if (work->scaleSteps <= 0) {
            return 0;
        }

        work->stateTimer++;
        work->scaleSteps--;
        break;
    case SMN_MUSHU_STATE_FIRE_WINDUP:
        AnimChangeWithDef(sSmnMushuAnimDefs, &work->anim, 2, 0, work->tiles);

        if (AnimIsFinished(&work->anim)) {
            work->state = SMN_MUSHU_STATE_FIRE;
            work->stateTimer = 0;
        }

        break;
    case SMN_MUSHU_STATE_FIRE:
        if (work->stateTimer == 0) {
            AnimChangeWithDef(sSmnMushuAnimDefs, &work->anim, 3, ANIM_FLAG_LOOP, work->tiles);

            switch (work->variant) {
            case SUMMON_LEVEL_SINGLE:
                work->scaleSteps = 0x78;
                break;
            case SUMMON_LEVEL_PAIR:
                work->scaleSteps = 0xF0;
                break;
            case SUMMON_LEVEL_TRIPLE:
            default:
                work->scaleSteps = 0x1E0;
                break;
            }
        }

        if (AnimGetGfxIndex(&work->anim) == 5 && work->anim.timer == 0) {
            if (body->flags & BTLOBJ_FLAG_FACING_LEFT) {
                x = body->x - 0xC800;
            } else {
                x = body->x + 0xC800;
            }

            y = body->y;
            z = 0;

            switch (work->variant) {
            case SUMMON_LEVEL_SINGLE:
                attack = 0x9D;
                break;
            case SUMMON_LEVEL_PAIR:
                attack = 0x9E;
                break;
            case SUMMON_LEVEL_TRIPLE:
            default:
                attack = 0x9F;
                break;
            }

            MakeOpponentsHittable();
            m4aSongNumStart(SONG_EF_MU_FIRE);

            if (body->flags & BTLOBJ_FLAG_FACING_LEFT) {
                BgFxStartFire(SPELL_TIER_BASE, body->x - 0x3800, body->y, body->z - 0x800,
                              x, y, z, TRUE, attack);
            } else {
                BgFxStartFire(SPELL_TIER_BASE, body->x + 0x3800, body->y, body->z - 0x800,
                              x, y, z, FALSE, attack);
            }
        }

        BgAnimGetFrameState(&frame, &timer);

        if (frame <= 3) {
            BgFxAddPosition(body->x - prevX, body->y - prevY, body->z - prevZ);
        }

        if (work->stateTimer > work->scaleSteps) {
            work->state = SMN_MUSHU_STATE_VANISH;
            work->stateTimer = 0;
        } else {
            work->stateTimer++;
        }

        break;
    }

    if (work->animating) {
        AnimUpdate(&work->anim);
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

void task_smn_mushu_2(SmnMushuWork* work) {
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
               -4101 - ((body->y >> 8) * 4));
    TaskPoolDraw(&work->tasks);
}

void task_smn_mushu_3(SmnMushuWork* work) {
    BtlWork* owner;

    owner = work->mainSide ? gBtlWork : gRikuBtlWork;
    owner->flags &= ~BTL_FLAG_SUMMON_ACTIVE;
    ReleaseObjPalette(work->palette);
    TaskPoolDestroy(&work->tasks);
}
