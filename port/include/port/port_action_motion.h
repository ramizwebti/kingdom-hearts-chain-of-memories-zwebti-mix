#ifndef PORT_ACTION_MOTION_H
#define PORT_ACTION_MOTION_H

#include "port/port_action.h"

/* Fixed-point distances per simulation tick. The game's original gravity,
 * collision, animation and clock continue to own the physical simulation. */
enum {
    PORT_ACTION_RUN_SPEED = 1024,
    PORT_ACTION_RUN_ACCEL = 512,
    PORT_ACTION_RUN_BRAKE = 512,
    PORT_ACTION_HASTE_SPEED = 1280,
    PORT_ACTION_AIR_SPEED = 896,
    PORT_ACTION_AIR_ACCEL = 448,
    PORT_ACTION_AIR_BRAKE = 448,
    PORT_ACTION_DODGE_SPEED = 1792,
    PORT_ACTION_DODGE_TICKS = 18,
    PORT_ACTION_DODGE_VULNERABLE_TICKS = 6,
    PORT_ACTION_BASIC_HIT_TICK = 15, /* normal authored variant's hit event */
    PORT_ACTION_BASIC_CHAIN_TICK = 17, /* voluntary dodge/jump only, never Attack */
    PORT_ACTION_BASIC_RECOVERY_TICK = 32, /* finite air-hang budget, not animation end */
    PORT_ACTION_ENDER_HIT_TICK = 11,
    PORT_ACTION_ENDER_RECOVERY_TICK = 21
};

typedef struct PortActionMotion {
    int speed;
    unsigned angle;
    unsigned facingLeft;
} PortActionMotion;

static inline int PortActionApproachSpeed(int speed, int moving, int cap,
        int acceleration, int brake) {
    if (cap <= 0) return 0;
    if (speed < 0) speed = 0;
    if (acceleration < 0) acceleration = 0;
    if (brake < 0) brake = 0;
    if (moving) speed = speed >= cap - acceleration ? cap : speed + acceleration;
    else speed = speed > brake ? speed - brake : 0;
    return speed > cap ? cap : speed;
}

static inline PortActionMotion PortActionMoveStep(int speed, unsigned angle,
        unsigned facingLeft, unsigned held, int airborne, int haste) {
    PortActionMotion result;
    int x = ((held & PORT_ACTION_RIGHT) != 0) - ((held & PORT_ACTION_LEFT) != 0);
    int y = ((held & PORT_ACTION_DOWN) != 0) - ((held & PORT_ACTION_UP) != 0);
    int cap = airborne ? PORT_ACTION_AIR_SPEED :
        (haste ? PORT_ACTION_HASTE_SPEED : PORT_ACTION_RUN_SPEED);
    int acceleration = airborne ? PORT_ACTION_AIR_ACCEL : PORT_ACTION_RUN_ACCEL;
    int brake = airborne ? PORT_ACTION_AIR_BRAKE : PORT_ACTION_RUN_BRAKE;

    result.angle = angle;
    result.facingLeft = facingLeft != 0;
    if (x || y) {
        if (x > 0) {
            result.angle = y < 0 ? 32 : (y > 0 ? 96 : 64);
            result.facingLeft = 0;
        } else if (x < 0) {
            result.angle = y < 0 ? 224 : (y > 0 ? 160 : 192);
            result.facingLeft = 1;
        } else {
            result.angle = y < 0 ? 0 : 128;
        }
    }
    result.speed = PortActionApproachSpeed(speed, x || y, cap, acceleration, brake);
    return result;
}

static inline int PortActionMeleeCanCancel(unsigned timer, unsigned comboStep,
        int grounded) {
    (void)timer;
    (void)comboStep;
    return grounded;
}

/* Controlled animation playback; collision events read the same authored
 * timeline. This never grants permission to restart a playing Attack. */
static inline unsigned PortActionAnimationSteps(unsigned elapsed, int finisher) {
    return finisher ? 1u : (elapsed & 1u ? 2u : 1u);
}

static inline int PortActionAirChainHang(unsigned timer, unsigned comboIndex,
        int finisherReady) {
    (void)finisherReady;
    /* Each non-looping aerial swing has a finite pause. The third swing
     * resumes gravity just after its hit; it never needs previous contact. */
    return comboIndex < 2 ? timer <= PORT_ACTION_BASIC_RECOVERY_TICK :
        (comboIndex == 2 && timer <= PORT_ACTION_BASIC_HIT_TICK + 3);
}

#endif
