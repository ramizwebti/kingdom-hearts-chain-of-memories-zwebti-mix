/**
 * mode_battle.c
 * Battle Mode
 */

#ifdef PLATFORM_PC
#include "engine.h"
#include "port/port_practice.h"
#include "port/port_action_game.h"
#include "key.h"
#include "gba/keys.h"
#include "battle.h"
#endif
#include "malloc.h"
#include "fade.h"
#include "songs.h"
#include "battle_actor.h"
#include "battle_bounds.h"
#include "battle_work.h"
#include "btl_collision.h"
#include "btl_effect.h"
#include "display.h"
#include "formation_data.h"
#include "game_state.h"
#include "gba/macro.h"
#include "m4a_song.h"
#include "mode.h"
#include "mode_chkbtl_api.h"
#include "registration_data.h"
#include <stddef.h>
#include "system_state.h"
#include "task_descriptors.h"
#include "taskpool.h"
#include "types.h"
#include "battle_ids.h"
#include "gba/io_reg.h"
#include "macros.h"

void mode_battle_0(u32 mode) {
#ifdef PLATFORM_PC
    PortSetBattleView(1); /* the stage repeats sideways in the wide margins */
#endif
    BtlWork** dest;

    gBtlWork = EwramAlloc(sizeof(BtlWork));
    gRikuBtlWork = NULL;
    BtlWorkInit();
    AllocBattleTiles();
    gBtlWork->battleId = mode;
    gGameState.flags &= ~GAME_FLAG_BATTLE_NOT_WON;

    switch (mode) {
    case BATTLE_GUARD_ARMOR ... BATTLE_MARLUXIA_2:
        gBtlWork->flags |= BTL_FLAG_BOSS_BATTLE;
        break;
    case BATTLE_TUTORIAL_0 ... BATTLE_TUTORIAL_6:
        gBtlWork->flags |= BTL_FLAG_TUTORIAL;
    case BATTLE_LEON ... BATTLE_ANSEM_2:
        gBtlWork->flags |= BTL_FLAG_HUM_BATTLE;
        dest = &gRikuBtlWork;
        *dest = EwramAlloc(sizeof(BtlWork));
        CpuFill32(0, gRikuBtlWork, sizeof(BtlWork));
        break;
    }

    if (gBtlWork->flags & BTL_FLAG_BOSS_BATTLE) {
        SetBgMode1();

        switch (mode) {
        case BATTLE_MARLUXIA_2:
            m4aSongNumStart(SONG_BGM_LASTBOSS2);
            break;
        case BATTLE_JAFAR:
        case BATTLE_URSULA:
        case BATTLE_DRAGON_MALEFICENT:
        case BATTLE_OOGIE_BOOGIE:
            m4aSongNumStart(SONG_BGM_BOSSWORLD);
            break;
        case BATTLE_GUARD_ARMOR:
        case BATTLE_TRICKMASTER:
        case BATTLE_PARASITE_CAGE:
        case BATTLE_DARKSIDE:
        default:
            m4aSongNumStart(SONG_BGM_BOSS1_WORLD);
            break;
        }

        gBtlWork->bg = 2;

        switch (mode) {
        case BATTLE_PARASITE_CAGE:
        case BATTLE_MARLUXIA_2:
            SetupBg(0, 0, 22, 0);
            SetupBg(1, 0, 24, 0);
            SetupBg(2, 2, 28, 10);
            SetBgPriority(0, 2);
            SetBgPriority(2, 0);
            SetBgPriority(1, 1);
            SetBgOverflow(2, FALSE);
            SetBgSize(1, BGCNT_TXT512x256);
            break;
        case BATTLE_GUARD_ARMOR:
        case BATTLE_JAFAR:
        case BATTLE_TRICKMASTER:
        case BATTLE_URSULA:
        case BATTLE_DRAGON_MALEFICENT:
        case BATTLE_DARKSIDE:
        case BATTLE_OOGIE_BOOGIE:
        default:
            SetupBg(0, 0, 24, 0);
            SetupBg(1, 0, 26, 0);
            SetupBg(2, 2, 28, 10);
            SetBgPriority(0, 2);
            SetBgPriority(2, 0);
            SetBgPriority(1, 1);
            SetBgOverflow(2, FALSE);
            break;
        }
    } else if (gBtlWork->flags & BTL_FLAG_TUTORIAL) {
        m4aSongNumStart(SONG_BGM_EVENT2);
        gBtlWork->bg = 3;
        gBtlWork->mapBg = 2;
        SetBgMode2();
        SetupBg(gBtlWork->mapBg, 0, 12, 0);
        SetupBg(gBtlWork->bg, 2, 28, 10);
        SetBgPriority(gBtlWork->mapBg, 2);
        SetBgPriority(gBtlWork->bg, 0);
        SetBgOverflow(gBtlWork->mapBg, TRUE);
        SetBgOverflow(gBtlWork->bg, FALSE);
    } else if (gBtlWork->flags & BTL_FLAG_HUM_BATTLE) {
        gBtlWork->bg = 2;
        gBtlWork->mapBg = 3;

        switch (mode) {
        case BATTLE_RIKU_1:
        case BATTLE_RIKU_2 ... BATTLE_RIKU_6:
            m4aSongNumStart(SONG_BGM_NISERIKU);
            break;
        case BATTLE_LEON:
        case BATTLE_CLOUD:
            m4aSongNumStart(SONG_BGM_EVENT2);
            break;
        case BATTLE_MARLUXIA:
            m4aSongNumStart(SONG_BGM_LASTBOSS1);
            break;
        case BATTLE_HOOK:
        case BATTLE_HADES:
            m4aSongNumStart(SONG_BGM_BOSSWORLD);
            break;
        case BATTLE_ANSEM_2:
            m4aSongNumStart(SONG_BGM_RIKU_ANSEM);
            break;
        default:
            m4aSongNumStart(SONG_BGM_BOSS3_XIII);
            break;
        }

        SetBgMode2();
        SetupBg(3, 0, 12, 0);
        SetupBg(2, 2, 28, 10);
        SetBgPriority(3, 2);
        SetBgPriority(2, 0);
        SetBgOverflow(3, TRUE);
        SetBgOverflow(2, FALSE);
    } else {
        gBtlWork->bg = 2;
        gBtlWork->mapBg = 3;

        switch (gGameState.battleStage) {
        case BATTLE_STAGE_WONDERLAND:
        case BATTLE_STAGE_GARDEN:
            m4aSongNumStart(SONG_BGM_ALICE_BTL);
            break;
        case BATTLE_STAGE_AGRABAH:
            m4aSongNumStart(SONG_BGM_ALADDIN_BATTLE);
            break;
        case BATTLE_STAGE_ATLANTICA:
            m4aSongNumStart(SONG_BGM_MARMAID_BATTLE);
            break;
        case BATTLE_STAGE_MONSTRO:
            if (mode == BATTLE_SHADOW_100) {
                m4aSongNumStart(SONG_BGM_EVENT2);
            } else {
                m4aSongNumStart(SONG_BGM_PINOCCHIO_BTL);
            }

            break;
        case BATTLE_STAGE_OLYMPUS_COLISEUM:
            m4aSongNumStart(SONG_BGM_HERCULES_BATTLE);
            break;
        case BATTLE_STAGE_HALLOWEEN_TOWN:
            m4aSongNumStart(SONG_BGM_HALLOWEEN_BTL);
            break;
        case BATTLE_STAGE_NEVER_LAND:
            m4aSongNumStart(SONG_BGM_PETERPAN_BTL);
            break;
        case BATTLE_STAGE_HOLLOW_BASTION:
            m4aSongNumStart(SONG_BGM_HOLLOW_BATTLE);
            break;
        case BATTLE_STAGE_TRAVERSE_TOWN:
            m4aSongNumStart(SONG_BGM_TOWN_BTL);
            break;
        case BATTLE_STAGE_DESTINY_ISLANDS:
            m4aSongNumStart(SONG_BGM_DESTINY_BATTLE);
            break;
        case BATTLE_STAGE_CASTLE_OBLIVION:
            m4aSongNumStart(SONG_BGM_F13F_FORGET_BATTLE);
            break;
        case BATTLE_STAGE_TWILIGHT_TOWN:
            m4aSongNumStart(SONG_BGM_TWILIGHT_BATTLE);
            break;
        }

        SetBgMode2();
        SetupBg(3, 0, 12, 0);
        SetupBg(2, 2, 28, 10);
        SetBgPriority(3, 2);
        SetBgPriority(2, 0);
        SetBgOverflow(3, TRUE);
        SetBgOverflow(2, FALSE);
    }

    TaskPoolInit(&gBtlWork->taskPools[0], 40);
    TaskPoolInit(&gBtlWork->taskPools[1], 32);
    TaskPoolInit(&gBtlWork->taskPools[2], 1);
    BgFxInit(BGCNT_256COLOR, gBtlWork->bg);
    ColliderPoolsInit();

    if (gGameState.flags & GAME_FLAG_RIKU) {
        TaskCreate(&gBtlWork->taskPools[0], &gTaskDescBtlRiku, NULL);
    } else {
        TaskCreate(&gBtlWork->taskPools[0], &gTaskDescBtlSora, NULL);
    }

    if (gBtlWork->flags & BTL_FLAG_BOSS_BATTLE) {
        gBtlWork->flags |= BTL_FLAG_NO_ENEMY_DROPS;

        switch (mode) {
        case BATTLE_DARKSIDE:
            SetBattleBounds(0, 0x100, 0x148, 0x1A8);
            TaskCreate(&gBtlWork->taskPools[0], &gTaskDescBosDsd, NULL);
            gBtlWork->fadeAmount = 10;
            break;
        case BATTLE_GUARD_ARMOR:
            SetBattleBounds(-32, 0x120, 0x120, 0x180);
            TaskCreate(&gBtlWork->taskPools[0], &gTaskDescBosGa, NULL);
            gBtlWork->fadeAmount = 5;
            break;
        case BATTLE_DRAGON_MALEFICENT:
#ifdef VERSION_EU
            SetBattleBounds(0, 0xE0, 0x130, 0x180);
#else
            SetBattleBounds(0, 0xE0, 0x118, 0x180);
#endif
            TaskCreate(&gBtlWork->taskPools[0], &gTaskDescBosMd, NULL);
            gBtlWork->fadeAmount = 5;
            break;
        case BATTLE_TRICKMASTER:
            SetBattleBounds(0x80, 0x180, 0x140, 0x180);
            TaskCreate(&gBtlWork->taskPools[0], &gTaskDescBosTm, NULL);
            gBtlWork->fadeAmount = 12;
            break;
        case BATTLE_URSULA:
            SetBattleBounds(0, 0x200, 0, 0x200);
            TaskCreate(&gBtlWork->taskPools[0], &gTaskDescBosUrsula, NULL);
            gBtlWork->fadeAmount = 5;
            break;
        case BATTLE_PARASITE_CAGE:
            SetBattleBounds(0x80, 0x1A8, 0x126, 0x180);
            TaskCreate(&gBtlWork->taskPools[0], &gTaskDescBosPc, NULL);
            TaskCreate(&gBtlWork->taskPools[0], &gTaskDescPcAcddmg, gBtlWork->actor);
            gBtlWork->fadeAmount = 12;
            break;
        case BATTLE_MARLUXIA_2:
            SetBattleBounds(0x80, 0x170, 0x1E0, 0x200);
            TaskCreate(&gBtlWork->taskPools[0], &gTaskDescBosLst, NULL);
            gBtlWork->fadeAmount = 10;
            break;
        case BATTLE_OOGIE_BOOGIE:
            SetBattleBounds(0x80, 0x170, 0x228, 0x278);
            TaskCreate(&gBtlWork->taskPools[0], &gTaskDescBosBoogie, NULL);
            gBtlWork->boundsCallback = &ClampBosBoogieBounds;
            gBtlWork->fadeAmount = 5;
            break;
        default:
            SetBattleBounds(0x1A4, 0x264, 0x148, 0x180);
            TaskCreate(&gBtlWork->taskPools[0], &gTaskDescBosJf, NULL);
            gBtlWork->boundsCallback = &ClampBosJfBounds;
            gBtlWork->fadeAmount = 10;
            break;
        }
    } else if (gBtlWork->flags & BTL_FLAG_TUTORIAL) {
        SetBattleBounds(0x68, 0x198, 0x160, 0x1A2);

        if (mode == BATTLE_TUTORIAL_0) {
            TaskCreate(&gBtlWork->taskPools[0], &gTaskDescHumRobe, NULL);
            TaskCreate(&gBtlWork->taskPools[1], &gTaskDescTutorial, NULL);
        } else {
            TaskCreate(&gBtlWork->taskPools[0], &gTaskDescHumLeon, NULL);
            TaskCreate(&gBtlWork->taskPools[1], &gTaskDescTutorial, (void*)1);
        }

        gGameState.battleStage = BATTLE_STAGE_TRAVERSE_TOWN;
        TaskCreate(&gBtlWork->taskPools[1], &gTaskDescBtlMap, NULL);
    } else if (gBtlWork->flags & BTL_FLAG_HUM_BATTLE) {
        gBtlWork->flags |= BTL_FLAG_NO_ENEMY_DROPS;

        switch (mode) {
        case BATTLE_HOOK:
            TaskCreate(&gBtlWork->taskPools[0], &gTaskDescHumHook, NULL);
            SetBattleBounds(0x68, 0x198, 0x160, 0x1A2);
            break;
        case BATTLE_ANSEM_1:
        case BATTLE_ANSEM_2:
            TaskCreate(&gBtlWork->taskPools[0], &gTaskDescHumAnsem, NULL);
            SetBattleBounds(0x50, 0x1B0, 0x160, 0x1A2);
            break;
        case BATTLE_CLOUD:
            TaskCreate(&gBtlWork->taskPools[0], &gTaskDescHumCloud, NULL);
            SetBattleBounds(0x50, 0x1B0, 0x160, 0x1A2);
            break;
        case BATTLE_HADES:
            TaskCreate(&gBtlWork->taskPools[0], &gTaskDescHumHades, NULL);
            SetBattleBounds(0x50, 0x1B0, 0x160, 0x1A2);
            break;
        case BATTLE_MARLUXIA:
            TaskCreate(&gBtlWork->taskPools[0], &gTaskDescHumMahluxia, NULL);
            SetBattleBounds(0x50, 0x1B0, 0x160, 0x1A2);
            break;
        case BATTLE_LARXENE_1:
        case BATTLE_LARXENE_2:
            TaskCreate(&gBtlWork->taskPools[0], &gTaskDescHumLaxene, NULL);
            SetBattleBounds(0x50, 0x1B0, 0x160, 0x1A2);
            break;
        case BATTLE_AXEL_1:
        case BATTLE_AXEL_2:
            TaskCreate(&gBtlWork->taskPools[0], &gTaskDescHumAxcel, NULL);
            SetBattleBounds(0x50, 0x1B0, 0x160, 0x1A2);
            break;
        case BATTLE_VEXEN_1:
        case BATTLE_VEXEN_2:
        case BATTLE_VEXEN_3:
            TaskCreate(&gBtlWork->taskPools[0], &gTaskDescHumVixen, NULL);
            SetBattleBounds(0x50, 0x1B0, 0x160, 0x1A2);
            break;
        case BATTLE_LEXAEUS:
            TaskCreate(&gBtlWork->taskPools[0], &gTaskDescHumLexceus, NULL);
            SetBattleBounds(0x50, 0x1B0, 0x160, 0x1A2);
            break;
        case BATTLE_LEON:
            TaskCreate(&gBtlWork->taskPools[0], &gTaskDescHumLeon, NULL);
            SetBattleBounds(0x50, 0x1B0, 0x160, 0x1A2);
            break;
        default:
            TaskCreate(&gBtlWork->taskPools[0], &gTaskDescHumRiku, NULL);
            SetBattleBounds(0x50, 0x1B0, 0x160, 0x1A2);
            break;
        }

        TaskCreate(&gBtlWork->taskPools[1], &gTaskDescBtlMap, NULL);
    } else {
        SetBattleBounds(0x68, 0x198, 0x160, 0x1A2);

        if (mode < ARRAY_COUNT(gBtlFormListByBattleId)) {
            TaskCreate(&gBtlWork->taskPools[0], &gTaskDescBtlForm, (void*)gBtlFormListByBattleId[mode]);
        } else if (gDebugFlags & DEBUG_FLAG_CHKBTL) {
            ChkBtlSpawnEnemy();
        }

        TaskCreate(&gBtlWork->taskPools[1], &gTaskDescBtlMap, NULL);

        if (mode == BATTLE_SHADOW_100) {
            gBtlWork->flags |= BTL_FLAG_NO_ENEMY_DROPS;
            gGameState.flags |= GAME_FLAG_MONSGAGE_BATTLE;
            TaskCreate(&gBtlWork->taskPools[0], &gTaskDescMonsgage, NULL);
        }
    }

    if (!(gBtlWork->flags & BTL_FLAG_TUTORIAL)) {
        TaskCreate(&gBtlWork->taskPools[2], &gTaskDescBtlPause, NULL);
    }

    FadeStartIn(FADE_MODE_BLACK, 60);
    gGameState.battleCount++;
}

