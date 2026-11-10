/**
 * btl_form.c
 * Enemy Formation Spawner
 */

#include "btl3.h"
#include "btl3_tasks.h"
#include "battle_actor.h"
#include "battle_actor_types.h"
#include "battle_work.h"
#include "engine_math.h"
#include "formation_types.h"
#include "game_state.h"
#include "taskpool.h"
#include "types.h"
#include <stddef.h>
#include "enemy_tile_counts.h"
#include "map_types.h"

#ifdef PLATFORM_PC
#include "port/port_action_game.h"
#include "enemy_ids.h"
#include "battle_ids.h"
#include "listpool.h"
#include "obj_api.h"
#include "sprite.h"
#include "task_descriptors.h"

/* Ordinary action battles retain their authored enemy types and encounter
 * order, but overlap waves and interleave finite, encounter-themed reinforcements. Tiles
 * remain per actor in this engine, even for matching kinds; never multiply
 * the formation threshold without checking the live resource pools too. */
enum {
    ACTION_CROWD_LIVE_CAP = 8,
    ACTION_CROWD_TILE_CAP = 352,
    ACTION_CROWD_TASK_RESERVE = 8,
    ACTION_CROWD_HEAP_RESERVE = 16384,
    ACTION_CROWD_SPAWN_INTERVAL = 8
};

static BtlFormWork* sCrowdOwner;
static u16 sCrowdBonusLeft, sCrowdBonusTotal, sCrowdSpawned, sCrowdPeak;
static u16 sCrowdCooldown, sCrowdBlocked;
static u8 sCrowdFlavors[8], sCrowdFlavorCount, sCrowdAuthoredSinceBonus;
extern s32 EwramGetFreeTotal(void);
extern void PortLog(const char* fmt, ...);
extern char* getenv(const char* name);
static u8 UpdateActionCrowd(BtlFormWork* work);

static u8 CrowdTrace(void) {
    const char* value = getenv("KHCOM_ACTION_TRACE");
    return value != NULL && value[0] != '0';
}

static s32 CrowdFreeNodes(const ListPool* pool) {
    const ListNode* node;
    s32 count = 0;
    for (node = pool->freeHead; node != NULL; node = node->next) count++;
    return count;
}

static u8 CrowdEligible(const BtlFormList* list) {
    s32 i, j;
    if (!PortActionCombatRequested() || (gGameState.flags & GAME_FLAG_RIKU) ||
        gBtlWork->battleId >= BATTLE_CARD_SOLDIERS ||
        (gBtlWork->flags & (BTL_FLAG_BOSS_BATTLE | BTL_FLAG_HUM_BATTLE |
                           BTL_FLAG_TUTORIAL | BTL_FLAG_VS_BATTLE))) return FALSE;
    for (i = 0; i < list->count; i++) {
        for (j = 0; j < list->entries[i]->count; j++) {
            u32 id = list->entries[i]->steps[j].id;
            if (id >= ENEMY_GUARD_ARMOR || id == ENEMY_WHITE_MUSHROOM || id == ENEMY_BLACK_FUNGUS)
                return FALSE;
        }
    }
    return list->count > 0;
}

static u8 CrowdCanSpawn(u16 tiles) {
    Task* task;
    /* Born tasks have not yet added their actors/tiles to the live counters.
     * Serialize those requests so reservations cannot race each other. */
    for (task = ListPoolFirst(&gBtlWork->taskPools[0]); task != NULL;
         task = ListPoolNext(&task->node)) {
        if (task->desc->update == gTaskDescBtlBorn.update) return FALSE;
    }
    if (gBtlWork->enemyCount >= ACTION_CROWD_LIVE_CAP ||
        gBtlWork->enemyTileCount + tiles > ACTION_CROWD_TILE_CAP ||
        CrowdFreeNodes(&gBtlWork->taskPools[0].head) < ACTION_CROWD_TASK_RESERVE ||
        EwramGetFreeTotal() < ACTION_CROWD_HEAP_RESERVE || gSpriteWork == NULL)
        return FALSE;
    /* Four tiles per status indicator, plus a contiguous 32-tile effect
     * reserve; tile descriptors and palette banks have independent caps. */
    if (!CanAllocObjTiles(tiles + 4 + 32) || !CanAllocObjPalette(2) ||
        CrowdFreeNodes(&gSpriteWork->tilePool.head) < 8 ||
        gSpriteWork->entryCount > 96) return FALSE;
    return TRUE;
}

