/**
 * smn_cloud.c
 * Cloud Summon
 */

#include "task_descriptors.h"
#include "smn.h"
#include "anim.h"
#include "sprites_cloud.h"
#include "btl_api.h"
#include "fade.h"
#include "songs.h"
#include "smn_cloud_api.h"
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

static const AnimDef sSmnCloudAnimDefs[8] = {
    { gCroud01Frames, gCroud01Anims, gCroud01Tiles, 0 },
    { gCroud02Frames, gCroud02Anims, gCroud02Tiles, 0 },
    { gCroud10Frames, gCroud10Anims, gCroud10Tiles, 0 },
    { gCroud11Frames, gCroud11Anims, gCroud11Tiles, 0 },
    { gCroud12Frames, gCroud12Anims, gCroud12Tiles, 0 },
    { gCroud13Frames, gCroud13Anims, gCroud13Tiles, 0 },
    { gCroud01Frames, gCroud01Anims, gCroud01Tiles, 1 },
    { gCroud02Frames, gCroud02Anims, gCroud02Tiles, 1 },
};

TaskDesc gTaskDescSmnCloud = {
    "task_smn_cloud",
    (TaskInitFunc)task_smn_cloud_0,
    (TaskUpdateFunc)task_smn_cloud_1,
    (TaskDrawFunc)task_smn_cloud_2,
    (TaskDestroyFunc)task_smn_cloud_3,
    sizeof(SmnCloudWork),
};

BtlObj* SmnCloudNextTarget(SmnCloudWork* work) {
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

BtlObj* SmnCloudPickTeleportTarget(SmnCloudWork* work) {
    BtlObj* list[10];
    BtlObj* obj;
    s16 count;
    s32 dz;

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
            dz = work->body.z - obj->z;

            if (dz >= 0 ? dz <= 0x3000 : obj->z - work->body.z <= 0x3000) {
                list[count] = obj;
                count++;

                if (count > 9) {
                    break;
                }
            }
        }

        obj = ListPoolNext(&obj->node);
    }

    if (count == 0) {
        return NULL;
    }

    obj = list[GetRandom() % count];
    return obj;
}

enum SmnCloudState {
    SMN_CLOUD_STATE_APPEAR,
    SMN_CLOUD_STATE_TRIPLE_SLASH,
    SMN_CLOUD_STATE_DOUBLE_SLASH,
    SMN_CLOUD_STATE_RISE,
    SMN_CLOUD_STATE_DIVE,
    SMN_CLOUD_STATE_VANISH,
    SMN_CLOUD_STATE_TELEPORT_OUT,
    SMN_CLOUD_STATE_TELEPORT_IN
};

enum SmnCloudAttackPhase {
    SMN_CLOUD_ATTACK_PHASE_OPENER,
    SMN_CLOUD_ATTACK_PHASE_FINISHER
};

void task_smn_cloud_0(SmnCloudWork* work, SmnArgs* args) {
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
        body->flags = BTLOBJ_FLAG_FACING_LEFT;
    } else {
        body->flags = 0;
    }

    work->variant = args->variant;
#ifdef PLATFORM_PC
    work->tiles = PortActionAssistSpawnTiles(args->mainSide, work->tiles);
#endif
    work->palette = LoadObjPalette(gCroudPalette, sizeof(gCroudPalette));
    work->vz = 0;
    AnimInit(&work->anim, NULL, NULL);
    AnimChangeWithDef(sSmnCloudAnimDefs, &work->anim, 0, 0, work->tiles);
    work->state = SMN_CLOUD_STATE_APPEAR;
    work->stateTimer = 0;
    work->speed = 0;
    work->unk_158 = 0;
    work->scaleX = 10;
    work->scaleY = 10;
    work->animating = FALSE;
    work->attackPhase = SMN_CLOUD_ATTACK_PHASE_OPENER;
    work->target = NULL;
    work->attackCount = 0;
    work->targetIndex = 0;
    TaskPoolInit(&work->tasks, 2);
    TaskCreate(&work->tasks, &gTaskDescBtlShadow, body);
}

