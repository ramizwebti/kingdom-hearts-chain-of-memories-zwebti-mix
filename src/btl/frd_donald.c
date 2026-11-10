/**
 * frd_donald.c
 * Donald Duck Battle Ally
 */

#include "task_descriptors.h"
#include "system_state.h"
#include "display.h"
#include "frd.h"
#include "sprites_evt.h"
#include "sprites_frd.h"
#include "btl_api.h"
#include "frd_donald_api.h"
#include "songs.h"
#include "frd_tasks.h"
#include "anim.h"
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
#include "battle_ids.h"
#include "card_label_data.h"
#include "btl.h"

static const AnimDef sFrdDonaldAnimDefs[6] = {
    { gDonaBtLl00Frames, gDonaBtLl00Anims, gDonaBtLl00Tiles, 0 },
    { gDonaBtLl00Frames, gDonaBtLl00Anims, gDonaBtLl00Tiles, 1 },
    { gDonaBtLl00Frames, gDonaBtLl00Anims, gDonaBtLl00Tiles, 2 },
    { gDonaBtLl00Frames, gDonaBtLl00Anims, gDonaBtLl00Tiles, 3 },
    { gDonaFl00Frames, gDonaFl00Anims, gDonaFl00Tiles, 5 },
    { gDonaBl00Frames, gDonaBl00Anims, gDonaBl00Tiles, 2 },
};

TaskDesc gTaskDescFrdDonald = {
    "task_frd_donald",
    (TaskInitFunc)task_frd_donald_0,
    (TaskUpdateFunc)task_frd_donald_1,
    (TaskDrawFunc)task_frd_donald_2,
    (TaskDestroyFunc)task_frd_donald_3,
    sizeof(FrdDonaldWork),
};

