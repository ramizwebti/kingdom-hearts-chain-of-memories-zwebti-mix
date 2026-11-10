#!/usr/bin/env python3
"""Run production action-target selection/approach with real 32-bit actors.

Uses the unchanged functions from btl_sora and actual listpool/controller code.
Only tracing and the battle-active predicate are replaced by test sinks.
"""
import errno
import os
from pathlib import Path
import re
import subprocess
import tempfile
from test_action_collision import run_elf32
ROOT=Path(__file__).resolve().parents[2]

def extract(source,name):
    m=re.search(r'(?:static )?(?:BtlObj\*|int|void) '+name+r'\([^;{}]*\)\s*\{',source)
    if not m: raise ValueError(name)
    cursor=m.end();depth=1
    while depth:
        depth+=(source[cursor]=='{')-(source[cursor]=='}');cursor+=1
    return source[m.start():cursor]

HARNESS=r'''
#include "btl.h"
#include "battle_work.h"
#include "battle_actor_types.h"
#include "listpool.h"
#include "port/port_action_target.h"
#include <stddef.h>
static BtlWork sBattle;
BtlWork* gBtlWork=&sBattle;
static BtlSoraWork sSora;
static BtlSoraWork* sActionWork=&sSora;
static BtlObj sEnemies[3];
static BtlObj* sActionTarget;
static PortActionState sActionState;
static PortActionIntent sActionCurrent;
static unsigned sActionIndependent=1,sActive=1,gFrameCounter;
int PortActionCombatActive(void) { return sActive; }
static int ActionTraceEnabled(void) { return 0; }
void PortLog(const char* fmt,...) { (void)fmt; }
void* memset(void* p,int v,size_t n) {
    unsigned char* x=p;while(n--) *x++=(unsigned char)v;return p;
}
__STATES__
__FUNCTIONS__
#define CHECK(test,code) do { if(!(test)) return code; } while(0)
static int run_tests(void) {
    unsigned i;
    PortActionInput in={0};PortActionIntent intent;
    BtlObj* act=&sSora.actor;
    sBattle.actor=act;act->btl=&sBattle;
    ListPoolInit(&sBattle.pool);
    for(i=0;i<3;++i) {
        ListNodeInit(&sEnemies[i].node,&sBattle.pool,&sEnemies[i]);
        ListPoolAppend(&sEnemies[i].node,&sBattle.pool);sEnemies[i].hp=10;
    }
    sEnemies[0].x=32*256;sEnemies[1].x=-32*256;sEnemies[2].y=90*256;
    ActionSelectMeleeTarget(&sSora,0);
    CHECK(sActionTarget==&sEnemies[0],1);
    ActionSelectMeleeTarget(&sSora,PORT_ACTION_LEFT);
    CHECK(sActionTarget==&sEnemies[1],2);
    ActionSelectMeleeTarget(&sSora,PORT_ACTION_RIGHT);
    CHECK(sActionTarget==&sEnemies[0],3);
    sEnemies[0].hp=0;ActionSelectMeleeTarget(&sSora,0);
    CHECK(sActionTarget==&sEnemies[1],4);
    sEnemies[0].hp=10;sEnemies[0].flags=BTLOBJ_FLAG_INTANGIBLE;
    ActionSelectMeleeTarget(&sSora,PORT_ACTION_RIGHT);CHECK(!sActionTarget,5);
    sEnemies[0].flags=BTLOBJ_FLAG_UNHITTABLE;
    ActionSelectMeleeTarget(&sSora,PORT_ACTION_RIGHT);CHECK(!sActionTarget,6);
    sEnemies[0].flags=BTLOBJ_FLAG_DEFEATED;
    ActionSelectMeleeTarget(&sSora,PORT_ACTION_RIGHT);CHECK(!sActionTarget,7);
    sEnemies[0].flags=BTLOBJ_FLAG_INVULNERABLE;
    ActionSelectMeleeTarget(&sSora,PORT_ACTION_RIGHT);CHECK(!sActionTarget,8);
    sEnemies[0].flags=0;sEnemies[0].x=65*256;
    ActionSelectMeleeTarget(&sSora,PORT_ACTION_RIGHT);CHECK(!sActionTarget,9);
    sEnemies[0].x=40*256;sEnemies[0].z=-37*256;
    ActionSelectMeleeTarget(&sSora,PORT_ACTION_RIGHT);CHECK(!sActionTarget,10);
    sEnemies[0].z=0;sEnemies[0].y=33*256;
    ActionSelectMeleeTarget(&sSora,PORT_ACTION_RIGHT);CHECK(!sActionTarget,11);
    sEnemies[0].y=0;sEnemies[0].x=34*256;sEnemies[1].x=32*256;
    sBattle.actor2=&sEnemies[0];ActionSelectMeleeTarget(&sSora,PORT_ACTION_RIGHT);
    CHECK(sActionTarget==&sEnemies[0],12); /* small retention bonus prevents jitter */
    sActionTarget=(BtlObj*)1;CHECK(!ActionLiveMeleeTarget(),13); /* never dereference stale pointer */
    sActionTarget=&sEnemies[0];CHECK(ActionLiveMeleeTarget()==sActionTarget,14);
    ListPoolRemove(&sEnemies[0].node,&sBattle.pool);CHECK(!ActionLiveMeleeTarget(),15);
    ListPoolAppend(&sEnemies[0].node,&sBattle.pool);
    sEnemies[0].x=97*256;CHECK(!ActionLiveMeleeTarget(),16);
    sEnemies[0].x=64*256;sEnemies[0].y=32*256;
    act->z=-5*256;act->originZ=act->z;
    for(i=0;i<30;++i) {
        int x=act->x,y=act->y;
        ActionApproachMeleeTarget(&sSora);
        CHECK(act->x-x>=-768 && act->x-x<=768,17);
        CHECK(act->y-y>=-384 && act->y-y<=384,18);
        CHECK(act->x>=-24*256 && act->x<=24*256,19);
        CHECK(act->y>=-12*256 && act->y<=12*256,20);
        CHECK(act->z==act->originZ,21);
    }
    CHECK(act->x==24*256 && act->y==12*256,22);
    act->x=act->y=act->z=0;act->flags=BTLOBJ_FLAG_FACING_LEFT;
    sEnemies[0].x=-64*256;sEnemies[0].y=-32*256;
    for(i=0;i<30;++i) ActionApproachMeleeTarget(&sSora);
    CHECK(act->x==-24*256 && act->y==-12*256,23);
    act->x=act->y=0;sActionTarget=NULL;
    for(i=0;i<30;++i) ActionApproachMeleeTarget(&sSora);
    CHECK(act->x==-8*256 && act->y==0,24);
    sActionTarget=&sEnemies[0];sActionCurrent.comboStep=1;sSora.state=BTL_SORA_STATE_COMBO;
    CHECK(PortActionLockMeleeTarget() && sBattle.actor2==sActionTarget,25);
    sEnemies[0].hp=0;CHECK(PortActionLockMeleeTarget() && !sBattle.actor2,26);
    sActive=0;CHECK(!PortActionLockMeleeTarget(),27);sActive=1;
    sActionIndependent=0;CHECK(!PortActionLockMeleeTarget(),28);sActionIndependent=1;
    sSora.state=BTL_SORA_STATE_HURT;CHECK(!PortActionLockMeleeTarget(),29);
    /* MP callback is gated by the actual melee actor state and controller token. */
    PortActionReset(&sActionState);sActionState.mp=60;
    in.canAct=1;in.pressed=PORT_ACTION_ATTACK;
    PortActionTick(&sActionState,&in);CHECK(PortActionPeek(&sActionState,&intent),30);
    CHECK(PortActionCommit(&sActionState,&intent),31);
    PortActionPlayerMeleeHit();CHECK(sActionState.mp==60,32); /* hurt cannot collect */
    sSora.state=BTL_SORA_STATE_COMBO;PortActionPlayerMeleeHit();CHECK(sActionState.mp==61,33);
    PortActionPlayerMeleeHit();CHECK(sActionState.mp==61,34);
    sActionState.meleeRewardAvailable=1;sActionCurrent.comboStep=0;
    PortActionPlayerMeleeHit();CHECK(sActionState.mp==61,35);
    sActionCurrent.comboStep=3;sActionState.lastCommand=PORT_ACTION_GROUND_3;
    PortActionPlayerMeleeHit();CHECK(sActionState.mp==66,36);
    act->x=20*256;act->y=0;act->flags=0;sActionTarget=&sEnemies[0];
    sEnemies[0].hp=0;ActionApproachMeleeTarget(&sSora);
    CHECK(act->x==20*256,37); /* losing a target must not pull Sora backward */
    sActionTarget=NULL;ActionApproachMeleeTarget(&sSora);
    CHECK(act->x==20*256,38); /* no backward whiff correction either */
    for(i=0;i<3;++i) sEnemies[i].hp=0;
    ActionSelectMeleeTarget(&sSora,PORT_ACTION_LEFT);
    CHECK(!sActionTarget && (act->flags&BTLOBJ_FLAG_FACING_LEFT),39);
    CHECK(!PortActionApproachStep(30*256,0,0,768,24*256),40);
    /* A connected third swing launches its target beyond basic aim height.
     * The earned follow-up retains that actor without teleporting or lift. */
    act->x=act->y=act->z=act->originX=act->originY=0;act->flags=0;
    sEnemies[0].hp=100;sEnemies[0].flags=0;
    sEnemies[0].x=72*256;sEnemies[0].y=32*256;sEnemies[0].z=-60*256;
    sActionCurrent.finisher=0;ActionSelectMeleeTarget(&sSora,PORT_ACTION_RIGHT);
    CHECK(!sActionTarget,41);
    sActionCurrent.finisher=1;ActionSelectMeleeTarget(&sSora,PORT_ACTION_RIGHT);
    CHECK(sActionTarget==&sEnemies[0] && ActionLiveMeleeTarget()==sActionTarget,42);
    sSora.state=BTL_SORA_STATE_BLITZ;
    CHECK(PortActionLockMeleeTarget() && sBattle.actor2==sActionTarget,43);
    for(i=0;i<30;++i) {
        int x=act->x,y=act->y;
        ActionApproachMeleeTarget(&sSora);
        CHECK(act->x-x>=0 && act->x-x<=5*256 && act->y-y>=0 && act->y-y<=2*256,44);
        CHECK(act->x<=48*256 && act->y<=24*256 && act->z==0,45);
    }
    CHECK(act->x==48*256 && act->y==24*256,46);
    sEnemies[0].z=-81*256;CHECK(!ActionLiveMeleeTarget(),47);
    act->x=act->y=0;ActionSelectMeleeTarget(&sSora,PORT_ACTION_RIGHT);
    CHECK(!sActionTarget,48);
    sEnemies[0].z=0;sEnemies[0].x=97*256;
    ActionSelectMeleeTarget(&sSora,PORT_ACTION_RIGHT);CHECK(!sActionTarget,49);
    return 0;
}
void _start(void) { int code=run_tests();__asm__ volatile("int $0x80"::"a"(1),"b"(code));__builtin_unreachable(); }
'''

