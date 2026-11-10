#!/usr/bin/env python3
"""Run real 32-bit collision target-routing regressions without ROM assets.

This links the game's collision and list-pool translation units with section
garbage collection. A freestanding Linux runner avoids a multilib libc
dependency while retaining the actual GBA-sized structures and hit tests.
Requires GCC's -m32 support. Hosts without Linux i386 execution support may
install the optional Python unicorn package for a bounded CPU-only runner.
"""
import errno
import os
from pathlib import Path
import struct
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "btl_collision.h"
#include "battle_work.h"
#include "listpool.h"
#include "port/port_action_game.h"

static BtlWork sBattle;
static BtlWork sOpponentBattle;
static BtlObj sPlayer;
static BtlObj sEnemy;
static BtlObj sEnemy2;
BtlWork* gBtlWork = &sBattle;
BtlWork* gRikuBtlWork = &sOpponentBattle;
static unsigned sFeedbackHits, sFeedbackDamage, sFxHits, sObserved, sMeleeContacts;
static int sAction = 1;
vu32 gFrameCounter;

/* Only external presentation/adaptor endpoints are substituted. Collision,
 * damage scaling, guards, target iteration and pause selection are production. */
int PortActionCombatRules(void) { return sAction; }
void PortActionFeedbackHit(unsigned damage) { ++sFeedbackHits; sFeedbackDamage += damage; }
void PortActionFxHit(s32 x, s32 y, s32 z, unsigned power) { ++sFxHits; }
void PortActionObserveEnemyAttack(s32 x, s32 y, s32 z, s16 hx, s16 hy, s16 hz) { ++sObserved; }
void PortActionPlayerMeleeHit(void) { ++sMeleeContacts; }
void CreateBtlPopTask(BtlObj* obj, s16 kind) { }
void m4aSongNumStart(u16 song) { }
u8 GetAngle(s32 x0, s32 y0, s32 x1, s32 y1) { return 0; }
#define EFFECT(name) void name(s32 x, s32 y, s32 z) { }
EFFECT(BgFxStartBlizzardHit)
EFFECT(BgFxStartEnemyHit)
EFFECT(BgFxStartFireHit)
EFFECT(BgFxStartFlashHit)
EFFECT(BgFxStartFriendHit)
EFFECT(BgFxStartRikuHit)
EFFECT(BgFxStartSoraHit)
EFFECT(BgFxStartThunderHit)
EFFECT(BgFxStartGuard)

#define CHECK(condition, code) do { if (!(condition)) return (code); } while (0)

