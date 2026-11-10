/* Uses the same bounded movement step as the live Sora adapter. */
#include "port/port_action_motion.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>

static void TestGroundResponsiveness(void) {
    PortActionMotion motion = { 0, 64, 0 };
    unsigned frame;
    for (frame = 0; frame < 2; frame++)
        motion = PortActionMoveStep(motion.speed, motion.angle, motion.facingLeft,
                                   PORT_ACTION_RIGHT, 0, 0);
    assert(motion.speed == PORT_ACTION_RUN_SPEED);
    motion = PortActionMoveStep(motion.speed, motion.angle, 0, PORT_ACTION_LEFT, 0, 0);
    assert(motion.speed == PORT_ACTION_RUN_SPEED && motion.angle == 192 && motion.facingLeft);
    for (frame = 0; frame < 2; frame++)
        motion = PortActionMoveStep(motion.speed, motion.angle, motion.facingLeft, 0, 0, 0);
    assert(motion.speed == 0);
}

static void TestAirControl(void) {
    PortActionMotion motion = { 0, 64, 0 };
    unsigned frame;
    for (frame = 0; frame < 2; frame++)
        motion = PortActionMoveStep(motion.speed, motion.angle, motion.facingLeft,
                                   PORT_ACTION_RIGHT, 1, 0);
    assert(motion.speed == PORT_ACTION_AIR_SPEED);
    motion = PortActionMoveStep(motion.speed, motion.angle, 0, PORT_ACTION_LEFT, 1, 0);
    assert(motion.speed == PORT_ACTION_AIR_SPEED && motion.angle == 192 && motion.facingLeft);
    for (frame = 0; frame < 2; frame++)
        motion = PortActionMoveStep(motion.speed, motion.angle, motion.facingLeft, 0, 1, 0);
    assert(motion.speed == 0);
}

static void TestDirectionsAndBounds(void) {
    static const unsigned buttons[8] = {
        PORT_ACTION_UP, PORT_ACTION_UP | PORT_ACTION_RIGHT, PORT_ACTION_RIGHT,
        PORT_ACTION_RIGHT | PORT_ACTION_DOWN, PORT_ACTION_DOWN,
        PORT_ACTION_DOWN | PORT_ACTION_LEFT, PORT_ACTION_LEFT,
        PORT_ACTION_LEFT | PORT_ACTION_UP
    };
    unsigned i;
    PortActionMotion motion;
    for (i = 0; i < 8; i++) {
        motion = PortActionMoveStep(INT_MAX, 0, 0, buttons[i], 0, 0);
        assert(motion.angle == i * 32 && motion.speed == PORT_ACTION_RUN_SPEED);
        motion = PortActionMoveStep(INT_MAX, 0, 0, buttons[i], 1, 1);
        assert(motion.speed == PORT_ACTION_AIR_SPEED);
    }
    motion = PortActionMoveStep(-10, 64, 0, PORT_ACTION_LEFT | PORT_ACTION_RIGHT, 0, 0);
    assert(motion.speed == 0 && motion.angle == 64);
    motion = PortActionMoveStep(0, 64, 0, PORT_ACTION_UP | PORT_ACTION_DOWN, 1, 0);
    assert(motion.speed == 0 && motion.angle == 64);
    motion = PortActionMoveStep(INT_MAX, 64, 0, PORT_ACTION_RIGHT, 0, 1);
    assert(motion.speed == PORT_ACTION_HASTE_SPEED);
    motion = PortActionMoveStep(0, 192, 1, PORT_ACTION_UP, 0, 0);
    assert(motion.facingLeft); /* Pure depth motion preserves the sprite's facing. */
}

static void TestCommittedCancelWindows(void) {
    assert(PortActionMeleeCanCancel(0,0,1));
    assert(PortActionMeleeCanCancel(PORT_ACTION_BASIC_HIT_TICK+2,0,1));
    assert(PortActionMeleeCanCancel(0,2,1));
    assert(PortActionMeleeCanCancel(PORT_ACTION_BASIC_HIT_TICK+2,2,1));
    assert(PORT_ACTION_BASIC_CHAIN_TICK > PORT_ACTION_BASIC_HIT_TICK);
    assert(PORT_ACTION_BASIC_RECOVERY_TICK > PORT_ACTION_BASIC_CHAIN_TICK);
    assert(PORT_ACTION_ENDER_RECOVERY_TICK > PORT_ACTION_ENDER_HIT_TICK);
    assert(!PortActionMeleeCanCancel(1000, 0, 0));
    assert(!PortActionMeleeCanCancel(1000, 2, 0));
    assert(PORT_ACTION_DODGE_VULNERABLE_TICKS > 0);
    assert(PORT_ACTION_DODGE_VULNERABLE_TICKS < PORT_ACTION_DODGE_TICKS);
    assert(PortActionAirChainHang(32,0,0));
    assert(!PortActionAirChainHang(33,0,0));
    assert(PortActionAirChainHang(18,2,0));
    assert(!PortActionAirChainHang(19,2,0));
    assert(PortActionAirChainHang(18,2,1));
    assert(!PortActionAirChainHang(19,2,1));
    assert(!PortActionAirChainHang(18,3,1));
}

static void TestAuthoredPlaybackRates(void) {
    static const unsigned durations[]={30,38,52,54,71};
    static const unsigned expected[]={20,26,35,54,36};
    unsigned i;
    for(i=0;i<5;i++) {
        unsigned ticks=0,frame=0,strikes=0;
        while(ticks<durations[i]) {
            unsigned next=ticks+(i==4 ? 2u : PortActionAnimationSteps(frame,i==3));
            if(ticks<15 && next>=15) ++strikes;
            ticks=next;++frame;
        }
        assert(frame==expected[i]);
        assert(strikes==1); /* Acceleration crosses the hit pose exactly once. */
        assert(ticks>=durations[i]); /* Recovery never discards authored time. */
    }
}

int main(void) {
    TestGroundResponsiveness();
    TestAirControl();
    TestDirectionsAndBounds();
    TestCommittedCancelWindows();
    TestAuthoredPlaybackRates();
    puts("PASS: ground/air acceleration, immediate turns, braking, eight directions, bounded speeds and cancel windows");
    return 0;
}