void mode_battle_1() {
#ifdef PLATFORM_PC
    /* The practice target is durable; allow leaving any drill through the
     * same mode-exit cleanup used by ordinary completed debug battles. */
    if(PortPracticeActive() && (GetKeysPressed()&SELECT_BUTTON)) {
        ExitBattle();
        return;
    }
    PortActionCaptureInput();
#endif
    TaskPoolUpdate(&gBtlWork->taskPools[2]);
    TaskPoolDraw(&gBtlWork->taskPools[2]);

    if (gBtlWork->freezeTimer > 0) {
        TaskPoolDraw(&gBtlWork->taskPools[1]);
        TaskPoolDraw(&gBtlWork->taskPools[0]);
        gBtlWork->freezeTimer--;
    } else {
        if (!gBtlWork->paused) {
            UpdateBattleState();

            if (gBtlWork->hitStop <= 0) {
                TaskPoolUpdate(&gBtlWork->taskPools[0]);
            } else {
                gBtlWork->hitStop--;
            }

            if (!(gBtlWork->flags & BTL_FLAG_BGFX_PAUSED)) {
                BgFxUpdate();
            }

            ColliderUpdateAll();
            TaskPoolDraw(&gBtlWork->taskPools[1]);

            if (gBtlWork->flags & BTL_FLAG_CARD_BREAK) {
                gBtlWork->flags &= ~BTL_FLAG_CARD_BREAK;
            }

            UpdatePlayTime();
        }

        if (!(gBtlWork->flags & BTL_FLAG_FIELD_HIDDEN)) {
            TaskPoolDraw(&gBtlWork->taskPools[0]);
        }
    }
}

void mode_battle_2() {
#ifdef PLATFORM_PC
    PortSetBattleView(0);
#endif
    gGameState.flags &= ~GAME_FLAG_FIRST_STRIKE;
    BgFxFree();
    TaskPoolDestroy(&gBtlWork->taskPools[2]);
    TaskPoolDestroy(&gBtlWork->taskPools[1]);
    TaskPoolDestroy(&gBtlWork->taskPools[0]);
    ReleaseBattleTiles();

    if (gBtlWork->flags & BTL_FLAG_HUM_BATTLE) {
        EwramFree(gRikuBtlWork);
    }

    EwramFree(gBtlWork);
}

Mode gModeBattle = { "mode_battle", (ModeInitFunc)mode_battle_0, mode_battle_1, mode_battle_2 };