static int run_tests(void) {
    BtlObj* previous;
    BtlObj* nested;
    sBattle.actor = &sPlayer;
    sBattle.actor3 = &sEnemy;
    sOpponentBattle.actor = &sEnemy;
    sEnemy.x = 100 * 256;
    sPlayer.height = sEnemy.height = 16;
    sPlayer.radiusX = sEnemy.radiusX = 4;
    sPlayer.radiusY = sEnemy.radiusY = 4;
    ListPoolInit(&sBattle.pool);
    ListNodeInit(&sEnemy.node, &sBattle.pool, &sEnemy);
    ListPoolAppend(&sEnemy.node, &sBattle.pool);

    /* Original enemy turn: only the player is a target. */
    CHECK(BtlGetAttackSource() == 0, 1);
    CHECK(!BtlAttackIsPlayer(), 2);
    CHECK(TestAttackBox(0, 0, 0, 1, 1, 1), 3);
    CHECK(!TestAttackBox(sEnemy.x, 0, 0, 1, 1, 1), 4);

    /* Independent Sora hit during an enemy turn must hit the enemy. */
    previous = BtlSetAttackSource(&sPlayer);
    CHECK(previous == 0 && BtlGetAttackSource() == &sPlayer, 5);
    CHECK(BtlAttackIsPlayer(), 6);
    CHECK(TestAttackBox(sEnemy.x, 0, 0, 1, 1, 1), 7);
    CHECK(!TestAttackBox(0, 0, 0, 1, 1, 1), 8);

    /* A nested enemy callback must still hit Sora during a player turn. */
    sBattle.flags = BTL_FLAG_PLAYER_CARD_ACTION;
    nested = BtlSetAttackSource(&sEnemy);
    CHECK(nested == &sPlayer && !BtlAttackIsPlayer(), 9);
    CHECK(TestAttackBox(0, 0, 0, 1, 1, 1), 10);
    CHECK(!TestAttackBox(sEnemy.x, 0, 0, 1, 1, 1), 11);
    BtlSetAttackSource(nested);
    CHECK(BtlGetAttackSource() == &sPlayer && BtlAttackIsPlayer(), 12);
    CHECK(TestAttackBox(sEnemy.x, 0, 0, 1, 1, 1), 13);

    /* Restoring NULL returns to cartridge ownership immediately. */
    BtlSetAttackSource(previous);
    CHECK(BtlGetAttackSource() == 0 && BtlAttackIsPlayer(), 14);
    CHECK(TestAttackBox(sEnemy.x, 0, 0, 1, 1, 1), 15);
    sBattle.flags = 0;
    CHECK(!BtlAttackIsPlayer(), 16);
    CHECK(TestAttackBox(0, 0, 0, 1, 1, 1), 17);

    /* The opponent-actor branch uses the same independent routing. */
    sBattle.flags = BTL_FLAG_VS_BATTLE;
    BtlSetAttackSource(&sPlayer);
    CHECK(TestAttackBox(sEnemy.x, 0, 0, 1, 1, 1), 18);
    CHECK(!TestAttackBox(0, 0, 0, 1, 1, 1), 19);
    BtlSetAttackSource(&sEnemy);
    CHECK(TestAttackBox(0, 0, 0, 1, 1, 1), 20);
    CHECK(!TestAttackBox(sEnemy.x, 0, 0, 1, 1, 1), 21);

    /* Confirmed physical damage increments feedback once and shortens the
     * authored 16-tick finisher pause to three without reducing its damage. */
    sBattle.flags = 0;
    sPlayer.attack = 16;
    sEnemy.hp = 100;
    BtlSetAttackSource(&sPlayer);
    CHECK(ApplyAttackToBtlObj(14, &sEnemy) == ATTACK_RESULT_HIT, 22);
    CHECK(sEnemy.damage == 24 && (sEnemy.flags & BTLOBJ_FLAG_DAMAGE_PENDING), 23);
    CHECK(sBattle.pendingHitStop == 3, 24);
    CHECK(sFeedbackHits == 1 && sFeedbackDamage == 24 && sFxHits == 1, 25);
    CHECK(sMeleeContacts == 1, 49);

    /* A weaker contact from the same frame cannot shorten its existing pause. */
    CHECK(ApplyAttackToBtlObj(12, &sEnemy) == ATTACK_RESULT_HIT, 26);
    CHECK(sBattle.pendingHitStop == 3 && sFeedbackHits == 2, 27);
    CHECK(sMeleeContacts == 2, 50);
    ++gFrameCounter;
    /* pendingHitStop intentionally still contains the prior frame's three. */
    CHECK(ApplyAttackToBtlObj(10, &sEnemy) == ATTACK_RESULT_HIT, 28);
    CHECK(sBattle.pendingHitStop == 2 && sEnemy.damage == 128, 29);
    CHECK(sFeedbackHits == 3 && sFeedbackDamage == 168, 30);
    CHECK(sMeleeContacts == 2, 51); /* magic cannot restore melee MP */

    /* Native immunity/guard handling creates no successful-damage feedback. */
    sEnemy.flags = BTLOBJ_FLAG_GUARD_PHYSICAL;
    CHECK(ApplyAttackToBtlObj(14, &sEnemy) == ATTACK_RESULT_BLOCKED, 31);
    CHECK(sFeedbackHits == 3 && sFxHits == 3, 32);
    sEnemy.flags = BTLOBJ_FLAG_INVULNERABLE;
    CHECK(ApplyAttackToBtlObj(14, &sEnemy) == ATTACK_RESULT_HIT, 33);
    CHECK(sFeedbackHits == 3 && sFxHits == 3, 34);
    sEnemy.flags = 0;
    sEnemy.hp = 0;
    CHECK(ApplyAttackToBtlObj(14, &sEnemy) == ATTACK_RESULT_HIT, 35);
    CHECK(sFeedbackHits == 3 && sFxHits == 3, 36);
    CHECK(sMeleeContacts == 2, 52); /* guards, immunity and dead targets */

    /* A real wide strike hits both targets and produces independent contacts. */
    sEnemy.hp = sEnemy2.hp = 100;
    sEnemy.invincibleTimer = 0;
    sEnemy.flags = 0;
    sEnemy2.x = 101 * 256;
    sEnemy2.height = 16;
    sEnemy2.radiusX = sEnemy2.radiusY = 4;
    ListNodeInit(&sEnemy2.node, &sBattle.pool, &sEnemy2);
    ListPoolAppend(&sEnemy2.node, &sBattle.pool);
    sBattle.pendingHitStop = 0;
    CHECK(ApplyAttackBox(14, sEnemy.x, 0, 0, 2, 2, 2) == ATTACK_RESULT_HIT, 37);
    CHECK(sEnemy.damage == 24 && sEnemy2.damage == 24, 38);
    CHECK(sFeedbackHits == 5 && sFxHits == 5 && sBattle.pendingHitStop == 3, 39);
    CHECK(sObserved == 0, 40);
    CHECK(sMeleeContacts == 4, 53); /* adapter/controller deduplicates per swing */

    /* AI range probes cannot award a perfect dodge. Actual enemy dispatch
     * invokes its observer and keeps the authored incoming-damage pause. */
    BtlSetAttackSource(&sEnemy);
    sEnemy.attack = 10;
    sPlayer.hp = 100;
    sPlayer.flags = 0;
    CHECK(TestAttackBox(0, 0, 0, 2, 2, 2) && sObserved == 0, 41);
    sBattle.pendingHitStop = 0;
    CHECK(ApplyAttackBox(14, 0, 0, 0, 2, 2, 2) == ATTACK_RESULT_HIT, 42);
    CHECK(sObserved == 1 && sPlayer.damage == 15 && sBattle.pendingHitStop == 16, 43);
    CHECK(sFeedbackHits == 5 && sFxHits == 5, 44);
    CHECK(sMeleeContacts == 4, 54); /* enemy hits cannot restore player MP */

    /* Incoming attacks may pause longer, but never poison a later player's
     * contact. Retain pendingHitStop=16 to exercise the production lifecycle. */
    ++gFrameCounter;
    BtlSetAttackSource(&sPlayer);
    CHECK(ApplyAttackToBtlObj(12, &sEnemy) == ATTACK_RESULT_HIT, 47);
    CHECK(sBattle.pendingHitStop == 2 && sFeedbackHits == 6, 48);
    CHECK(sMeleeContacts == 5, 55);

    /* Classic action-disabled damage retains cartridge pause behavior. */
    BtlSetAttackSource(&sPlayer);
    sAction = 0;
    sBattle.pendingHitStop = 0;
    CHECK(ApplyAttackToBtlObj(14, &sEnemy) == ATTACK_RESULT_HIT, 45);
    CHECK(sBattle.pendingHitStop == 16 && sFeedbackHits == 6 && sFxHits == 6, 46);
    CHECK(sMeleeContacts == 5, 56); /* classic behavior stays untouched */
    /* Earned enders keep a short stronger beat, and cannot farm basic MP. */
    sAction = 1;
    ++gFrameCounter;
    CHECK(ApplyAttackToBtlObj(85, &sEnemy) == ATTACK_RESULT_HIT, 57);
    CHECK(sBattle.pendingHitStop == 3, 58);
    CHECK(sMeleeContacts == 5, 59);
    ++gFrameCounter;
    CHECK(ApplyAttackToBtlObj(93, &sEnemy) == ATTACK_RESULT_HIT, 60);
    CHECK(sBattle.pendingHitStop == 3, 61);
    CHECK(sMeleeContacts == 5, 62);
    ++gFrameCounter;
    CHECK(ApplyAttackToBtlObj(12, &sEnemy) == ATTACK_RESULT_HIT, 63);
    CHECK(sBattle.pendingHitStop == 2, 64);
    BtlSetAttackSource(0);
    return 0;
}