u8 task_smn_cloud_1(SmnCloudWork* work) {
    BtlObj* body = &work->body;
    BtlWork* owner;
    s32 x;
    s32 dz;
    s32 targetZ;
    s32 pixelX;
    owner = work->mainSide ? gBtlWork : gRikuBtlWork;

    if (owner->flags & BTL_FLAG_DISMISS_SUMMONS) return 0;

    if (work->state == SMN_CLOUD_STATE_DIVE) BtlMapFollowPosition(body->x, body->y, body->z + 0x2000);
    else BtlMapFollowPosition(body->x, body->y, body->z);

    switch (work->state) {
    case SMN_CLOUD_STATE_APPEAR:
        if (work->stateTimer == 0) {
            work->steps = 12;
            BgFxStartSummon(body->x, body->y, body->z);
            m4aSongNumStart(SONG_EF_SUMMON_UP);
        }

        ApproachValue(&work->scaleX, Q_8_8(1), work->steps);
        work->scaleY = work->scaleX;

        if (work->steps <= 0) {
            work->stateTimer = 0;

            switch (work->variant) {
            case SUMMON_LEVEL_SINGLE:
                work->state = SMN_CLOUD_STATE_DOUBLE_SLASH;
                work->animating = TRUE;
                break;
            case SUMMON_LEVEL_PAIR:
                work->state = SMN_CLOUD_STATE_TRIPLE_SLASH;
                work->animating = TRUE;
                break;
            case SUMMON_LEVEL_TRIPLE:
                work->state = SMN_CLOUD_STATE_RISE;
                work->animating = TRUE;
                break;
            default:
                work->state = SMN_CLOUD_STATE_TELEPORT_OUT;
                break;
            }
        } else {
            STEP_STATE(work);
        }

        break;
    case SMN_CLOUD_STATE_VANISH:
        if (work->stateTimer == 0) {
            work->steps = 30;
            BgFxStartSummon(body->x, body->y, body->z);
            m4aSongNumStart(SONG_EF_SUMMON_DOWN);
        }

        ApproachValue(&work->scaleX, Q_8_8(0.1), work->steps);
        work->scaleY = work->scaleX;

        if (work->steps <= 0) return 0;

        STEP_STATE(work);
        break;
    case SMN_CLOUD_STATE_TELEPORT_OUT:
        if (work->stateTimer == 0) work->steps = 8;

        ApproachValue(&work->scaleX, 10, work->steps);
        ApproachValue(&work->scaleY, Q_8_8(2), work->steps);

        if (--work->steps <= 0) {
            work->state = SMN_CLOUD_STATE_TELEPORT_IN;
            work->stateTimer = 0;
            m4aSongNumStart(SONG_EF_TELEP);
        } else work->stateTimer++;

        break;
    case SMN_CLOUD_STATE_TELEPORT_IN: {
        BtlObj* target;

        if (work->stateTimer == 0) {
            target = SmnCloudPickTeleportTarget(work);
            work->steps = 8;

            if (target != NULL) {
                body->y = target->y;
                body->z = target->groundZ;
                body->groundZ = target->groundZ;

                if (target->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    body->x = target->x + 0x2000;
                    body->flags |= BTLOBJ_FLAG_FACING_LEFT;
                } else {
                    body->x = target->x - 0x2000;
                    body->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
                }
            }
        }

        ApproachValue(&work->scaleX, Q_8_8(1), work->steps);
        ApproachValue(&work->scaleY, Q_8_8(1), work->steps);

        if (--work->steps <= 0) {
            work->animating = TRUE;
            work->state = SMN_CLOUD_STATE_TRIPLE_SLASH;
            work->stateTimer = 0;
        } else work->stateTimer++;

        break;
    }
    case SMN_CLOUD_STATE_TRIPLE_SLASH:
        if (work->stateTimer == 0) {
            AnimChangeWithDef(sSmnCloudAnimDefs, &work->anim, 0, 0, work->tiles);
        } else if ((s16)work->attackPhase == SMN_CLOUD_ATTACK_PHASE_OPENER && AnimIsFinished(&work->anim)) {
            AnimChangeWithDef(sSmnCloudAnimDefs, &work->anim, 1, 0, work->tiles);
            work->attackPhase++;
        } else if (AnimIsFinished(&work->anim)) {
            work->state = SMN_CLOUD_STATE_VANISH;
            work->stateTimer = 0;
            break;
        }

        if (work->anim.timer == 0) {
            if ((s16)work->attackPhase == SMN_CLOUD_ATTACK_PHASE_OPENER) {
                switch (AnimGetFrame(&work->anim)) {
                case 2:
                    m4aSongNumStart(SONG_VO_KU_ATTACK00);
                    break;
                case 6:
                    if (body->flags & BTLOBJ_FLAG_FACING_LEFT ? ApplyAttackBox(151, body->x - 0x2800, body->y, body->z, 24, 24, 48) : ApplyAttackBox(151, body->x + 0x2800, body->y, body->z, 24, 24, 48)) {
                        m4aSongNumStart(SONG_EF_KU_ATT00);
                        FadeStartIn(FADE_MODE_ADD_WHITE, 20);

                        if (body->flags & BTLOBJ_FLAG_FACING_LEFT) {
                            SetBattleZoom(6, Q_8_8(1.2), body->x - 0x2000, body->y - 0x1800 + body->z);
                        } else {
                            SetBattleZoom(6, Q_8_8(1.2), body->x + 0x2000, body->y - 0x1800 + body->z);
                        }
                    }

                    break;
                case 7:
                    SetBattleZoom(6, Q_8_8(1), gBtlWork->x2, gBtlWork->y2);
                    break;
                case 9:
                    m4aSongNumStart(SONG_VO_KU_ATTACK01);
                    break;
                }
            } else {
                switch (AnimGetFrame(&work->anim)) {
                case 0:
                    MakeOpponentsHittable();

                    if (body->flags & BTLOBJ_FLAG_FACING_LEFT ? ApplyAttackBox(151, body->x - 0x2800, body->y, body->z, 24, 24, 48) : ApplyAttackBox(151, body->x + 0x2800, body->y, body->z, 24, 24, 48)) {
                        m4aSongNumStart(SONG_EF_KU_ATT01);
                        FadeStartIn(FADE_MODE_ADD_WHITE, 20);

                        if (body->flags & BTLOBJ_FLAG_FACING_LEFT) {
                            SetBattleZoom(6, Q_8_8(1.2), body->x - 0x2000, body->y - 0x1800 + body->z);
                        } else {
                            SetBattleZoom(6, Q_8_8(1.2), body->x + 0x2000, body->y - 0x1800 + body->z);
                        }
                    }

                    break;
                case 1:
                    SetBattleZoom(6, Q_8_8(1), gBtlWork->x2, gBtlWork->y2);
                    break;
                case 4:
                    m4aSongNumStart(SONG_VO_KU_ATTACK02);
                    break;
                case 5:
                    MakeOpponentsHittable();

                    if (body->flags & BTLOBJ_FLAG_FACING_LEFT ? ApplyAttackBox(151, body->x - 0x2800, body->y, body->z, 24, 24, 48) : ApplyAttackBox(151, body->x + 0x2800, body->y, body->z, 24, 24, 48)) {
                        m4aSongNumStart(SONG_EF_KU_ATT02);
                        FadeStartIn(FADE_MODE_ADD_WHITE, 50);

                        if (body->flags & BTLOBJ_FLAG_FACING_LEFT) {
                            SetBattleZoom(6, Q_8_8(2), body->x - 0x2000, body->y - 0x1800 + body->z);
                        } else {
                            SetBattleZoom(6, Q_8_8(2), body->x + 0x2000, body->y - 0x1800 + body->z);
                        }
                    }

                    break;
                case 6:
                    SetBattleZoom(6, Q_8_8(1), gBtlWork->x2, gBtlWork->y2);
                    break;
                }
            }
        }

        work->stateTimer++;
        break;
    case SMN_CLOUD_STATE_DOUBLE_SLASH:
        if (work->stateTimer == 0) {
            AnimChangeWithDef(sSmnCloudAnimDefs, &work->anim, 6, 0, work->tiles);
        } else if ((s16)work->attackPhase == SMN_CLOUD_ATTACK_PHASE_OPENER && AnimIsFinished(&work->anim)) {
            AnimChangeWithDef(sSmnCloudAnimDefs, &work->anim, 7, 0, work->tiles);
            work->attackPhase++;
        } else if (AnimIsFinished(&work->anim)) {
            work->state = SMN_CLOUD_STATE_VANISH;
            work->stateTimer = 0;
            break;
        }

        if (work->anim.timer == 0) {
            if ((s16)work->attackPhase == SMN_CLOUD_ATTACK_PHASE_OPENER) {
                switch (AnimGetFrame(&work->anim)) {
                case 2:
                    m4aSongNumStart(SONG_VO_KU_ATTACK00);
                    break;
                case 6:
                    if (body->flags & BTLOBJ_FLAG_FACING_LEFT ? ApplyAttackBox(151, body->x - 0x2800, body->y, body->z, 24, 24, 48) : ApplyAttackBox(151, body->x + 0x2800, body->y, body->z, 24, 24, 48)) {
                        m4aSongNumStart(SONG_EF_KU_ATT00);
                        FadeStartIn(FADE_MODE_ADD_WHITE, 20);

                        if (body->flags & BTLOBJ_FLAG_FACING_LEFT) {
                            SetBattleZoom(6, Q_8_8(1.2), body->x - 0x2000, body->y - 0x1800 + body->z);
                        } else {
                            SetBattleZoom(6, Q_8_8(1.2), body->x + 0x2000, body->y - 0x1800 + body->z);
                        }
                    }

                    break;
                case 7:
                    SetBattleZoom(6, Q_8_8(1), gBtlWork->x2, gBtlWork->y2);
                    break;
                case 9:
                    m4aSongNumStart(SONG_VO_KU_ATTACK01);
                    break;
                }
            } else {
                switch (AnimGetFrame(&work->anim)) {
                case 0:
                    MakeOpponentsHittable();

                    if (body->flags & BTLOBJ_FLAG_FACING_LEFT ? ApplyAttackBox(151, body->x - 0x2800, body->y, body->z, 24, 24, 48) : ApplyAttackBox(151, body->x + 0x2800, body->y, body->z, 24, 24, 48)) {
                        m4aSongNumStart(SONG_EF_KU_ATT01);
                        FadeStartIn(FADE_MODE_ADD_WHITE, 20);

                        if (body->flags & BTLOBJ_FLAG_FACING_LEFT) {
                            SetBattleZoom(6, Q_8_8(1.2), body->x - 0x2000, body->y - 0x1800 + body->z);
                        } else {
                            SetBattleZoom(6, Q_8_8(1.2), body->x + 0x2000, body->y - 0x1800 + body->z);
                        }
                    }

                    break;
                case 1:
                    SetBattleZoom(6, Q_8_8(1), gBtlWork->x2, gBtlWork->y2);
                    break;
                }
            }
        }

        work->stateTimer++;
        break;
    case SMN_CLOUD_STATE_RISE:
        if (work->stateTimer == 0) {
            AnimChangeWithDef(sSmnCloudAnimDefs, &work->anim, 2, ANIM_FLAG_LOOP, work->tiles);
            work->speed = 0;
        }

        if (body->flags & BTLOBJ_FLAG_FACING_LEFT) pixelX = gBtlWork->xMin + 50;
        else pixelX = gBtlWork->xMax - 50;

        x = pixelX * 256;
        targetZ = -0xC800;
        body->x += (x - body->x) >> 4;
        dz = (targetZ - body->z) >> 3;

        if (dz > work->speed) dz = work->speed;

        if (dz < -work->speed) dz = -work->speed;

        body->z += dz;
        work->speed += 128;

        if ((body->z - targetZ >= 0 ? body->z - targetZ : targetZ - body->z) < 0x1000) {
            work->state = SMN_CLOUD_STATE_DIVE;
            work->stateTimer = 0;
        } else work->stateTimer++;

        break;
    case SMN_CLOUD_STATE_DIVE: {
        BtlObj* target;

        if (work->stateTimer == 0) {
            target = SmnCloudNextTarget(work);
            work->target = target;

            if (target == NULL) {
                work->state = SMN_CLOUD_STATE_VANISH;
                work->stateTimer = 0;
                break;
            }

            work->targetX = target->x;
            work->targetY = target->y;
            work->targetZ = target->z - 0x1000;
            MakeOpponentsHittable();

            switch ((s16)work->attackCount) {
            case 0:
                m4aSongNumStart(SONG_VO_KU_ATTACK00);
                AnimChangeWithDef(sSmnCloudAnimDefs, &work->anim, 3, 0, work->tiles);
                break;
            case 1:
                m4aSongNumStart(SONG_VO_KU_ATTACK01);
                AnimChangeWithDef(sSmnCloudAnimDefs, &work->anim, 4, 0, work->tiles);
                break;
            case 2:
            default:
                m4aSongNumStart(SONG_VO_KU_ATTACK02);
                AnimChangeWithDef(sSmnCloudAnimDefs, &work->anim, 5, 0, work->tiles);
                break;
            }

            if (body->x < work->targetX) body->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
            else body->flags |= BTLOBJ_FLAG_FACING_LEFT;
        }

        body->x += (work->targetX - body->x) >> 3;
        body->y += (work->targetY - body->y) >> 3;
        body->z += (work->targetZ - body->z) >> 3;

        if (work->anim.timer == 0) {
            switch ((s16)work->attackCount) {
            case 0:
                if (AnimGetFrame(&work->anim) == 4) {
                    if (body->flags & BTLOBJ_FLAG_FACING_LEFT ? ApplyAttackBox(152, body->x - 0x1800, body->y, body->z, 40, 24, 48) : ApplyAttackBox(152, body->x + 0x1800, body->y, body->z, 40, 24, 48)) {
                        m4aSongNumStart(SONG_EF_KU_ATT00);
                        FadeStartIn(FADE_MODE_ADD_WHITE, 20);
                    }
                }

                break;
            case 1:
                if (AnimGetFrame(&work->anim) == 3) {
                    MakeOpponentsHittable();

                    if (body->flags & BTLOBJ_FLAG_FACING_LEFT ? ApplyAttackBox(152, body->x - 0x1800, body->y, body->z, 40, 24, 48) : ApplyAttackBox(152, body->x + 0x1800, body->y, body->z, 40, 24, 48)) {
                        m4aSongNumStart(SONG_EF_KU_ATT01);
                        FadeStartIn(FADE_MODE_ADD_WHITE, 20);
                    }
                }

                break;
            case 2:
            default:
                if (AnimGetFrame(&work->anim) == 3) {
                    MakeOpponentsHittable();

                    if (body->flags & BTLOBJ_FLAG_FACING_LEFT ? ApplyAttackBox(152, body->x - 0x1800, body->y, body->z, 40, 24, 48) : ApplyAttackBox(152, body->x + 0x1800, body->y, body->z, 40, 24, 48)) {
                        m4aSongNumStart(SONG_EF_KU_ATT02);
                        FadeStartIn(FADE_MODE_ADD_WHITE, 20);
                    }
                }

                break;
            }
        }

        if (work->stateTimer > 23 && AnimIsFinished(&work->anim)) {
            work->stateTimer = 0;

            if ((s16)++work->attackCount > 2) work->state = SMN_CLOUD_STATE_VANISH;
            else work->state = SMN_CLOUD_STATE_RISE;
        } else work->stateTimer++;

        break;
    }
    }

    body->groundZ = 0;
    ApplyBattleBounds(&body->x, &body->y, &body->z, &body->groundZ);

    if (body->z > body->groundZ) body->z = body->groundZ;

    if (work->animating) AnimUpdate(&work->anim);

    TaskPoolUpdate(&work->tasks);
    return 1;
}