static void CrowdRememberFlavor(s32 id) {
    u8 i;
    /* Only reuse light enemies that this formation has already introduced.
     * Healers, explosives, heavy guards and special actors are deliberately
     * absent: extra copies would change encounter rules or stall cleanup. */
    switch (id) {
    case ENEMY_RED_NOCTURNE: case ENEMY_BLUE_RHAPSODY: case ENEMY_YELLOW_OPERA:
    case ENEMY_SOLDIER: case ENEMY_POWERWILD: case ENEMY_AIR_SOLDIER:
    case ENEMY_BANDIT: case ENEMY_WIGHT_KNIGHT: case ENEMY_PIRATE:
        break;
    default: return;
    }
    if (gEnemyTileCounts[id] > 64) return;
    for (i = 0; i < sCrowdFlavorCount; i++) if (sCrowdFlavors[i] == id) return;
    if (sCrowdFlavorCount < sizeof(sCrowdFlavors))
        sCrowdFlavors[sCrowdFlavorCount++] = id;
}

static s32 CrowdReinforcementKind(void) {
    u16 used = sCrowdBonusTotal - sCrowdBonusLeft;
    /* Half of all additions remain small, independently attacking Shadows.
     * The other half rotate through the encounter's native light enemies. */
    if ((used & 1) && sCrowdFlavorCount)
        return sCrowdFlavors[(used >> 1) % sCrowdFlavorCount];
    return ENEMY_SHADOW;
}

static void CrowdReinforcementPosition(s32* x, s32* y) {
    static const s16 offsets[8][2] = {
        { -104, -24 }, { 104, 24 }, { -80, 40 }, { 80, -40 },
        { -120, 0 }, { 120, 0 }, { 0, -56 }, { 0, 56 }
    };
    s32 i, index, cx, cy, dx, dy, score, best = -1;
    BtlObj* player = gBtlWork->actor;
    /* Clamp each candidate before testing clearance. Near an arena wall,
     * blindly clamping a behind-player spawn used to put it on top of Sora. */
    for (i = 0; i < 8; i++) {
        index = (sCrowdBonusTotal - sCrowdBonusLeft + i) & 7;
        cx = player->x + offsets[index][0] * 256;
        cy = player->y + offsets[index][1] * 256;
        ClampBattlePosition(&cx, &cy, -24, -12);
        dx = cx - player->x; if (dx < 0) dx = -dx;
        dy = cy - player->y; if (dy < 0) dy = -dy;
        score = dx + dy * 2;
        if (score > best) { *x = cx; *y = cy; best = score; }
        if (dx >= 72 * 256 || dy >= 48 * 256) { *x = cx; *y = cy; return; }
    }
    /* Very small bespoke arenas still make progress at their safest point. */
}
#endif

TaskDesc gTaskDescBtlForm = {
    "task_btl_form",
    (TaskInitFunc)task_btl_form_0,
    (TaskUpdateFunc)task_btl_form_1,
    NULL,
    (TaskDestroyFunc)task_btl_form_3,
    sizeof(BtlFormWork),
};

u16 GetBtlFormEntryTileCount(const BtlFormEntry* list) {
    u16 total;
    s32 i;

    total = 0;

    for (i = 0; i < list->count; i++) {
        total += gEnemyTileCounts[list->steps[i].id];
    }

    return total;
}

void task_btl_form_0(BtlFormWork* work, const BtlFormList* list) {
    s32 i;

    gBtlWork->flags |= BTL_FLAG_FORMATION_ACTIVE;
    work->flags = 0;
    work->list = list;
    work->entry = list->entries[0];
    work->timer = work->entry->delay;
    work->entryIndex = 1;
    work->stepTimer = 0;
    work->stepIndex = 0;
    work->nextTileCount = 0;
    work->waitTimer = 100;
    gBtlWork->pendingEnemies = 0;

    for (i = 0; i < list->count; i++) {
        gBtlWork->pendingEnemies += list->entries[i]->count;
    }
#ifdef PLATFORM_PC
    sCrowdOwner = NULL;
    if (CrowdEligible(list)) {
        sCrowdOwner = work;
        sCrowdBonusTotal = gBtlWork->pendingEnemies;
        if (sCrowdBonusTotal < 6) sCrowdBonusTotal = 6;
        if (sCrowdBonusTotal > 24) sCrowdBonusTotal = 24;
        sCrowdBonusLeft = sCrowdBonusTotal;
        gBtlWork->pendingEnemies += sCrowdBonusTotal;
        sCrowdSpawned = sCrowdPeak = sCrowdCooldown = sCrowdBlocked = 0;
        sCrowdFlavorCount = sCrowdAuthoredSinceBonus = 0;
        if (CrowdTrace()) PortLog("crowd: begin battle=%u authored=%d bonus=%u cap=%d tiles=%d",
            gBtlWork->battleId, gBtlWork->pendingEnemies - sCrowdBonusTotal,
            sCrowdBonusTotal, ACTION_CROWD_LIVE_CAP, ACTION_CROWD_TILE_CAP);
    }
#endif
}

