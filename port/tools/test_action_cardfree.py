#!/usr/bin/env python3
"""Exercise the real boss-reaction entry points with the game's 32-bit ABI.

Only the unchanged function bodies under test are extracted from battle_runtime;
all battle fields, card IDs and song IDs come from real project headers. Classic
and action paths are both executed. Actual tutorial transitions are exercised by
the separate native integration suite.
"""
import errno
import os
from pathlib import Path
import subprocess
import tempfile
from test_action_collision import run_elf32

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    start = source.index("void " + name + "(")
    opening = source.index("{", start)
    depth = 1
    cursor = opening + 1
    while depth:
        depth += (source[cursor] == "{") - (source[cursor] == "}")
        cursor += 1
    return source[start:cursor]


HARNESS = r'''
#include "battle_work.h"
#include "card_ids.h"
#include "songs.h"

static BtlWork sBattle;
BtlWork* gBtlWork = &sBattle;
static int sAction, sCards, sSounds;
static unsigned sLastCard;
int PortActionCombatRules(void) { return sAction; }
void PortLog(const char* fmt, ...) { (void)fmt; }
void m4aSongNumStart(unsigned short song) { if (song == SONG_BTL_GMIC_OK) sSounds++; }
void CreateGimmickCardTask(TaskPool* pool, int x, int y, int z, unsigned short id) {
    (void)pool; (void)x; (void)y; (void)z;
    sCards++; sLastCard = id;
}
__FUNCTIONS__

#define CHECK(test, code) do { if (!(test)) return (code); } while (0)
static int run_tests(void) {
    unsigned i;
    const unsigned cards[5] = { CARD_GIMMICK_0, CARD_GIMMICK_1, CARD_GIMMICK_2,
                               CARD_GIMMICK_3, CARD_GIMMICK_4 };
    sAction = 1;
    for (i = 0; i < 5; ++i) {
        sBattle.gimmickFlags = 0;
        DropGimmickCard(i, 1, 2, 3);
        CHECK(sBattle.gimmickFlags == (1u << i), 1+i);
        CHECK(sCards == 0, 6+i);
        CHECK(sSounds == i+1, 11+i);
    }
    DropGimmickCard(5, 1, 2, 3);
    DropGimmickCard(255, 1, 2, 3);
    CHECK(sBattle.gimmickFlags == 16 && sCards == 0 && sSounds == 5, 16);
    sAction = 0;
    sBattle.gimmickFlags = 0;
    for (i = 0; i < 5; ++i) {
        DropGimmickCard(i, 1, 2, 3);
        CHECK(sCards == i+1 && sLastCard == cards[i], 17+i);
        CHECK(sBattle.gimmickFlags == 0 && sSounds == 5, 22+i);
    }
    DropGimmickCard(5, 1, 2, 3);
    CHECK(sCards == 5, 27);
    return 0;
}
void _start(void) {
    int code = run_tests();
    __asm__ volatile ("int $0x80" : : "a"(1), "b"(code));
    __builtin_unreachable();
}
'''


def main():
    source = (ROOT / "src/btl/battle_runtime.c").read_text()
    functions = "\n".join(function(source, name) for name in ("SetGimmickFlag", "DropGimmickCard"))
    includes = [ROOT / "include", ROOT / "port/include"]
    includes += sorted(p for p in (ROOT / "include").iterdir() if p.is_dir() and p.name != "gba")
    with tempfile.TemporaryDirectory(prefix="rekhcom-cardfree-") as folder:
        folder = Path(folder)
        src, executable = folder / "runner.c", folder / "runner"
        src.write_text(HARNESS.replace("__FUNCTIONS__", functions))
        command = [os.environ.get("CC", "gcc"), "-m32", "-O2", "-std=gnu89",
                   "-ffreestanding", "-fno-builtin", "-fno-stack-protector", "-fno-pie",
                   "-DPLATFORM_PC", "-DVERSION_US", "-nostdlib", "-static", "-no-pie"]
        command += ["-I" + str(p) for p in includes]
        subprocess.run(command + [str(src), "-o", str(executable)], cwd=ROOT, check=True)
        try:
            status = subprocess.run([str(executable)], check=False).returncode
        except OSError as error:
            if error.errno != errno.ENOEXEC: raise
            status = run_elf32(executable)
        if status: raise SystemExit("Boss reaction regression failed at check " + str(status))
    print("PASS: 27 real boss-reaction checks; action triggers without cards, classic drops preserved")


if __name__ == "__main__":
    main()
