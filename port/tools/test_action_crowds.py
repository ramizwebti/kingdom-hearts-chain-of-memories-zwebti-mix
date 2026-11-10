#!/usr/bin/env python3
"""Execute the production formation scheduler with the real 32-bit game ABI.

Only enemy creation and resource availability are fixtures: the production
formation/list-pool code is used directly. No ROM, SDL or multilib libc needed.
This proves scheduling/backpressure, not actual graphics/OAM load safety.
"""
import errno
import os
from pathlib import Path
import subprocess
import tempfile

from test_action_collision import run_elf32

ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "btl3.h"
#include "battle_work.h"
#include "game_state.h"
#include "enemy_ids.h"
#include "battle_ids.h"
#include "sprite.h"
#include "taskpool.h"
#include "listpool.h"
#include "task_descriptors.h"

static BtlWork battle;
static BtlObj player;
static SpriteWork sprites;
static Task mainTasks[40];
static int requested, tileRoom, paletteRoom, fakeFailure;
static s32 heapFree;
static unsigned spawned, peak;
static s32 spawnedIds[128], spawnedX[128], spawnedY[128];
BtlWork* gBtlWork = &battle;
GameState gGameState;
SpriteWork* gSpriteWork = &sprites;
u16 gEnemyTileCounts[54];

#define CHECK(condition, code) do { if (!(condition)) return (code); } while (0)
static void zero(void* destination, unsigned size) {
    unsigned char* p = destination;
    while (size--) *p++ = 0;
}
static u8 enemyUpdate(void* work, Task* task) { return 1; }
static u8 bornUpdate(void* work, Task* task) { return 1; }
static TaskDesc enemyDescriptor = { "enemy", 0, enemyUpdate, 0, 0, 0 };
TaskDesc gTaskDescBtlBorn = { "born", 0, bornUpdate, 0, 0, 0 };
int PortActionCombatRequested(void) { return requested; }
s32 EwramGetFreeTotal(void) { return heapFree; }
void PortLog(const char* format, ...) {}
char* getenv(const char* name) { return 0; }
u16 GetRandom(void) { return 3; }
u8 CanAllocObjTiles(u16 count) { return tileRoom; }
u8 CanAllocObjPalette(u16 count) { return paletteRoom; }
u32 ClampBattlePosition(s32* x, s32* y, s32 mx, s32 my) {
    if (*x < (battle.xMin - mx) * 256) *x = (battle.xMin - mx) * 256;
    if (*x > (battle.xMax + mx) * 256) *x = (battle.xMax + mx) * 256;
    if (*y < (battle.yMin - my) * 256) *y = (battle.yMin - my) * 256;
    if (*y > (battle.yMax + my) * 256) *y = (battle.yMax + my) * 256;
    return 0;
}
u8 SpawnEnemy(s32 id, s32 x, s32 y, s32 z) {
    Task* task;
    if (fakeFailure) return 1; /* legacy SpawnEnemy can lie about TaskCreate */
    task = ListPoolFirstFree(&battle.taskPools[0]);
    if (!task) return 1;
    task->desc = &enemyDescriptor;
    task->work = (void*)(id + 1);
    ListPoolActivate(&task->node, &battle.taskPools[0]);
    battle.enemyCount++;
    battle.enemyTileCount += gEnemyTileCounts[id];
    battle.pendingEnemies--;
    spawnedIds[spawned] = id; spawnedX[spawned] = x; spawnedY[spawned] = y;
    spawned++;
    if (battle.enemyCount > peak) peak = battle.enemyCount;
    return 1;
}
#include "PRODUCTION_FORMATION"