def main():
    source=(ROOT/'src/btl/btl_sora.c').read_text()
    start=source.rfind('enum',0,source.index('BTL_SORA_STATE_ENTER,'));states=source[start:source.index('};',start)+2]
    names=['ActionTargetUsable','ActionLiveMeleeTarget','ActionSelectMeleeTarget','PortActionLockMeleeTarget','ActionApproachMeleeTarget','PortActionPlayerMeleeHit']
    functions='\n'.join(extract(source,n) for n in names)
    includes=[ROOT/'include',ROOT/'port/include']+sorted(p for p in (ROOT/'include').iterdir() if p.is_dir() and p.name!='gba')
    with tempfile.TemporaryDirectory(prefix='rekhcom-target-') as folder:
        folder=Path(folder);runner=folder/'target.c';exe=folder/'target'
        runner.write_text(HARNESS.replace('__STATES__',states).replace('__FUNCTIONS__',functions))
        (folder/'string.h').write_text('#include <stddef.h>\nvoid* memset(void*,int,size_t);\n')
        cmd=[os.environ.get('CC','gcc'),'-m32','-O2','-std=gnu89','-ffreestanding','-fno-builtin','-fno-stack-protector','-fno-pie','-ffunction-sections','-fdata-sections','-DPLATFORM_PC','-DVERSION_US','-nostdlib','-static','-no-pie','-Wl,--gc-sections']
        cmd+=['-I'+str(folder)]+['-I'+str(p) for p in includes]
        subprocess.run(cmd+[str(runner),str(ROOT/'src/listpool.c'),str(ROOT/'port/src/port_action.c'),'-o',str(exe)],check=True)
        try: status=subprocess.run([str(exe)],check=False).returncode
        except OSError as e:
            if e.errno!=errno.ENOEXEC: raise
            status=run_elf32(exe)
        if status: raise SystemExit('FAIL target/flow check '+str(status))
    print('PASS: 49 production target, launched finisher retention, bounded chase, no vertical lift and melee MP adapter checks')
if __name__=='__main__':main()