u8 task_btl_form_1(BtlFormWork* work) {
    const BtlFormList* list;
    const BtlFormStep* step;
    BtlObj* obj;
    s32 x;
    s32 y;
    s32 z;

#ifdef PLATFORM_PC
    if (sCrowdOwner == work) return UpdateActionCrowd(work);
#endif

    if (gBtlWork->flags & BTL_FLAG_STOP_SPAWNING) {
        return 0;
    }

    if (work->flags & BTL_FORM_FLAG_WAIT_NEXT_ENTRY) {
        list = work->list;

        if (list->threshold >= work->nextTileCount + gBtlWork->enemyTileCount) {
            if (work->nextTileCount == 0) {
                return 0;
            }

            work->entry = list->entries[work->entryIndex];
            work->timer = work->entry->delay;
            work->stepTimer = 0;
            work->stepIndex = 0;
            work->flags &= ~BTL_FORM_FLAG_WAIT_NEXT_ENTRY;
            work->entryIndex++;
            work->waitTimer = 100;
        }
    } else if (work->entry->count <= work->stepIndex) {
        if (work->waitTimer-- <= 0) {
            work->flags |= BTL_FORM_FLAG_WAIT_NEXT_ENTRY;

            if (gGameState.roomEffect != ROOM_EFFECT_LASTING_DAZE) {
                gGameState.flags &= ~GAME_FLAG_FIRST_STRIKE;
            }

            if (work->entryIndex >= work->list->count) {
                work->nextTileCount = 0;
                return 0;
            }

            work->nextTileCount = GetBtlFormEntryTileCount(work->list->entries[work->entryIndex]);
        }
    } else {
        if (work->timer > 0) {
            work->timer--;
        } else {
            if (work->timer == 0) {
                obj = gBtlWork->actor;
                work->x = (obj->x + 0x10000) >> 1;
                work->y = obj->y;
                work->z = 0;

                if (obj->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    if (GetRandom() % 5 != 0) {
                        work->flags |= BTL_FORM_FLAG_MIRROR_X;
                    } else {
                        work->flags &= ~BTL_FORM_FLAG_MIRROR_X;
                    }
                } else {
                    if (GetRandom() % 5 == 0) {
                        work->flags |= BTL_FORM_FLAG_MIRROR_X;
                    } else {
                        work->flags &= ~BTL_FORM_FLAG_MIRROR_X;
                    }
                }

                work->timer = 0xFFFF;
            }

            step = &work->entry->steps[work->stepIndex];

            if (work->stepTimer >= step->delay) {
                if (work->flags & BTL_FORM_FLAG_MIRROR_X) {
                    x = work->x - (step->x << 8);
                } else {
                    x = work->x + (step->x << 8);
                }

                y = work->y + (step->y << 8);
                z = work->z + (step->z << 8);
                SpawnEnemy(step->id, x, y, z);
                work->stepIndex++;
            } else {
                work->stepTimer++;
            }
        }
    }

    return 1;
}

void task_btl_form_3() {
#ifdef PLATFORM_PC
    if (sCrowdOwner != NULL) {
        if (CrowdTrace()) PortLog("crowd: end spawned=%u peak=%u bonus=%u remaining=%d",
            sCrowdSpawned, sCrowdPeak, sCrowdBonusTotal, gBtlWork->pendingEnemies);
        sCrowdOwner = NULL;
    }
#endif
    gBtlWork->flags &= ~BTL_FLAG_FORMATION_ACTIVE;
}