static void reset(void) {
    unsigned i;
    zero(&battle, sizeof(battle)); zero(&player, sizeof(player));
    zero(&sprites, sizeof(sprites)); zero(&gGameState, sizeof(gGameState));
    zero(mainTasks, sizeof(mainTasks));
    battle.actor = &player; player.x = 256 * 256; player.y = 384 * 256;
    battle.xMax = battle.yMax = 512;
    battle.phase = BTL_PHASE_IDLE; battle.battleId = BATTLE_TRAVERSE_TOWN_0;
    requested = tileRoom = paletteRoom = 1; fakeFailure = 0;
    heapFree = 65536; spawned = peak = 0;
    ListPoolInit(&battle.taskPools[0]);
    for (i = 0; i < 40; i++) ListPoolAddFree(&mainTasks[i].node, &battle.taskPools[0], &mainTasks[i]);
    ListPoolInit(&sprites.tilePool);
    for (i = 0; i < 128; i++) ListPoolAddFree(&sprites.tiles[i].node, &sprites.tilePool, &sprites.tiles[i]);
    for (i = 0; i < 54; i++) gEnemyTileCounts[i] = 32;
    gEnemyTileCounts[ENEMY_SOLDIER] = 44;
}
static void removeOne(void) {
    Task* task = ListPoolFirst(&battle.taskPools[0]);
    s32 id;
    if (!task) return;
    id = (s32)task->work - 1;
    battle.enemyTileCount -= gEnemyTileCounts[id];
    battle.enemyCount--;
    ListPoolRelease(&task->node, &battle.taskPools[0]);
}
static int run(void) {
    static const BtlFormStep soldiers[] = {
        { ENEMY_SOLDIER, -45, 0, 0, 0 }, { ENEMY_SOLDIER, 45, 0, 0, 0 }
    };
    static const BtlFormEntry soldierEntry = { 2, soldiers, 0 };
    static const BtlFormEntry* soldierEntries[] = { &soldierEntry };
    static const BtlFormList small = { 1, soldierEntries, 256 };
    static const BtlFormStep shadowSteps[] = {
        { ENEMY_SHADOW, -80, -20, 0, 0 }, { ENEMY_SHADOW, 80, 20, 0, 0 },
        { ENEMY_SHADOW, -45, 20, 0, 0 }, { ENEMY_SHADOW, 45, -20, 0, 0 }
    };
    static const BtlFormEntry shadowEntry = { 4, shadowSteps, 0 };
    static const BtlFormEntry* shadowEntries[] = { &shadowEntry, &shadowEntry, &shadowEntry };
    static const BtlFormList large = { 3, shadowEntries, 256 };
    static const BtlFormStep mushroomSteps[] = { { ENEMY_WHITE_MUSHROOM, 0, 0, 0, 0 } };
    static const BtlFormEntry mushroomEntry = { 1, mushroomSteps, 0 };
    static const BtlFormEntry* mushroomEntries[] = { &mushroomEntry };
    static const BtlFormList mushroom = { 1, mushroomEntries, 256 };
    static const BtlFormStep mixedSteps[] = {
        { ENEMY_SOLDIER, -60, 0, 0, 0 }, { ENEMY_RED_NOCTURNE, 60, 0, 0, 0 },
        { ENEMY_BLUE_RHAPSODY, -80, 20, 0, 0 }, { ENEMY_PIRATE, 80, 20, 0, 0 }
    };
    static const BtlFormEntry mixedEntry = { 4, mixedSteps, 0 };
    static const BtlFormEntry* mixedEntries[] = { &mixedEntry };
    static const BtlFormList mixed = { 1, mixedEntries, 256 };
    static const s32 mixedOrder[] = { ENEMY_SOLDIER, ENEMY_RED_NOCTURNE, ENEMY_SHADOW,
        ENEMY_BLUE_RHAPSODY, ENEMY_PIRATE, ENEMY_SOLDIER, ENEMY_SHADOW,
        ENEMY_RED_NOCTURNE, ENEMY_SHADOW, ENEMY_BLUE_RHAPSODY };
    BtlFormWork work;
    Task* born;
    unsigned i, j;
    s32 x, y, dx, dy;
    int alive;

    reset(); zero(&work, sizeof(work));
    task_btl_form_0(&work, &small);
    CHECK(battle.pendingEnemies == 8, 1);
    for (i = 0; i < 200; i++) if (!task_btl_form_1(&work)) break;
    CHECK(i < 200 && spawned == 8 && peak == 8, 2);
    CHECK(battle.enemyTileCount == 316 && battle.pendingEnemies == 0, 3);
    CHECK(spawnedIds[2] == ENEMY_SHADOW && spawnedIds[3] == ENEMY_SOLDIER &&
          spawnedIds[4] == ENEMY_SHADOW && spawnedIds[5] == ENEMY_SOLDIER, 40);
    task_btl_form_3();
    CHECK(!(battle.flags & BTL_FLAG_FORMATION_ACTIVE), 4);

    reset(); zero(&work, sizeof(work));
    task_btl_form_0(&work, &large);
    CHECK(battle.pendingEnemies == 24, 5);
    for (i = 0; i < 200; i++) CHECK(task_btl_form_1(&work), 6);
    CHECK(peak == 8 && spawned == 8 && battle.pendingEnemies == 16, 7);
    for (i = 0; i < 1000; i++) {
        if (i % 15 == 0 && battle.enemyCount) removeOne();
        alive = task_btl_form_1(&work);
        CHECK(battle.enemyCount <= 8 && battle.enemyTileCount <= 352, 8);
        CHECK(battle.pendingEnemies >= 0, 9);
        if (!alive) break;
    }
    CHECK(i < 1000 && spawned == 24 && battle.pendingEnemies == 0, 10);

    reset(); zero(&work, sizeof(work)); heapFree = 16000;
    task_btl_form_0(&work, &small);
    for (i = 0; i < 100; i++) CHECK(task_btl_form_1(&work), 11);
    CHECK(!spawned && battle.pendingEnemies == 8 && work.stepIndex == 0, 12);
    heapFree = 65536; tileRoom = 0;
    for (i = 0; i < 100; i++) CHECK(task_btl_form_1(&work), 13);
    CHECK(!spawned && battle.pendingEnemies == 8, 14);
    tileRoom = 1; paletteRoom = 0;
    for (i = 0; i < 100; i++) CHECK(task_btl_form_1(&work), 15);
    CHECK(!spawned && battle.pendingEnemies == 8, 16);
    paletteRoom = 1; sprites.entryCount = 97;
    for (i = 0; i < 100; i++) CHECK(task_btl_form_1(&work), 17);
    CHECK(!spawned && battle.pendingEnemies == 8, 18);
    sprites.entryCount = 0; fakeFailure = 1;
    for (i = 0; i < 100; i++) CHECK(task_btl_form_1(&work), 19);
    CHECK(!spawned && battle.pendingEnemies == 8 && work.stepIndex == 0, 20);
    fakeFailure = 0;
    for (i = 0; i < 200; i++) if (!task_btl_form_1(&work)) break;
    CHECK(i < 200 && spawned == 8 && battle.pendingEnemies == 0, 21);

    reset(); zero(&work, sizeof(work));
    gEnemyTileCounts[ENEMY_SOLDIER] = 100;
    task_btl_form_0(&work, &small);
    for (i = 0; i < 200; i++) CHECK(task_btl_form_1(&work), 34);
    CHECK(spawned == 6 && battle.enemyTileCount == 328 && battle.pendingEnemies == 2, 35);
    removeOne();
    for (i = 0; i < 200; i++) if (!task_btl_form_1(&work)) break;
    CHECK(i < 200 && spawned == 8 && battle.enemyTileCount <= 352, 36);

    reset(); zero(&work, sizeof(work));
    task_btl_form_0(&work, &small);
    for (i = 0; i < 34; i++) {
        Task* occupied = ListPoolFirstFree(&battle.taskPools[0]);
        occupied->desc = &enemyDescriptor;
        ListPoolActivate(&occupied->node, &battle.taskPools[0]);
    }
    for (i = 0; i < 100; i++) CHECK(task_btl_form_1(&work), 37);
    CHECK(!spawned && battle.pendingEnemies == 8, 38);
    while (ListPoolFirst(&battle.taskPools[0])) {
        Task* occupied = ListPoolFirst(&battle.taskPools[0]);
        ListPoolRelease(&occupied->node, &battle.taskPools[0]);
    }
    for (i = 0; i < 200; i++) if (!task_btl_form_1(&work)) break;
    CHECK(i < 200 && spawned == 8, 39);

    reset(); zero(&work, sizeof(work));
    task_btl_form_0(&work, &small);
    born = ListPoolFirstFree(&battle.taskPools[0]); born->desc = &gTaskDescBtlBorn;
    ListPoolActivate(&born->node, &battle.taskPools[0]);
    for (i = 0; i < 100; i++) CHECK(task_btl_form_1(&work), 22);
    CHECK(!spawned && battle.pendingEnemies == 8, 23);
    ListPoolRelease(&born->node, &battle.taskPools[0]);
    for (i = 0; i < 200; i++) if (!task_btl_form_1(&work)) break;
    CHECK(spawned == 8 && battle.pendingEnemies == 0, 24);

    reset(); zero(&work, sizeof(work)); requested = 0;
    task_btl_form_0(&work, &small);
    CHECK(sCrowdOwner == 0 && battle.pendingEnemies == 2, 25);
    reset(); gGameState.flags = GAME_FLAG_RIKU;
    task_btl_form_0(&work, &small);
    CHECK(sCrowdOwner == 0 && battle.pendingEnemies == 2, 26);
    reset(); battle.flags = BTL_FLAG_BOSS_BATTLE;
    task_btl_form_0(&work, &small);
    CHECK(sCrowdOwner == 0 && battle.pendingEnemies == 2, 27);
    reset(); battle.flags = BTL_FLAG_TUTORIAL;
    task_btl_form_0(&work, &small);
    CHECK(sCrowdOwner == 0 && battle.pendingEnemies == 2, 28);
    reset(); battle.battleId = BATTLE_SHADOW_100;
    task_btl_form_0(&work, &small);
    CHECK(sCrowdOwner == 0 && battle.pendingEnemies == 2, 29);
    reset(); task_btl_form_0(&work, &mushroom);
    CHECK(sCrowdOwner == 0 && battle.pendingEnemies == 1, 30);
    reset(); task_btl_form_0(&work, &small);
    battle.phase = BTL_PHASE_START;
    for (i = 0; i < 100; i++) CHECK(task_btl_form_1(&work), 31);
    CHECK(!spawned && battle.pendingEnemies == 8, 32);
    battle.flags |= BTL_FLAG_STOP_SPAWNING;
    CHECK(!task_btl_form_1(&work) && !spawned, 33);

    reset(); task_btl_form_0(&work, &mixed);
    for (i = 0; i < 1000; i++) {
        if (i % 15 == 0 && battle.enemyCount) removeOne();
        alive = task_btl_form_1(&work);
        CHECK(battle.enemyCount <= 8 && battle.enemyTileCount <= 352, 41);
        if (!alive) break;
    }
    CHECK(i < 1000 && spawned == 10 && battle.pendingEnemies == 0, 42);
    for (i = 0; i < 10; i++) CHECK(spawnedIds[i] == mixedOrder[i], 43);
    CHECK(sCrowdFlavorCount == 4, 44);
    CrowdRememberFlavor(ENEMY_SOLDIER);
    CHECK(sCrowdFlavorCount == 4, 45);
    CrowdRememberFlavor(ENEMY_GREEN_REQUIEM);
    CrowdRememberFlavor(ENEMY_BARREL_SPIDER);
    CrowdRememberFlavor(ENEMY_LARGE_BODY);
    CHECK(sCrowdFlavorCount == 4, 46);

    /* Every reinforcement rotation remains clear after arena-edge clamping. */
    for (i = 0; i < 4; i++) {
        player.x = (i & 1 ? 496 : 16) * 256;
        player.y = (i & 2 ? 496 : 16) * 256;
        for (j = 0; j < 8; j++) {
            sCrowdBonusTotal = 8; sCrowdBonusLeft = 8 - j;
            CrowdReinforcementPosition(&x, &y);
            dx = x - player.x; if (dx < 0) dx = -dx;
            dy = y - player.y; if (dy < 0) dy = -dy;
            CHECK(dx >= 72 * 256 || dy >= 48 * 256, 47);
            CHECK(x >= 24 * 256 && x <= 488 * 256 &&
                  y >= 12 * 256 && y <= 500 * 256, 48);
        }
    }
    battle.xMax = 120; battle.yMax = 64;
    player.x = 60 * 256; player.y = 32 * 256;
    CrowdReinforcementPosition(&x, &y);
    CHECK(x >= 24 * 256 && x <= 96 * 256 && y >= 12 * 256 && y <= 52 * 256, 49);
    CHECK(x != player.x || y != player.y, 50);

    /* A newly started battle cannot inherit an earlier encounter's roster. */
    reset(); task_btl_form_0(&work, &large);
    CHECK(sCrowdFlavorCount == 0 && sCrowdAuthoredSinceBonus == 0, 51);
    for (i = 0; i < 200; i++) CHECK(task_btl_form_1(&work), 52);
    for (i = 0; i < spawned; i++) CHECK(spawnedIds[i] == ENEMY_SHADOW, 53);
    return 0;
}
void _start(void) {
    int code = run();
    __asm__ volatile ("int $0x80" : : "a"(1), "b"(code));
    __builtin_unreachable();
}
'''


def main():
    includes = [ROOT / 'include', ROOT / 'port/include']
    includes += sorted(p for p in (ROOT / 'include').iterdir() if p.is_dir() and p.name != 'gba')
    with tempfile.TemporaryDirectory(prefix='rekhcom-crowd-') as temporary:
        directory = Path(temporary)
        harness = directory / 'crowd_runner.c'
        harness.write_text(HARNESS.replace('PRODUCTION_FORMATION', str(ROOT / 'src/btl/btl_form.c')))
        executable = directory / 'crowd_runner'
        command = [os.environ.get('CC', 'gcc'), '-m32', '-O2', '-std=gnu89',
                   '-ffreestanding', '-fno-builtin', '-fno-stack-protector', '-fno-pie',
                   '-ffunction-sections', '-fdata-sections', '-w', '-DPLATFORM_PC', '-DVERSION_US',
                   '-nostdlib', '-static', '-no-pie', '-Wl,--gc-sections']
        command += ['-I' + str(path) for path in includes]
        command += [str(ROOT / 'src/listpool.c'), str(harness), '-o', str(executable)]
        subprocess.run(command, cwd=ROOT, check=True)
        try:
            status = subprocess.run([str(executable)], check=False).returncode
        except OSError as error:
            if error.errno != errno.ENOEXEC:
                raise
            status = run_elf32(executable)
        if status:
            raise SystemExit('Crowd scheduler regression failed at check ' + str(status))
    print('PASS: real 32-bit crowd scheduler, 53 bounds/backpressure/exclusion/mixed-wave/clearance checks')


if __name__ == '__main__':
    main()
