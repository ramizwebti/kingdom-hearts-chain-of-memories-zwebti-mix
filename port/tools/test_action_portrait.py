#!/usr/bin/env python3
"""Check the real portrait palette loader and original destructor lifecycle.

The fixture models the engine's source-pointer palette sharing, then applies
the body's native flash color. It checks the complete source sheet used by
normal, hurt and low-HP frame tile offsets. No ROM data or save is needed.
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(text, name):
    match = re.search(r"^(?:static\s+)?(?:const\s+)?\w+\*?\s+" + name + r"\([^;]*?\)\s*\{", text, re.M)
    assert match, name
    start = text.index("{", match.start())
    depth, end = 1, start + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[match.start():end]


def source_harness():
    source = (ROOT / "src/btl/btl_hpply.c").read_text()
    unit = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "port/port_action_hud.h"
typedef uint16_t u16;
typedef uint8_t u8;
typedef struct ObjPalette {const void*src;u16 color[16];int references;} ObjPalette;
typedef struct BtlHpplyWork {void*tiles,*tiles2,*tiles3,*tiles4;ObjPalette*palette,*palette2;} BtlHpplyWork;
enum {GAME_FLAG_RIKU=1,BTL_FLAG_TUTORIAL=2,BTL_FLAG_VS_BATTLE=4,SONG_SYS_ALART};
static struct {int actionCombat,cardHudRaise;} gPortConfig;
static struct {int wide,width,height;} gPortScreen;
static int gPortHudScale,gPortHudW,gPortHudH,sHudActive;
static int PortPpuHudActive(void){return sHudActive;}
static struct {unsigned flags;} gGameState;
static struct Battle {unsigned flags;} battle;
static struct Battle*gBtlWork=&battle;
static int PortActionCombatRules(void){return gPortConfig.actionCombat &&
 !(gGameState.flags&GAME_FLAG_RIKU) && !(gBtlWork->flags&BTL_FLAG_VS_BATTLE);}
static u16 gSoraPalette[16],sActionSoraPortraitPalette[16];
/* Native source has three faces; only the displayed face occupies0x280VRAM.
 * These are synthetic bytes, not game graphics. */
static u8 gBtlHpSoraFaceTiles[0x680];
static ObjPalette palettes[8];static int used,available=8,failDistinct,lastBudgetCheck;
static int tilesReleased,palettesReleased;
static int CanAllocObjPalette(int count){lastBudgetCheck=count;return available>=count;}
static ObjPalette*LoadObjPalette(const void*src,u16 size){
 int i;assert(size==32);
 if(failDistinct && src==sActionSoraPortraitPalette)return 0;
 for(i=0;i<used;i++)if(palettes[i].src==src){palettes[i].references++;return &palettes[i];}
 if(!available)return 0;
 assert(used<8);palettes[used].src=src;memcpy(palettes[used].color,src,size);
 palettes[used].references=1;available--;return &palettes[used++];
}
static void ReleaseObjPalette(ObjPalette*p){if(p){assert(p->references>0);p->references--;palettesReleased++;}}
static void ReleaseObjTiles(void*p){(void)p;tilesReleased++;}
static void m4aSongNumStop(int song){assert(song==SONG_SYS_ALART);}
'''
    # Compile the real private-buffer declaration so a one-frame-sized source
    # copy fails this regression rather than merely testing a parallel model.
    declaration = re.search(r'^static u8 sActionSoraPortraitTiles\[[^\n]+;', source, re.M)
    assert declaration, 'production portrait source buffer'
    unit += declaration.group().split('__attribute__')[0].rstrip().rstrip(';') + ';\n'
    unit += function(source, "UseActionHud")
    unit += function(source, "SoraHudTiles")
    unit += function(source, "PortActionHudCanvas")
    unit += function(source, "LoadSoraHudPalette")
    unit += function(source, "task_btl_hpply_3")
    unit += r'''
int main(void){
 ObjPalette*body,*face;BtlHpplyWork work={0};int i,w,h,inset;
 gPortConfig.actionCombat=1;gPortScreen.wide=1;gPortScreen.width=320;gPortScreen.height=180;
 assert(PortActionHudCanvas(&w,&h,&inset)&&w==320&&h==180&&inset==0);
 sHudActive=1;gPortHudScale=4;gPortHudW=300;gPortHudH=170;gPortConfig.cardHudRaise=8;
 assert(PortActionHudCanvas(&w,&h,&inset)&&w==300&&h==170&&inset==8);
 gPortConfig.cardHudRaise=99;assert(PortActionHudCanvas(&w,&h,&inset)&&inset==16);
 gPortConfig.cardHudRaise=-1;assert(PortActionHudCanvas(&w,&h,&inset)&&inset==0);
 sHudActive=0;assert(PortActionHudCanvas(&w,&h,&inset)&&w==320&&h==180&&inset==0);
 gPortScreen.wide=0;assert(PortActionHudCanvas(&w,&h,&inset)&&w==240&&h==160&&inset==0);
 gPortConfig.actionCombat=0;assert(!PortActionHudCanvas(&w,&h,&inset));
 assert(!PortActionHudCanvas(0,&h,&inset));
 for(i=0;i<0x680;i++)gBtlHpSoraFaceTiles[i]=(u8)(i*37);
 assert(SoraHudTiles()==gBtlHpSoraFaceTiles);
 gPortConfig.actionCombat=1;
 assert(SoraHudTiles()!=gBtlHpSoraFaceTiles);
 assert(sizeof(sActionSoraPortraitTiles)==sizeof(gBtlHpSoraFaceTiles));
 assert(!memcmp(SoraHudTiles(),gBtlHpSoraFaceTiles,0x680));
 /* Exclusive source ranges read by the three native frame descriptors.
  * The former0x280 copy omitted most hurt and every low-HP face tile. */
 { const unsigned ranges[3][2]={{0,0x240},{0x240,0x460},{0x460,0x680}};
   const u8*portrait=SoraHudTiles();unsigned frame;
   for(frame=0;frame<3;frame++) {
    assert(ranges[frame][1]<=sizeof(sActionSoraPortraitTiles));
    assert(!memcmp(portrait+ranges[frame][0],gBtlHpSoraFaceTiles+ranges[frame][0],
                   ranges[frame][1]-ranges[frame][0]));
   }
 }
 for(i=0;i<16;i++)gSoraPalette[i]=(u16)(i*163);
 body=LoadObjPalette(gSoraPalette,sizeof(gSoraPalette));
 gPortConfig.actionCombat=1;face=LoadSoraHudPalette();
 assert(face && face!=body && face->src!=body->src);
 assert(!memcmp(face->color,body->color,32));assert(lastBudgetCheck==2);
 for(i=1;i<16;i++)body->color[i]=0x3954;
 assert(memcmp(face->color,body->color,32));assert(!memcmp(face->color,gSoraPalette,32));
 work.palette=face;task_btl_hpply_3(&work);
 assert(face->references==0&&body->references==1&&tilesReleased==4&&palettesReleased==1);
 gPortConfig.actionCombat=0;assert(LoadSoraHudPalette()==body);ReleaseObjPalette(body);
 gPortConfig.actionCombat=1;gGameState.flags=GAME_FLAG_RIKU;
 assert(LoadSoraHudPalette()==body);ReleaseObjPalette(body);gGameState.flags=0;
 battle.flags=BTL_FLAG_TUTORIAL;assert(LoadSoraHudPalette()==body);ReleaseObjPalette(body);
 battle.flags=BTL_FLAG_VS_BATTLE;assert(LoadSoraHudPalette()==body);ReleaseObjPalette(body);battle.flags=0;
 available=1;assert(LoadSoraHudPalette()==body);ReleaseObjPalette(body);
 available=6;failDistinct=1;assert(LoadSoraHudPalette()==body);ReleaseObjPalette(body);
 assert(body->references==1);
 puts("PASS native portrait: all three full-source face ranges, palette/lifecycle and bottom-right canvas/fallback layout");
}
'''
    return unit


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", default="gcc")
    args = parser.parse_args()
    unit = source_harness()
    with tempfile.TemporaryDirectory(prefix="rekhcom-action-portrait-") as temp:
        temp = Path(temp)
        cfile = temp / "portrait.c"
        binary = temp / "portrait"
        cfile.write_text(unit)
        subprocess.run([args.cc, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT/'port/include'),
                        "-fsanitize=undefined", str(cfile), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