void task_smn_cloud_2(SmnCloudWork* work) {
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
    WorldToScreen(&sx, &sy, body->x, body->y, body->z);

    if (work->scaleX == Q_8_8(1) && work->scaleY == work->scaleX) {
        if (body->flags & BTLOBJ_FLAG_FACING_LEFT) {
            sclY = gBtlWork->scale;
            sclX = sclY;
        } else if (gBtlWork->scale == work->scaleY) {
            sclY = gBtlWork->scale;
            sclX = sclY;
            flags |= SPRITE_FLAG_HFLIP;
        } else {
            sclY = gBtlWork->scale;
            sclX = -sclY;
        }
    } else if (body->flags & BTLOBJ_FLAG_FACING_LEFT) {
        sclX = gBtlWork->scale * work->scaleX >> 8;
        sclY = gBtlWork->scale * work->scaleY >> 8;
    } else {
        sclX = -(gBtlWork->scale * work->scaleX >> 8);
        sclY = gBtlWork->scale * work->scaleY >> 8;
    }

    if (sclY == Q_8_8(1) && sclX == sclY) {
        affine = NULL;
    } else if (sclY <= 255) {
        affine = AllocObjAffine(0, sclX, sclY, FALSE);
    } else {
        affine = AllocObjAffine(0, sclX, sclY, TRUE);
    }

    DrawSprite(sx, sy, gfx, work->tiles, work->palette, affine, flags,
               -4100 - ((body->y >> 8) * 4));
    body->shadowPriority = (-4100 - ((body->y >> 8) * 4)) | 2;
    TaskPoolDraw(&work->tasks);
}

void task_smn_cloud_3(SmnCloudWork* work) {
    BtlWork* owner;

    owner = work->mainSide ? gBtlWork : gRikuBtlWork;
    owner->flags &= ~BTL_FLAG_SUMMON_ACTIVE;
    ReleaseObjPalette(work->palette);
    TaskPoolDestroy(&work->tasks);
}