void _start(void) {
    int code = run_tests();
    __asm__ volatile ("int $0x80" : : "a"(1), "b"(code));
    __builtin_unreachable();
}
'''


def run_elf32(path):
    """Execute only this freestanding test when the host disables ELF32."""
    try:
        from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_INTR
        from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ESP
    except ImportError as exc:
        raise SystemExit("This host cannot execute i386 ELF; install the optional "
                         "Python unicorn package to run the collision test.") from exc
    data = path.read_bytes()
    if data[:5] != b"\x7fELF\x01":
        raise SystemExit("Expected a 32-bit ELF test runner")
    entry, phoff = struct.unpack_from("<II", data, 24)
    phsize, phcount = struct.unpack_from("<HH", data, 42)
    segments = []
    pages = set()
    for index in range(phcount):
        kind, offset, address, _, filesize, memsize, _, _ = struct.unpack_from(
            "<8I", data, phoff + index * phsize)
        if kind == 1 and memsize:
            segments.append((offset, address, filesize))
            pages.update(range(address & ~4095, (address + memsize + 4095) & ~4095, 4096))
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    for page in sorted(pages):
        cpu.mem_map(page, 4096)
    for offset, address, size in segments:
        cpu.mem_write(address, data[offset:offset + size])
    cpu.mem_map(0x7FE00000, 0x20000)
    cpu.reg_write(UC_X86_REG_ESP, 0x7FE1FFF0)
    status = []

    def syscall(machine, number, _):
        if number != 0x80 or machine.reg_read(UC_X86_REG_EAX) != 1:
            raise RuntimeError("Unexpected syscall in collision test")
        status.append(machine.reg_read(UC_X86_REG_EBX))
        machine.emu_stop()

    cpu.hook_add(UC_HOOK_INTR, syscall)
    cpu.emu_start(entry, 0, count=1000000)
    if not status:
        raise SystemExit("Collision test exceeded the instruction limit")
    return status[0]


def main():
    includes = [ROOT / "include", ROOT / "port/include"]
    includes += sorted(p for p in (ROOT / "include").iterdir()
                       if p.is_dir() and p.name != "gba")
    with tempfile.TemporaryDirectory(prefix="rekhcom-collision-") as folder:
        folder = Path(folder)
        harness = folder / "collision_runner.c"
        executable = folder / "collision_runner"
        harness.write_text(HARNESS)
        command = [os.environ.get("CC", "gcc"), "-m32", "-O2", "-std=gnu89",
                   "-ffreestanding", "-fno-builtin", "-fno-stack-protector",
                   "-fno-pie", "-ffunction-sections", "-fdata-sections", "-w",
                   "-DPLATFORM_PC", "-DVERSION_EU", "-nostdlib", "-static",
                   "-no-pie", "-Wl,--gc-sections"]
        command += ["-I" + str(path) for path in includes]
        command += [str(ROOT / "src/btl/btl_collision.c"),
                    str(ROOT / "src/listpool.c"), str(harness), "-o", str(executable)]
        subprocess.run(command, cwd=ROOT, check=True)
        try:
            status = subprocess.run([str(executable)], check=False).returncode
        except OSError as exc:
            if exc.errno != errno.ENOEXEC:
                raise
            status = run_elf32(executable)
        if status:
            raise SystemExit("Collision regression failed at check " + str(status))
    print("PASS: 64 real collision routing, damage, per-frame pause, ender, feedback, attack-observer and melee-contact checks")


if __name__ == "__main__":
    main()