u8 FrdDonaldApplyGravity(FrdDonaldWork* work) {
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

void UpdateDonaldFlame(BtlObj* body, u8 attacking, s16 dx, s16 dz) {
    s32 x;
    s32 y;
    s32 z;
    s32 scaleY;
    s32 scaleX;
    s16 halfX;
    s16 halfY;
    s16 halfZ;

    y = body->y;
    z = body->z - (dz * 256);
    scaleY = Q_8_8(1.5);

    if (body->flags & BTLOBJ_FLAG_FACING_LEFT) {
        x = body->x + (dx * 256);
        scaleX = Q_8_8(-1.5);
    } else {
        x = body->x - (dx * 256);
        scaleX = scaleY;
    }

    BgFxSetPosition(x, y, z);
    BgFxSetScale(scaleX, scaleY);

    if (attacking) {
        if (gBtlWork->battleId == BATTLE_PARASITE_CAGE) {
            halfX = 0x20;
            halfY = 0x20;
            halfZ = 0x30;
        } else {
            halfX = 0x0A;
            halfY = 0x0A;
            halfZ = 0x0A;
        }

        if (ApplyAttackBox(0x84, x, y, z, halfX, halfY, halfZ) != ATTACK_RESULT_NONE) {
            m4aSongNumStart(SONG_EF_FIRE01);
        }
    }
}

enum FrdDonaldState {
    FRD_DONALD_STATE_ENTER,
    FRD_DONALD_STATE_LAND,
    FRD_DONALD_STATE_ATTACK_END,
    FRD_DONALD_STATE_LEAVE,
    FRD_DONALD_STATE_CAST_FIRE,
    FRD_DONALD_STATE_CAST_BLIZZARD,
    FRD_DONALD_STATE_CAST_THUNDER,
    FRD_DONALD_STATE_CAST_CURE,
    FRD_DONALD_STATE_FLAME_RUN
};

void task_frd_donald_0(FrdDonaldWork* work, FrdArgs* args) {
    BtlObj* body;

    body = &work->body;

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
    work->state = FRD_DONALD_STATE_ENTER;
    work->stateTimer = 0;
    work->steps = 0;
    work->vz = 0;

    if (work->actor->flags & BTLOBJ_FLAG_FACING_LEFT) {
        work->unk_158 = work->actor->x - 0x3000;
        body->x = (gBtlWork->xMax + 0x30) << 8;
        body->flags = BTLOBJ_FLAG_FACING_LEFT;
    } else {
        work->unk_158 = work->actor->x + 0x3000;
        body->x = (gBtlWork->xMin - 0x30) << 8;
        body->flags = 0;
    }

    body->y = work->actor->y;
    body->z = -0x5000;
    body->groundZ = 0;
#ifdef PLATFORM_PC
    work->tiles = PortActionAssistSpawnTiles(args->mainSide, work->tiles);
#endif
    work->palette = LoadObjPalette(gDonaldPalette, sizeof(gDonaldPalette));
    AnimInit(&work->anim, NULL, NULL);
    AnimChangeWithDef(sFrdDonaldAnimDefs, &work->anim, 0, 0, work->tiles);

    switch (args->variant) {
    case SUMMON_LEVEL_SINGLE:
        work->repeatsLeft = 1;

#ifdef VERSION_EU
        if (gLanguage == LANGUAGE_ITALIAN) {
            m4aSongNumStart(SONG_VO_SR_SUMMON00);
        } else {
            m4aSongNumStart(SONG_VO_SR_SUMMON04);
        }
#else
        m4aSongNumStart(SONG_VO_SR_SUMMON04);
#endif
        break;
    case SUMMON_LEVEL_PAIR:
        work->repeatsLeft = 1;

#ifdef VERSION_EU
        if (gLanguage == LANGUAGE_ITALIAN) {
            m4aSongNumStart(SONG_VO_SR_SUMMON00);
        } else {
            m4aSongNumStart(SONG_VO_SR_SUMMON04);
        }
#else
        m4aSongNumStart(SONG_VO_SR_SUMMON04);
#endif
        break;
    case SUMMON_LEVEL_TRIPLE:
        work->repeatsLeft = 1;

#ifdef VERSION_EU
        if (gLanguage == LANGUAGE_ITALIAN) {
            m4aSongNumStart(SONG_VO_SR_SUMMON00);
        } else {
            m4aSongNumStart(SONG_VO_SR_SUMMON04);
        }
#else
        m4aSongNumStart(SONG_VO_SR_SUMMON04);
#endif
        break;
    default:
        m4aSongNumStart(SONG_VO_DL_ATTACK00);
        BgFxStartFlame(0, 0, 0, Q_8_8(1.5));
        UpdateDonaldFlame(body, FALSE, 8, 8);
        break;
    }

    TaskPoolInit(&work->tasks, 1);
    TaskCreate(&work->tasks, &gTaskDescBtlShadow, body);
}

u8 task_frd_donald_1(FrdDonaldWork* work) {
    BtlObj* body = &work->body;
    BtlWork* owner;
    BtlObj* target;
    s32 angle;

    if (work->mainSide) {
        owner = gBtlWork;
        target = owner->actor2;
    } else {
        owner = gRikuBtlWork;
        target = owner->actor2;
    }

    if (owner->flags & BTL_FLAG_DISMISS_SUMMONS) return 0;

    switch (work->state) {
    case FRD_DONALD_STATE_ENTER:
        if (work->stateTimer == 0) {
            AnimChangeWithDef(sFrdDonaldAnimDefs, &work->anim, 2, 0, work->tiles);
            work->stateTimer++;
        }

        body->x += (work->unk_158 - body->x) >> 4;
        ClampBattlePosition(&body->x, &body->y, -16, 0);

        if (work->variant == SUMMON_LEVEL_SLEIGHT) UpdateDonaldFlame(body, FALSE, 8, 8);

        if (FrdDonaldApplyGravity(work)) {
            work->stateTimer = 0;

            if (work->variant == SUMMON_LEVEL_SLEIGHT) work->state = FRD_DONALD_STATE_FLAME_RUN;
            else {
                work->state = FRD_DONALD_STATE_LAND;
                m4aSongNumStart(SONG_VO_DL_ATTACK00);
            }
        }

        break;
    case FRD_DONALD_STATE_LAND:
        if (work->stateTimer == 0) AnimChangeWithDef(sFrdDonaldAnimDefs, &work->anim, 3, 0, work->tiles);

        if (AnimIsFinished(&work->anim)) {
            SelectLockonTarget();

            if (gBtlWork->flags & BTL_FLAG_TUTORIAL) work->state = FRD_DONALD_STATE_CAST_FIRE;
            else {
                u16 spell = GetRandom();
                spell &= 3;

                switch (spell) {
                case 0:
                    work->state = FRD_DONALD_STATE_CAST_FIRE;
                    break;
                case 1:
                    work->state = FRD_DONALD_STATE_CAST_BLIZZARD;
                    break;
                case 2:
                    work->state = FRD_DONALD_STATE_CAST_THUNDER;
                    break;
                case 3:
                    work->state = FRD_DONALD_STATE_CAST_CURE;
                    break;
                }
            }

            work->stateTimer = 0;
        } else work->stateTimer++;

        break;
    case FRD_DONALD_STATE_ATTACK_END:
        if (work->repeatsLeft > 0) {
            SelectLockonTarget();

            if (gBtlWork->flags & BTL_FLAG_TUTORIAL) work->state = FRD_DONALD_STATE_CAST_FIRE;
            else {
                u16 spell = GetRandom();
                spell &= 3;

                switch (spell) {
                case 0:
                    work->state = FRD_DONALD_STATE_CAST_FIRE;
                    break;
                case 1:
                    work->state = FRD_DONALD_STATE_CAST_BLIZZARD;
                    break;
                case 2:
                    work->state = FRD_DONALD_STATE_CAST_THUNDER;
                    break;
                case 3:
                    work->state = FRD_DONALD_STATE_CAST_CURE;
                    break;
                }
            }

            work->stateTimer = 0;
            work->repeatsLeft--;
        } else {
            if (work->stateTimer == 0) AnimChangeWithDef(sFrdDonaldAnimDefs, &work->anim, 3, 0, work->tiles);

            if (AnimIsFinished(&work->anim)) {
                work->state = FRD_DONALD_STATE_LEAVE;
                work->stateTimer = 0;
            } else work->stateTimer++;
        }

        break;
    case FRD_DONALD_STATE_LEAVE:
        if (work->stateTimer == 0) {
            AnimChangeWithDef(sFrdDonaldAnimDefs, &work->anim, 2, 0, work->tiles);

            if (!(body->flags & BTLOBJ_FLAG_FACING_LEFT)) work->unk_158 = (gBtlWork->xMin - 64) * 256;
            else work->unk_158 = (gBtlWork->xMax + 64) * 256;

            work->vz = -0x500;
            work->steps = 30;
        }

        ApproachValue(&body->x, work->unk_158, work->steps);

        if (work->variant == SUMMON_LEVEL_SLEIGHT) UpdateDonaldFlame(body, FALSE, 8, 8);

        FrdDonaldApplyGravity(work);

        if (work->steps <= 0) {
            if (work->variant == SUMMON_LEVEL_SLEIGHT) BgAnimStop();

            return 0;
        }

        STEP_STATE(work);
        break;
    case FRD_DONALD_STATE_FLAME_RUN:
        if (work->stateTimer == 0) {
            if (body->flags & BTLOBJ_FLAG_FACING_LEFT) angle = GetRandom() % 2 ? 0xAD : 0xD3;
            else angle = GetRandom() % 2 ? 0x53 : 0x2D;

            work->unk_158 = gSineTable[angle] * 3;
            work->vy = -gSineTable[angle + 64] * 3;
        }

        if (work->vy > 0) AnimChangeWithDef(sFrdDonaldAnimDefs, &work->anim, 4, ANIM_FLAG_LOOP, work->tiles);
        else AnimChangeWithDef(sFrdDonaldAnimDefs, &work->anim, 5, ANIM_FLAG_LOOP, work->tiles);

        if (work->unk_158 < 0) body->flags |= BTLOBJ_FLAG_FACING_LEFT;
        else body->flags &= ~BTLOBJ_FLAG_FACING_LEFT;

        body->x += work->unk_158;
        body->y += work->vy;
        FrdDonaldApplyGravity(work);
        UpdateDonaldFlame(body, TRUE, 2, 8);

        switch (ClampBattlePosition(&body->x, &body->y, 0, 0)) {
        case BATTLE_EDGE_LEFT:
        case BATTLE_EDGE_RIGHT:
            work->unk_158 = -work->unk_158;
            break;
        case BATTLE_EDGE_TOP:
        case BATTLE_EDGE_BOTTOM:
            work->vy = -work->vy;
            break;
        }

        if (work->stateTimer > 179) {
            work->stateTimer = 0;
            work->state = FRD_DONALD_STATE_LEAVE;
        } else work->stateTimer++;

        break;
    case FRD_DONALD_STATE_CAST_FIRE:
        {
            s32 x,y,z;

            if (work->stateTimer == 0) {
                AnimChangeWithDef(sFrdDonaldAnimDefs, &work->anim, 0, 0, work->tiles);
                AnimReset(&work->anim);

                if (target != NULL) {
                    if (target->x < body->x) body->flags |= BTLOBJ_FLAG_FACING_LEFT;
                    else body->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
                }
            }

            if (work->stateTimer == 40) {
                if (target != NULL) {
                    x = target->x;
                    y = target->y;
                    z = target->z - target->centerHeight * 256;

                    if (x < body->x) body->flags |= BTLOBJ_FLAG_FACING_LEFT;
                    else body->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
                } else {
                    if (body->flags & BTLOBJ_FLAG_FACING_LEFT) x = body->x - 0xC800;
                    else x = body->x + 0xC800;

                    y = body->y;
                    z = body->z - 0x800;
                }

                switch (work->variant) {
                case SUMMON_LEVEL_SINGLE:
                    if (body->flags & BTLOBJ_FLAG_FACING_LEFT) BgFxStartFire(SPELL_TIER_BASE, body->x - 0x5000, body->y, body->z - 0x800, x, y, z, TRUE, 123);
                    else BgFxStartFire(SPELL_TIER_BASE, body->x + 0x5000, body->y, body->z - 0x800, x, y, z, FALSE, 123);

                    break;
                case SUMMON_LEVEL_PAIR:
                    if (body->flags & BTLOBJ_FLAG_FACING_LEFT) BgFxStartFire(SPELL_TIER_RA, body->x - 0x5000, body->y, body->z - 0x800, x, y, z, TRUE, 124);
                    else BgFxStartFire(SPELL_TIER_RA, body->x + 0x5000, body->y, body->z - 0x800, x, y, z, FALSE, 124);

                    break;
                case SUMMON_LEVEL_TRIPLE:
                default:
                    if (body->flags & BTLOBJ_FLAG_FACING_LEFT) BgFxStartFire(SPELL_TIER_GA, body->x - 0x5000, body->y, body->z - 0x800, x, y, z, TRUE, 125);
                    else BgFxStartFire(SPELL_TIER_GA, body->x + 0x5000, body->y, body->z - 0x800, x, y, z, FALSE, 125);

                    break;
                }
            }

            if (work->stateTimer > 40) {
                if (!BgFxIsActive()) {
                    work->state = FRD_DONALD_STATE_ATTACK_END;
                    work->stateTimer = 0;
                    break;
                }

                if (target != NULL) BgFxSetTarget(target->x, target->y, target->z - target->centerHeight * 256);
            }

            work->stateTimer++;
            break;
        }
    case FRD_DONALD_STATE_CAST_BLIZZARD:
        {
            s32 x,y,z;

            if (work->stateTimer == 0) {
                AnimChangeWithDef(sFrdDonaldAnimDefs, &work->anim, 0, 0, work->tiles);
                AnimReset(&work->anim);

                if (target != NULL) {
                    if (target->x < body->x) body->flags |= BTLOBJ_FLAG_FACING_LEFT;
                    else body->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
                }
            }

            if (work->stateTimer == 40) {
                if (target != NULL) {
                    x = target->x;
                    y = target->y;
                    z = target->z - target->centerHeight * 256;

                    if (x < body->x) body->flags |= BTLOBJ_FLAG_FACING_LEFT;
                    else body->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
                } else {
                    if (body->flags & BTLOBJ_FLAG_FACING_LEFT) x = body->x - 0x6400;
                    else x = body->x + 0x6400;

                    y = body->y;
                    z = body->z - 0x800;
                }

                switch (work->variant) {
                case SUMMON_LEVEL_SINGLE:
                    if (body->flags & BTLOBJ_FLAG_FACING_LEFT) BgFxStartBlizzard(SPELL_TIER_BASE, body->x - 0x5000, body->y, body->z - 0x800, x, y, z, TRUE, 126);
                    else BgFxStartBlizzard(SPELL_TIER_BASE, body->x + 0x5000, body->y, body->z - 0x800, x, y, z, FALSE, 126);

                    break;
                case SUMMON_LEVEL_PAIR:
                    if (body->flags & BTLOBJ_FLAG_FACING_LEFT) BgFxStartBlizzard(SPELL_TIER_RA, body->x - 0x5000, body->y, body->z - 0x800, x, y, z, TRUE, 127);
                    else BgFxStartBlizzard(SPELL_TIER_RA, body->x + 0x5000, body->y, body->z - 0x800, x, y, z, FALSE, 127);

                    break;
                case SUMMON_LEVEL_TRIPLE:
                default:
                    if (body->flags & BTLOBJ_FLAG_FACING_LEFT) BgFxStartBlizzard(SPELL_TIER_GA, body->x - 0x5000, body->y, body->z - 0x800, x, y, z, TRUE, 128);
                    else BgFxStartBlizzard(SPELL_TIER_GA, body->x + 0x5000, body->y, body->z - 0x800, x, y, z, FALSE, 128);

                    break;
                }
            }

            if (work->stateTimer > 40) {
                if (!BgFxIsActive()) {
                    work->state = FRD_DONALD_STATE_ATTACK_END;
                    work->stateTimer = 0;
                    break;
                }

                if (target != NULL) BgFxSetTarget(target->x, target->y, target->z - target->centerHeight * 256);
            }

            work->stateTimer++;
            break;
        }
    case FRD_DONALD_STATE_CAST_THUNDER:
        if (work->stateTimer == 0) {
            AnimChangeWithDef(sFrdDonaldAnimDefs, &work->anim, 1, 0, work->tiles);
            AnimReset(&work->anim);

            if (target != NULL) {
                if (target->x < body->x) body->flags |= BTLOBJ_FLAG_FACING_LEFT;
                else body->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
            }
        }

        if (work->stateTimer == 40) {
            switch (work->variant) {
            case SUMMON_LEVEL_SINGLE:
                {
                    s32 x,y,z;

                    if (target != NULL) {
                        x=target->x;
                        y=target->y;
                        z=target->groundZ;
                    } else {
                        if (body->flags & BTLOBJ_FLAG_FACING_LEFT) x=body->x-0x5000;
                        else x=body->x+0x5000;

                        y=body->y;
                        z=0;
                    }

                    BgFxStartThunder(SPELL_TIER_BASE, body->x, body->y, body->z-0x4000, x,y,z,129);
                    break;
                }
            case SUMMON_LEVEL_PAIR:
                BgFxStartWideThunder(SPELL_TIER_RA,body->x,body->y,body->z-0x4000,body->groundZ,130);
                break;
            case SUMMON_LEVEL_TRIPLE:
            default:
                BgFxStartWideThunder(SPELL_TIER_GA,body->x,body->y,body->z-0x4000,body->groundZ,131);
                break;
            }
        }

        if (work->stateTimer == 60) SetBattleZoom(15,148,0x10000,0x12C00);

        if (work->stateTimer > 40 && !BgFxIsActive()) {
            work->state=FRD_DONALD_STATE_ATTACK_END;
            SetBattleZoom(15,Q_8_8(1),gBtlWork->x2,gBtlWork->y2);
            work->stateTimer=0;
        } else work->stateTimer++;

        break;
    case FRD_DONALD_STATE_CAST_CURE:
        {
            BtlObj* ally=work->mainSide ? gBtlWork->actor : gRikuBtlWork->actor;

            if (work->stateTimer == 0) {
                AnimChangeWithDef(sFrdDonaldAnimDefs,&work->anim,1,0,work->tiles);
                AnimReset(&work->anim);

                if (ally->x < body->x) body->flags |= BTLOBJ_FLAG_FACING_LEFT;
                else body->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
            }

            if (work->stateTimer == 40) {
                switch (work->variant) {
                case SUMMON_LEVEL_SINGLE:
                    BgFxStartCure(SPELL_TIER_BASE,ally->x,ally->y,ally->z-0x2C00);
                    break;
                case SUMMON_LEVEL_PAIR:
                    BgFxStartCure(SPELL_TIER_RA,ally->x,ally->y,ally->z-0x2C00);
                    break;
                case SUMMON_LEVEL_TRIPLE:
                    BgFxStartCure(SPELL_TIER_GA,ally->x,ally->y,ally->z-0x2C00);
                    break;
                default:
                    BgFxStartCure(SPELL_TIER_BASE,ally->x,ally->y,ally->z-0x2C00);
                    break;
                }
            }

            if (work->stateTimer > 40) {
                if (BgFxIsActive()) {
                    BgFxSetPosition(ally->x,ally->y,ally->z-0x2C00);
                } else {
                    if (ally->btl->hcEffect == HC_EFFECT_CURE_BOOST) {
                        switch (work->variant) {
                        case SUMMON_LEVEL_SINGLE:
                            ally->hp+=75;
                            break;
                        case SUMMON_LEVEL_PAIR:
                            ally->hp+=225;
                            break;
                        case SUMMON_LEVEL_TRIPLE:
                            ally->hp+=450;
                            break;
                        }
                    } else {
                        switch (work->variant) {
                        case SUMMON_LEVEL_SINGLE:
                            ally->hp+=50;
                            break;
                        case SUMMON_LEVEL_PAIR:
                            ally->hp+=150;
                            break;
                        case SUMMON_LEVEL_TRIPLE:
                            ally->hp+=300;
                            break;
                        }
                    }

                    if (ally->hp > ally->maxHp) ally->hp=ally->maxHp;

                    CreateBtlPopTask(ally,BTL_POP_RECOVER);
                    work->state=FRD_DONALD_STATE_ATTACK_END;
                    SetBattleZoom(15,Q_8_8(1),gBtlWork->x2,gBtlWork->y2);
                    work->stateTimer=0;
                    break;
                }
            }

            work->stateTimer++;
            break;
        }
    }

    AnimUpdate(&work->anim);
    TaskPoolUpdate(&work->tasks);
    return 1;
}

void task_frd_donald_2(FrdDonaldWork* work) {
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

void task_frd_donald_3(FrdDonaldWork* work) {
    BtlWork* owner;

    owner = work->mainSide ? gBtlWork : gRikuBtlWork;
    owner->flags &= ~BTL_FLAG_SUMMON_ACTIVE;
    ReleaseObjPalette(work->palette);
    TaskPoolDestroy(&work->tasks);
}