#ifdef PLATFORM_PC
static u8 UpdateActionCrowd(BtlFormWork* work) {
    const BtlFormStep* step = NULL;
    ListNode* freeBefore;
    s32 x, y, z, id;
    u8 reinforcement = FALSE;

    if (gBtlWork->flags & BTL_FLAG_STOP_SPAWNING) return 0;
    if (gBtlWork->enemyCount > sCrowdPeak) {
        sCrowdPeak = gBtlWork->enemyCount;
        if (CrowdTrace()) PortLog("crowd: peak live=%u tiles=%d", sCrowdPeak, gBtlWork->enemyTileCount);
    }
    if (gBtlWork->phase != BTL_PHASE_IDLE && gBtlWork->phase != BTL_PHASE_CARD_PLAY) return 1;
    if (sCrowdCooldown && gBtlWork->enemyCount) { sCrowdCooldown--; return 1; }
    sCrowdCooldown = 0;
    if (work->timer > 0) { work->timer--; return 1; }

    while (work->entry != NULL && work->stepIndex >= work->entry->count) {
        if (gGameState.roomEffect != ROOM_EFFECT_LASTING_DAZE)
            gGameState.flags &= ~GAME_FLAG_FIRST_STRIKE;
        if (work->entryIndex >= work->list->count) {
            work->entry = NULL;
            break;
        }
        work->entry = work->list->entries[work->entryIndex++];
        work->stepIndex = 0;
        /* Overlap the next authored wave, without the cartridge's 100-frame
         * wait-and-threshold pause. The global resource gate still applies. */
    }
    /* Thread additions through the encounter instead of ending every battle
     * with a long homogeneous Shadow tail. Authored steps keep their order. */
    reinforcement = sCrowdBonusLeft &&
        (work->entry == NULL || sCrowdAuthoredSinceBonus >= 2);
    if (work->entry != NULL && !reinforcement) {
        step = &work->entry->steps[work->stepIndex];
        id = step->id;
        x = ((gBtlWork->actor->x + 0x10000) >> 1) + (step->x * 256);
        y = gBtlWork->actor->y + (step->y * 256);
        z = step->z * 256;
    } else {
        if (!sCrowdBonusLeft) return 0;
        id = CrowdReinforcementKind();
        CrowdReinforcementPosition(&x, &y);
        z = 0;
        reinforcement = TRUE;
    }
    if (!CrowdCanSpawn(gEnemyTileCounts[id])) {
        if (++sCrowdBlocked == 180 && CrowdTrace())
            PortLog("crowd: waiting live=%u tiles=%d tasks=%d heap=%d",
                gBtlWork->enemyCount, gBtlWork->enemyTileCount,
                CrowdFreeNodes(&gBtlWork->taskPools[0].head), EwramGetFreeTotal());
        return 1;
    }
    sCrowdBlocked = 0;
    ClampBattlePosition(&x, &y, -24, -12);
    freeBefore = gBtlWork->taskPools[0].head.freeHead;
    /* SpawnEnemy's legacy API reports success even if TaskCreate fails.
     * Require the free-task head to move before consuming an authored step
     * or a reinforcement. No silently lost enemies or stuck pending count. */
    if (!SpawnEnemy(id, x, y, z) ||
        gBtlWork->taskPools[0].head.freeHead == freeBefore) return 1;
    if (reinforcement) {
        sCrowdBonusLeft--;
        sCrowdAuthoredSinceBonus = 0;
    } else {
        CrowdRememberFlavor(id);
        work->stepIndex++;
        sCrowdAuthoredSinceBonus++;
    }
    sCrowdSpawned++;
    sCrowdCooldown = gBtlWork->enemyCount < 3 ? 4 : ACTION_CROWD_SPAWN_INTERVAL;
    if (CrowdTrace()) PortLog("crowd: spawn id=%d live=%u tiles=%d pending=%d total=%u draw=%u bonus=%u mixed=%u x=%d y=%d player=%d,%d",
        id, gBtlWork->enemyCount, gBtlWork->enemyTileCount,
        gBtlWork->pendingEnemies, sCrowdSpawned, gSpriteWork->entryCount,
        reinforcement, reinforcement && id != ENEMY_SHADOW,
        x >> 8, y >> 8, gBtlWork->actor->x >> 8, gBtlWork->actor->y >> 8);
    return 1;
}
#endif
