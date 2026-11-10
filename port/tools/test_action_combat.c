/* Pure controller invariants under real held/pressed edge semantics. */
#include "port/port_action.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned rng=0x10373u;
static unsigned Next(void) { rng=rng*1664525u+1013904223u;return rng; }
int main(void) {
    PortActionState state;PortActionReset(&state);unsigned previous=0,commits=0;
    for(unsigned frame=0;frame<100000;++frame) {
        PortActionInput input={0};unsigned held=(Next()>>8)&0x7fffu;
        input.held=(uint16_t)held;input.pressed=(uint16_t)(held&~previous);previous=held;
        input.canAct=(Next()%3)!=0;input.airborne=(Next()%5)==0;input.facingLeft=Next()&1;
        input.spellBusy=(Next()%7)==0;input.interrupted=(Next()%113)==0;
        PortActionTick(&state,&input);
        if((Next()%53)==0) { PortActionBeginDodge(&state);assert(PortActionPerfectDodge(&state));assert(!PortActionPerfectDodge(&state)); }
        if((Next()%7)==0) { PortActionRestoreOnHit(&state);unsigned gauge=state.assistGauge;PortActionRestoreOnHit(&state);assert(state.assistGauge==gauge); }
        uint32_t assist;
        if(PortActionPendingAssist(&state,&assist) && (Next()%3)==0) {
            unsigned gauge=state.assistGauge;assert(!PortActionCommitAssist(&state,assist^0x1234u));assert(state.assistGauge==gauge);
            assert(PortActionCommitAssist(&state,assist));
        }
        PortActionIntent intent;
        if(PortActionPeek(&state,&intent)) {
            assert(input.canAct && intent.comboStep<=3);
            PortActionIntent stale=intent;stale.token++;assert(!PortActionCommit(&state,&stale));
            assert(PortActionCommit(&state,&intent));++commits;
        }
        assert(state.mp<=PORT_ACTION_MAX_MP && state.assistGauge<=PORT_ACTION_MAX_ASSIST);
        assert(state.comboStep<=3 && state.assistFamily<14 && state.keybladeIndex<17);
        assert(state.magicIndex<7 && state.attackChargeTicks<=30 && state.magicChargeTicks<=30 && state.assistChargeTicks<=30);
    }
    assert(commits>1000);
    PortActionReset(&state);PortActionQueueNative(&state,0);uint32_t native;
    assert(PortActionPendingNative(&state,&native) && native==0);
    PortActionQueueNative(&state,0x8002acabu);assert(PortActionPendingNative(&state,&native) && native==0x8002acabu);
    PortActionAddAssistGauge(&state,100);PortActionQueueAssist(&state,0x8002acabu,0);PortActionInterrupt(&state);
    assert(PortActionPendingAssist(&state,&native) && state.assistGauge==100);
    assert(PortActionCommitAssist(&state,native) && !state.assistGauge);
    printf("PASS: 100000 controller-edge/resource/intent steps, %u accepted actions, no held replay, native32-bit IDs and interruption-safe assists.\n",commits);
    return 0;
}
