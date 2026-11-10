#!/usr/bin/env python3
"""Run the unchanged battle-entry function with actual 32-bit game state."""
import errno
from pathlib import Path
import subprocess
import tempfile
from test_action_cardfree import function
from test_action_collision import run_elf32

ROOT=Path(__file__).resolve().parents[2]
HARNESS=r'''
#include "game_state.h"
#include "mode_chkbtl.h"
#include "mode.h"
#include "gba/keys.h"
GameState gGameState;
static ChkBtlWork sWork;
ChkBtlWork* gChkBtlWork=&sWork;
static const ChkBtlEntry sChkBtlEntries[1]={{7,0,42,0,"fixture"}};
static const ChkBtlWorld sChkBtlWorlds[1]={{3,"fixture"}};
Mode gModeBattle;
vu32 gFrameCounter;
static int sPractice,sRequested,sRiku;
static u16 sKeys;
u16 GetKeysHeld(void) { return sKeys; }
int PortPracticeActive(void) { return sPractice; }
void SeedRandom(u32 seed) { (void)seed; }
void InitRikuDeckForWorld(u8 floor) { sRiku=floor+1; }
void ModeRequest(Mode* mode,s32 arg) { sRequested=(mode==&gModeBattle)?arg:-1; }
__FUNCTION__
#define CHECK(v,n) do { if(!(v)) return n; } while(0)
static int tests(void) {
    sWork.hp=560;sWork.floor=2;
    sPractice=1;sKeys=L_BUTTON;gGameState.flags=GAME_FLAG_FIRST_STRIKE;
    StartSelectedBattle();
    CHECK(!(gGameState.flags&GAME_FLAG_FIRST_STRIKE),1);
    CHECK(gGameState.hp==560&&gGameState.progression.maxHp==560,2);
    CHECK(gGameState.world==7&&gGameState.battleStage==3&&gGameState.floor==2,3);
    CHECK(sRequested==42&&!sRiku,4);
    sPractice=0;StartSelectedBattle();
    CHECK(gGameState.flags&GAME_FLAG_FIRST_STRIKE,5);
    sKeys=0;StartSelectedBattle();
    CHECK(!(gGameState.flags&GAME_FLAG_FIRST_STRIKE),6);
    sPractice=1;sKeys=L_BUTTON;gGameState.flags=GAME_FLAG_RIKU|GAME_FLAG_FIRST_STRIKE;
    StartSelectedBattle();
    CHECK(!(gGameState.flags&GAME_FLAG_FIRST_STRIKE)&&sRiku==3,7);
    CHECK(gGameState.flags&GAME_FLAG_RIKU,8);
    return 0;
}
void _start(void) {
    int code=tests();
    __asm__ volatile("int $0x80"::"a"(1),"b"(code));
    __builtin_unreachable();
}
'''

def main():
    body=function((ROOT/"src/debug/mode_chkbtl.c").read_text(),"StartSelectedBattle")
    includes=[ROOT/"include",ROOT/"port/include"]
    includes+=sorted(p for p in (ROOT/"include").iterdir() if p.is_dir() and p.name!="gba")
    with tempfile.TemporaryDirectory(prefix="practice-rules-") as temp:
        folder=Path(temp);source=folder/"test.c";exe=folder/"test"
        source.write_text(HARNESS.replace("__FUNCTION__",body))
        cmd=["gcc","-m32","-O2","-std=gnu89","-ffreestanding","-fno-builtin",
             "-fno-stack-protector","-fno-pie","-ffunction-sections","-fdata-sections",
             "-DPLATFORM_PC","-DVERSION_US","-nostdlib","-static","-no-pie","-Wl,--gc-sections"]
        subprocess.run(cmd+["-I"+str(p) for p in includes]+[str(source),"-o",str(exe)],check=True)
        try:code=subprocess.run([str(exe)]).returncode
        except OSError as exc:
            if exc.errno!=errno.ENOEXEC:raise
            code=run_elf32(exe)
        if code:raise SystemExit("practice rule check failed: "+str(code))
    print("PASS: 8 actual battle-entry rules, practice first-strike suppression and classic fallback")
if __name__=="__main__":main()
