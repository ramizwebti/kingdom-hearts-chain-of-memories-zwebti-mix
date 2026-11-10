#!/usr/bin/env python3
"""Exercise production field steering and jump/landing branches without a ROM.

Extract the actual changed functions and state branches so the fixture catches
incorrect engine state transitions, preserved scripted jumps and legacy gates.
Collision, room transitions and camera still require the native game harness.
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(text, name):
    match = re.search(r"^(?:static\s+)?\w+\s+" + name + r"\([^;]*?\)\s*\{", text, re.M)
    assert match, name
    start = text.index("{", match.start())
    depth, end = 1, start + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[match.start():end]


def enum(path, name):
    text = path.read_text()
    match = re.search(r"enum " + name + r"\s*\{[^}]*\};", text, re.S)
    assert match, name
    return match.group()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", default="gcc")
    args = parser.parse_args()
    source = (ROOT / "src/map/fld_sora.c").read_text()
    jump = function(source, "FldSoraJump")
    jump_start = jump[jump.index("    case FLD_STATE_JUMP_START:"):jump.index("    case FLD_STATE_JUMP_RISE:")]
    landing = jump[jump.index("    case FLD_STATE_LAND:"):jump.index("\n    }\n\n    if (work->collider.colliding)")]
    enums = "\n".join((enum(ROOT / "include/map/fld_types.h", "FldAngle"),
                       enum(ROOT / "include/map/fld_types.h", "FldFlag"),
                       enum(ROOT / "include/map/fld_types.h", "FldState"),
                       enum(ROOT / "include/map/field_state.h", "FieldFlag"),
                       enum(ROOT / "include/map/map_types.h", "RoomFlag")))
    prefix = r'''
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "port/port_action_motion.h"
#include "gba/keys.h"
#define PLATFORM_PC 1
typedef uint8_t u8; typedef int8_t s8; typedef uint16_t u16;
typedef int32_t s32; typedef uint32_t u32;
typedef struct Mode { int id; } Mode;
static Mode gModeMapFld={1},gModeMapFix={2},eventMode={3};
static Mode* gCurrentMode=&gModeMapFld;
static struct {int actionCombat;} gPortConfig={1};
typedef struct FldActor {s32 speed;u8 angle;struct {s32 x,y,z;} fieldPosition;} FldActor;
typedef struct FldWork {u32 state;u16 flags;s32 timer,vz,targetZ;u16 sounds[8];} FldWork;
static int gSineTable[320];
static struct Field {u32 flags;FldActor actor;} field;
static struct Field* gFieldState=&field;
static struct Room {u32 flags;s32 jumpGmkHeight;u8 jumpGmkAngle;} room;
static struct Room* gMapRoomState=&room;
static u16 held,pressed;static u8 released[256];
static u16 GetKeysHeld(void){return held;}
static u16 GetKeysPressed(void){return pressed;}
static u8 GetKeyReleaseTime(u16 key){return released[(key>>4)&255];}
static u8 GetAngleDiff(u8 a,u8 b){return b-a;}
static void FldSoraSetAnim(FldWork*w,int a,int b){(void)w;(void)a;(void)b;}
static void m4aSongNumStart(int a){(void)a;}
static int GetRandom(void){return 1;}
static void FldSoraGmkJump(void){}
static void task_fld_sora_1(void){}
typedef void (*TaskUpdateFunc)(void);static TaskUpdateFunc update;
static void SetTaskUpdate(void*t,TaskUpdateFunc fn){(void)t;update=fn;}
enum {SONG_SYS_SR_I_VO00,SONG_SYS_SR_I_VO01};
'''
    functions = "\n".join(function(source, name) for name in
                          ("ActionFieldFreeControl", "ActionFieldSpeed", "FldSoraSetAngleFromDpad", "FldSoraTurn"))
    unit = prefix + enums + functions + r'''
static void StartJump(FldWork*work){
 FldActor*act=&gFieldState->actor;void*task=0;
 int actionMovement=ActionFieldFreeControl() && !(work->flags&FLD_FLAG_NO_AIR_TURN);
 switch(work->state){
''' + jump_start + r'''
 default:break;}
}
static void Land(FldWork*work){
 FldActor*act=&gFieldState->actor;void*task=0;
 int actionMovement=ActionFieldFreeControl() && !(work->flags&FLD_FLAG_NO_AIR_TURN);
 switch(work->state){
''' + landing + r'''
 default:break;}
}
int main(void){
 int i,speed;FldActor actor={0};FldWork work={0};
 const u32 blocked[]={FIELD_FLAG_FREEZE_PLAYER,FIELD_FLAG_MENU_OPEN,FIELD_FLAG_ROOM_CREATE,
  FIELD_FLAG_AUTO_WALK,FIELD_FLAG_EXIT_ROOM,0x4000};
 const u32 roomBlocked[]={ROOM_FLAG_SAVE_MENU_OPEN,ROOM_FLAG_ENEMY_STRUCK,ROOM_FLAG_TUTORIAL_ACTIVE};
 assert(ActionFieldFreeControl());gPortConfig.actionCombat=0;assert(!ActionFieldFreeControl());
 gPortConfig.actionCombat=1;gCurrentMode=&eventMode;assert(!ActionFieldFreeControl());
 gCurrentMode=&gModeMapFix;assert(ActionFieldFreeControl());gCurrentMode=&gModeMapFld;
 for(i=0;i<(int)(sizeof(blocked)/sizeof(*blocked));i++){field.flags=blocked[i];assert(!ActionFieldFreeControl());}
 field.flags=0;
 for(i=0;i<(int)(sizeof(roomBlocked)/sizeof(*roomBlocked));i++){room.flags=roomBlocked[i];assert(!ActionFieldFreeControl());}
 room.flags=0;gFieldState=0;assert(!ActionFieldFreeControl());gFieldState=&field;
 gMapRoomState=0;assert(!ActionFieldFreeControl());gMapRoomState=&room;
 /* A released diagonal must stop steering immediately; legacy keeps grace. */
 held=DPAD_DOWN;actor.angle=FLD_ANGLE_DOWN_LEFT;FldSoraSetAngleFromDpad(&actor);assert(actor.angle==FLD_ANGLE_DOWN);
 gPortConfig.actionCombat=0;FldSoraSetAngleFromDpad(&actor);assert(actor.angle==FLD_ANGLE_DOWN_LEFT);
 gPortConfig.actionCombat=1;
 held=DPAD_UP|DPAD_RIGHT;FldSoraSetAngleFromDpad(&actor);assert(actor.angle==FLD_ANGLE_UP_RIGHT);
 held=DPAD_DOWN|DPAD_RIGHT;FldSoraSetAngleFromDpad(&actor);assert(actor.angle==FLD_ANGLE_DOWN_RIGHT);
 held=DPAD_UP|DPAD_LEFT;FldSoraSetAngleFromDpad(&actor);assert(actor.angle==FLD_ANGLE_UP_LEFT);
 actor.angle=FLD_ANGLE_RIGHT;actor.speed=716;held=DPAD_LEFT;FldSoraTurn(&actor);assert(actor.speed==716&&actor.angle==FLD_ANGLE_LEFT);
 for(i=0;i<256;i++)released[i]=255;
 gPortConfig.actionCombat=0;actor.angle=FLD_ANGLE_RIGHT;FldSoraTurn(&actor);assert(actor.speed==0);
 gPortConfig.actionCombat=1;
 speed=0;for(i=0;i<4;i++){speed=ActionFieldSpeed(speed,DPAD_RIGHT,0);}assert(speed==716);
 for(i=0;i<100;i++){speed=ActionFieldSpeed(speed,DPAD_RIGHT,0);}assert(speed==716);
 for(i=0;i<3;i++){speed=ActionFieldSpeed(speed,0,0);}assert(speed==0);
 assert(ActionFieldSpeed(INT_MAX,DPAD_RIGHT,0)==716);
 speed=0;for(i=0;i<8;i++){speed=ActionFieldSpeed(speed,DPAD_RIGHT,1);}assert(speed==512);
 for(i=0;i<6;i++){speed=ActionFieldSpeed(speed,0,1);}assert(speed==0);
 /* Real ordinary jump branch launches after two updates, with original vz. */
 held=0;work.state=FLD_STATE_JUMP_START;field.actor.speed=600;
 StartJump(&work);assert(work.state==FLD_STATE_JUMP_START&&field.actor.speed==300);
 StartJump(&work);assert(work.state==FLD_STATE_JUMP_RISE&&work.vz==-1265&&field.actor.speed==600);
 /* Spring jump keeps the authored five-update startup and target/angle. */
 work=(FldWork){0};work.state=FLD_STATE_JUMP_START;room.jumpGmkHeight=0x4000;room.jumpGmkAngle=FLD_ANGLE_UP_LEFT;
 for(i=0;i<4;i++){StartJump(&work);assert(work.state==FLD_STATE_JUMP_START);}
 StartJump(&work);assert(work.state==FLD_STATE_GMK_JUMP_START&&update==FldSoraGmkJump);
 assert(work.targetZ==field.actor.fieldPosition.z-0x4000&&field.actor.angle==FLD_ANGLE_UP_LEFT);
 /* Scripted recovery with NO_AIR_TURN still takes the original startup. */
 room.jumpGmkHeight=0;work=(FldWork){0};work.state=FLD_STATE_JUMP_START;work.flags=FLD_FLAG_NO_AIR_TURN;
 for(i=0;i<4;i++){StartJump(&work);assert(work.state==FLD_STATE_JUMP_START);}
 StartJump(&work);assert(work.state==FLD_STATE_JUMP_RISE);
 /* Landing control returns sooner, while B still requests the real jump task. */
 work=(FldWork){0};work.state=FLD_STATE_LAND;field.flags=FIELD_FLAG_PLAYER_JUMPING;pressed=0;
 for(i=0;i<3;i++){Land(&work);assert(work.state==FLD_STATE_LAND);}
 Land(&work);assert(work.state==FLD_STATE_GROUND&&update==task_fld_sora_1&&!(field.flags&FIELD_FLAG_PLAYER_JUMPING));
 work.state=FLD_STATE_LAND;work.flags=FLD_FLAG_NO_AIR_TURN;work.timer=0;pressed=B_BUTTON;
 Land(&work);assert(work.state==FLD_STATE_JUMP_START&&!(work.flags&FLD_FLAG_NO_AIR_TURN));
 puts("PASS actual field control gates, isometric turns, acceleration/braking, ordinary/scripted jump and landing transitions");
}
'''
    with tempfile.TemporaryDirectory(prefix="rekhcom-action-field-") as temp:
        temp = Path(temp)
        cfile = temp / "field.c"
        cfile.write_text(unit)
        binary = temp / "field"
        subprocess.run([args.cc, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                        "-fsanitize=undefined", "-I"+str(ROOT / "port/include"),
                        "-I"+str(ROOT / "include"), str(cfile), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
