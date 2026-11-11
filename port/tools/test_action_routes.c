/* Controller-shaped requests: held/released faces and analog-direction intent. */
#include "port/port_action.h"
#include "port/port_action_routes.h"
#include "port/port_action_catalog.h"
#include "card_ids.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static PortActionInput Input(unsigned pressed,unsigned held,int ready) {
    PortActionInput value={0};value.pressed=(uint16_t)pressed;value.held=(uint16_t)held;value.canAct=(uint8_t)ready;return value;
}
static void Tick(PortActionState* state,unsigned pressed,unsigned held,int ready) {
    PortActionInput input=Input(pressed,held,ready);PortActionTick(state,&input);
}
static PortActionIntent Commit(PortActionState* state) {
    PortActionIntent intent;assert(PortActionPeek(state,&intent));assert(PortActionCommit(state,&intent));return intent;
}
static void ThreeNormals(PortActionState* state) {
    for(unsigned n=1;n<=3;++n) {
        Tick(state,PORT_ACTION_ATTACK,PORT_ACTION_ATTACK,1);
        PortActionIntent intent=Commit(state);assert(intent.command==n && intent.comboStep==n);
        Tick(state,0,0,0);
    }
}
static void TestTree(void) {
    const unsigned directions[]={0,PORT_ACTION_RIGHT,PORT_ACTION_LEFT,PORT_ACTION_UP,PORT_ACTION_DOWN};
    const uint32_t taps[]={MOVE_BLITZ,MOVE_SLIDING_DASH,MOVE_STRIKE_RAID,MOVE_RAGNAROK,MOVE_STUN_IMPACT};
    const uint32_t holds[]={MOVE_ARS_ARCANUM,MOVE_SONIC_BLADE,MOVE_ZANTETSUKEN,MOVE_RAGNAROK,MOVE_STUN_IMPACT};
    for(unsigned branch=0;branch<5;++branch) for(unsigned hold=0;hold<2;++hold) {
        PortActionState state;PortActionReset(&state);ThreeNormals(&state);
        Tick(&state,PORT_ACTION_ATTACK,PORT_ACTION_ATTACK|directions[branch],1);
        PortActionIntent intent;assert(!PortActionPeek(&state,&intent));
        for(unsigned n=1;n<(hold?16u:2u);++n) Tick(&state,0,PORT_ACTION_ATTACK|directions[branch],1);
        Tick(&state,0,0,1);intent=Commit(&state);
        assert(intent.command==PORT_ACTION_NATIVE_TECHNIQUE && intent.finisher && !intent.comboStep);
        assert(intent.nativeMove==(hold?holds[branch]:taps[branch]));
        /* A held face cannot repeat or restart the accepted native action. */
        for(unsigned n=0;n<240;++n) { Tick(&state,0,PORT_ACTION_ATTACK,0);assert(!PortActionPeek(&state,&intent)); }
    }
    assert(PortActionRouteFinisher(0,1,0,0,0,0)==MOVE_SONIC_BLADE);
    assert(PortActionRouteFinisher(1,0,0,0,0,1)==MOVE_BLIZZARD_RAID);
    assert(PortActionRouteFinisher(0,0,0,0,16,1)==MOVE_FIRAGA_BREAK);
    assert(PortActionRouteFinisher(2,0,0,0,16,1)==MOVE_SHOCK_IMPACT);
    assert(PortActionRouteFinisher(3,0,0,0,16,1)==MOVE_TRINITY_LIMIT);
    assert(PortActionRouteFinisher(0,0,PORT_ACTION_LEFT,1,0,0)==MOVE_SLIDING_DASH);
}
static void TestMagic(void) {
    for(unsigned tier=0;tier<3;++tier) {
        PortActionState state;PortActionReset(&state);unsigned ticks=tier==2?30:tier==1?16:2;
        Tick(&state,PORT_ACTION_MAGIC,PORT_ACTION_MAGIC,1);
        for(unsigned n=1;n<ticks;++n) Tick(&state,0,PORT_ACTION_MAGIC,1);
        Tick(&state,0,0,1);PortActionIntent intent=Commit(&state);
        assert(intent.command==(tier?PORT_ACTION_NATIVE_TECHNIQUE:PORT_ACTION_FIRE));
        if(tier) assert(intent.nativeMove==(tier==1?MOVE_FIRA:MOVE_FIRAGA));
        assert(state.mp==PORT_ACTION_MAX_MP-intent.mpCost);
    }
    PortActionState state;PortActionReset(&state);state.mp=0;state.recharging=1;
    Tick(&state,PORT_ACTION_MAGIC,PORT_ACTION_MAGIC,1);Tick(&state,0,0,1);
    PortActionIntent intent;assert(!PortActionPeek(&state,&intent));assert(state.denial==PORT_ACTION_DENIAL_RECHARGE);
    assert(PortActionRouteMagic(1,PORT_ACTION_UP,0,0,1)==MOVE_AQUA_SPLASH);
    assert(PortActionRouteMagic(4,PORT_ACTION_DOWN,0,0,1)==MOVE_WARP);
    assert(PortActionRouteMagic(5,PORT_ACTION_LEFT,0,0,1)==MOVE_CONFUSE);
}
static void TestAssist(void) {
    PortActionState state;PortActionReset(&state);uint32_t move;
    Tick(&state,PORT_ACTION_ASSIST,PORT_ACTION_ASSIST,1);Tick(&state,0,0,1);
    assert(!PortActionPendingAssist(&state,&move) && !state.assistGauge);
    ThreeNormals(&state);for(unsigned n=0;n<3;++n) { state.meleeRewardAvailable=1;PortActionRestoreOnHit(&state); }
    assert(state.assistGauge>0 && state.assistGauge<100);
    PortActionAddAssistGauge(&state,100);assert(state.assistGauge==100);
    Tick(&state,PORT_ACTION_ASSIST,PORT_ACTION_ASSIST,1);Tick(&state,0,0,1);
    assert(PortActionPendingAssist(&state,&move) && move==MOVE_SIMBA && state.assistGauge==100);
    assert(!PortActionCommitAssist(&state,MOVE_GENIE) && state.assistGauge==100);
    assert(PortActionCommitAssist(&state,move) && state.assistGauge==0);
    assert(!PortActionCommitAssist(&state,move));
    assert(PortActionRouteAssist(2,0,1)==MOVE_IDYLL_ROMP);
    assert(PortActionRouteAssist(6,0,1)==MOVE_CROSS_SLASH_PLUS);
    assert(PortActionRouteAssist(7,0,1)==MOVE_BLAZING_DONALD);
    Tick(&state,PORT_ACTION_PREV_ASSIST|PORT_ACTION_PREV_WEAPON,0,1);
    assert(state.assistFamily==13 && PortActionKeybladeMove(&state)==MOVE_ONE_WINGED_ANGEL);
    Tick(&state,PORT_ACTION_NEXT_ASSIST|PORT_ACTION_NEXT_WEAPON,0,1);
    assert(state.assistFamily==0 && PortActionKeybladeMove(&state)==MOVE_KINGDOM_KEY);
    PortActionAddAssistGauge(&state,35);PortActionInterrupt(&state);assert(state.assistGauge==35);
    PortActionQueueAssist(&state,MOVE_OMNISLASH,1);assert(PortActionPendingAssist(&state,&move));
    assert(PortActionCommitAssist(&state,move) && state.assistGauge==35);
}
static void TestSpellWeave(void) {
    PortActionState state;PortActionReset(&state);
    Tick(&state,PORT_ACTION_ATTACK,PORT_ACTION_ATTACK,1);assert(Commit(&state).comboStep==1);
    Tick(&state,0,0,0);
    Tick(&state,PORT_ACTION_MAGIC,PORT_ACTION_MAGIC,1);Tick(&state,0,0,1);
    assert(Commit(&state).command==PORT_ACTION_FIRE && state.comboStep==1);
    for(unsigned n=0;n<90;++n) Tick(&state,0,0,0);
    Tick(&state,PORT_ACTION_ATTACK,PORT_ACTION_ATTACK,1);assert(Commit(&state).comboStep==2);
    Tick(&state,0,0,0);
    Tick(&state,PORT_ACTION_MAGIC,PORT_ACTION_MAGIC,1);
    for(unsigned n=1;n<16;++n) Tick(&state,0,PORT_ACTION_MAGIC,1);
    Tick(&state,0,0,1);assert(Commit(&state).nativeMove==MOVE_FIRA && state.comboStep==2);
    Tick(&state,PORT_ACTION_ATTACK,PORT_ACTION_ATTACK,1);assert(Commit(&state).comboStep==3);
    Tick(&state,0,0,0);Tick(&state,PORT_ACTION_ATTACK,PORT_ACTION_ATTACK,1);Tick(&state,0,0,1);
    assert(Commit(&state).nativeMove==MOVE_BLITZ && !state.comboStep);
}
static void TestRequestLifetime(void) {
    PortActionState state;PortActionIntent intent;uint32_t native;
    PortActionReset(&state);
    Tick(&state,PORT_ACTION_ATTACK,PORT_ACTION_LEFT,0);
    assert(state.bufferAgeFrames==PORT_ACTION_BUFFER_MAX_FRAMES);
    for(unsigned n=1;n<PORT_ACTION_BUFFER_MAX_FRAMES;++n) {
        Tick(&state,0,0,0);assert(state.bufferFrames && !PortActionPeek(&state,&intent));
    }
    Tick(&state,0,0,0);assert(!state.bufferFrames && !state.bufferAgeFrames);
    Tick(&state,0,0,1);assert(!PortActionPeek(&state,&intent));

    /* An early request still survives a whole ordinary busy phase; its
     * direction is the accepted press, not whatever movement is held later. */
    Tick(&state,PORT_ACTION_ATTACK,PORT_ACTION_LEFT,0);
    for(unsigned n=0;n<20;++n) Tick(&state,0,PORT_ACTION_RIGHT,0);
    Tick(&state,0,0,1);assert(state.bufferedHeld==PORT_ACTION_LEFT);
    assert(Commit(&state).comboStep==1 && !state.bufferAgeFrames);

    /* Effect-channel contention cannot keep a spell/native request alive
     * forever, whether the actor is neutral or locked in another action. */
    for(unsigned ready=0;ready<2;++ready) {
        PortActionReset(&state);
        PortActionInput input=Input(PORT_ACTION_MAGIC,PORT_ACTION_MAGIC,ready);
        input.spellBusy=1;PortActionTick(&state,&input);
        input.pressed=input.held=0;PortActionTick(&state,&input);
        assert(state.magicQueued && state.mp==PORT_ACTION_MAX_MP);
        for(unsigned n=0;n<PORT_ACTION_BUFFER_MAX_FRAMES;++n) PortActionTick(&state,&input);
        assert(!state.magicQueued && !state.bufferFrames && !state.bufferAgeFrames);
        input.spellBusy=0;input.canAct=1;PortActionTick(&state,&input);
        assert(!PortActionPeek(&state,&intent) && state.mp==PORT_ACTION_MAX_MP);
    }
    PortActionQueueNative(&state,MOVE_BLITZ);
    assert(PortActionPendingNative(&state,&native) && native==MOVE_BLITZ);
    for(unsigned n=0;n<PORT_ACTION_BUFFER_MAX_FRAMES;++n) Tick(&state,0,0,0);
    assert(!PortActionPendingNative(&state,&native));

    /* Releasing a queued finisher keeps the gesture's captured facing even
     * when Sora is facing the other way at the eventual transition. */
    PortActionReset(&state);ThreeNormals(&state);
    PortActionInput press=Input(PORT_ACTION_ATTACK,PORT_ACTION_ATTACK|PORT_ACTION_LEFT,0);
    press.facingLeft=1;PortActionTick(&state,&press);
    Tick(&state,0,0,0);
    assert(state.bufferedFacing==1 && state.bufferedHeld==(PORT_ACTION_ATTACK|PORT_ACTION_LEFT));
    Tick(&state,0,PORT_ACTION_RIGHT,1);assert(Commit(&state).nativeMove==MOVE_SLIDING_DASH);
}
static void TestDefensiveContinuation(void) {
    PortActionState state;PortActionIntent intent;
    PortActionReset(&state);
    Tick(&state,PORT_ACTION_ATTACK,PORT_ACTION_ATTACK,1);assert(Commit(&state).comboStep==1);
    state.mp=50;PortActionRestoreOnHit(&state);
    unsigned mp=state.mp,gauge=state.assistGauge;
    Tick(&state,PORT_ACTION_ATTACK,PORT_ACTION_ATTACK,1);
    assert(PortActionPeek(&state,&intent));
    PortActionDefend(&state);
    assert(!PortActionCommit(&state,&intent));
    assert(state.comboStep==1 && state.defenseComboFrames==PORT_ACTION_DEFENSE_COMBO_FRAMES);
    assert(!state.bufferFrames && !state.attackCharging && !state.magicCharging);
    assert(!state.confirmedChain && !state.meleeRewardAvailable && !state.counterFrames);
    PortActionRestoreOnHit(&state);assert(state.mp==mp && state.assistGauge==gauge);
    for(unsigned n=0;n<18;++n) Tick(&state,0,0,0);
    Tick(&state,PORT_ACTION_ATTACK,PORT_ACTION_ATTACK,1);
    assert(Commit(&state).comboStep==2 && !state.defenseComboFrames);

    /* Holding/re-entering defense may not refresh a ready third-hit chain. */
    PortActionReset(&state);ThreeNormals(&state);PortActionDefend(&state);
    for(unsigned n=0;n<PORT_ACTION_DEFENSE_COMBO_FRAMES;++n) {
        PortActionDefend(&state);Tick(&state,0,0,0);
    }
    assert(!state.comboStep && !state.comboFrames && !state.defenseComboFrames);
    Tick(&state,PORT_ACTION_ATTACK,PORT_ACTION_ATTACK,1);assert(Commit(&state).comboStep==1);

    /* Legitimate three-piece completion still earns a finisher on misses,
     * including immediately after a voluntary defensive escape. */
    PortActionReset(&state);ThreeNormals(&state);PortActionDefend(&state);
    for(unsigned n=0;n<18;++n) Tick(&state,0,0,0);
    Tick(&state,PORT_ACTION_ATTACK,PORT_ACTION_ATTACK,1);Tick(&state,0,0,1);
    intent=Commit(&state);assert(intent.finisher && intent.nativeMove==MOVE_BLITZ);
    assert(!state.comboStep && !state.defenseComboFrames);

    /* Damage overrides retained sequence/charge, while released assist
     * ownership and its unspent gauge keep their original safe semantics. */
    PortActionReset(&state);ThreeNormals(&state);PortActionDefend(&state);
    PortActionAddAssistGauge(&state,100);PortActionQueueAssist(&state,MOVE_SIMBA,0);
    PortActionInput hurt=Input(PORT_ACTION_ATTACK,PORT_ACTION_ATTACK,1);hurt.interrupted=1;
    PortActionTick(&state,&hurt);
    assert(!state.comboStep && !state.defenseComboFrames && !state.bufferFrames && !state.attackCharging);
    uint32_t assist;assert(PortActionPendingAssist(&state,&assist) && assist==MOVE_SIMBA);
    assert(state.assistGauge==100);
}
static void TestRecoveryGrace(void) {
    PortActionState state;PortActionIntent intent;
    PortActionReset(&state);
    Tick(&state,PORT_ACTION_ATTACK,PORT_ACTION_ATTACK,1);assert(Commit(&state).comboStep==1);
    PortActionInput recovery=Input(0,0,1);recovery.comboGracePaused=1;
    for(unsigned n=0;n<20;++n) PortActionTick(&state,&recovery);
    assert(state.comboStep==1 && state.comboFrames==PORT_ACTION_COMBO_FRAMES);
    recovery.pressed=PORT_ACTION_MAGIC;recovery.held=PORT_ACTION_MAGIC;recovery.spellBusy=1;
    PortActionTick(&state,&recovery);recovery.pressed=recovery.held=0;
    PortActionTick(&state,&recovery);
    for(unsigned n=0;n<PORT_ACTION_BUFFER_MAX_FRAMES;++n) PortActionTick(&state,&recovery);
    assert(state.comboStep==1 && state.comboFrames==PORT_ACTION_COMBO_FRAMES);
    assert(!state.bufferFrames && !state.bufferAgeFrames && !PortActionPeek(&state,&intent));
    /* Only natural neutral resumes the ordinary grace clock. */
    recovery.comboGracePaused=0;recovery.spellBusy=0;
    PortActionTick(&state,&recovery);assert(state.comboFrames==PORT_ACTION_COMBO_FRAMES-1);
    recovery.pressed=PORT_ACTION_ATTACK;recovery.held=PORT_ACTION_ATTACK;
    PortActionTick(&state,&recovery);assert(Commit(&state).comboStep==2);
}
static void TestLastMomentSpellWeave(void) {
    for(unsigned hold=0;hold<2;++hold) {
        PortActionState state;PortActionReset(&state);
        for(unsigned step=1;step<=2;++step) {
            Tick(&state,PORT_ACTION_ATTACK,PORT_ACTION_ATTACK,1);
            assert(Commit(&state).comboStep==step);Tick(&state,0,0,0);
        }
        for(unsigned n=1;n<PORT_ACTION_COMBO_FRAMES;++n) Tick(&state,0,0,1);
        assert(state.comboStep==2 && state.comboFrames==1);
        /* A physical spell tap/hold begins before the last grace tick; its
         * legitimate charge/release must not silently turn the weave into a
         * fresh combo. Charge duration remains bounded by the existing cap. */
        Tick(&state,PORT_ACTION_MAGIC,hold?PORT_ACTION_MAGIC:0,1);
        if(hold) {
            for(unsigned n=1;n<13;++n) Tick(&state,0,PORT_ACTION_MAGIC,1);
            assert(state.comboStep==2 && state.comboFrames==1);
            Tick(&state,0,0,1);
        }
        PortActionIntent spell=Commit(&state);
        assert(spell.command==(hold?PORT_ACTION_NATIVE_TECHNIQUE:PORT_ACTION_FIRE));
        assert(!spell.finisher && state.comboStep==2 && state.comboFrames==PORT_ACTION_COMBO_FRAMES);
        for(unsigned n=0;n<20;++n) Tick(&state,0,0,0);
        Tick(&state,PORT_ACTION_ATTACK,PORT_ACTION_ATTACK,1);
        assert(Commit(&state).comboStep==3);
    }
}
static void Cover(unsigned char* seen,uint32_t move) {
    int index=PortActionCatalogFindNative(move);assert(index>=0);seen[index]=1;
}
static void TestCompleteRoutes(void) {
    unsigned char seen[124]={0};const unsigned directions[]={0,PORT_ACTION_LEFT,PORT_ACTION_RIGHT,PORT_ACTION_UP,PORT_ACTION_DOWN};
    const unsigned charges[]={0,16,30};
    for(unsigned magic=0;magic<7;++magic) for(unsigned d=0;d<5;++d)
    for(unsigned charge=0;charge<3;++charge) for(unsigned modifier=0;modifier<2;++modifier) {
        Cover(seen,PortActionRouteFinisher(magic,0,directions[d],0,charges[charge],modifier));
        Cover(seen,PortActionRouteFinisher(magic,1,directions[d],0,charges[charge],modifier));
        Cover(seen,PortActionRouteMagic(magic,directions[d],0,charges[charge],modifier));
    }
    for(unsigned family=0;family<14;++family) for(unsigned charge=0;charge<3;++charge)
    for(unsigned modifier=0;modifier<2;++modifier) Cover(seen,PortActionRouteAssist(family,charges[charge],modifier));
    for(unsigned n=0;n<PortActionRouteKeybladeCount();++n) Cover(seen,PortActionRouteKeyblade(n));
    unsigned total=0;
    for(unsigned n=0;n<124;++n) {
        const PortActionCatalogEntry* entry=PortActionCatalogGet(n);
        if(entry->category!=PORT_CATALOG_ITEM && entry->nativeMove!=MOVE_KEYBLADE_1) assert(seen[n]);
        total+=seen[n];
    }
    assert(total==116);
}
int main(void) { TestTree();TestMagic();TestAssist();TestSpellWeave();TestRequestLifetime();TestDefensiveContinuation();TestRecoveryGrace();TestLastMomentSpellWeave();TestCompleteRoutes();puts("PASS: controller tree, bounded busy/effect buffers, captured gesture facing, defensive continuation/expiry, early-recovery grace, last-moment spell weaving, damage priority, earned assists and 116 meaningful native routes.");return 0; }
