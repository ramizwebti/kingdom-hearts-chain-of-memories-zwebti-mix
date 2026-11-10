#ifndef PORT_ACTION_TARGET_H
#define PORT_ACTION_TARGET_H
#include "port/port_action.h"

/* Fixed-point, short-range aim assistance. A negative score rejects a target.
 * Vertical separation affects selection only: melee never lifts the player. */
static inline int PortActionTargetScoreWithin(int dx, int dy, int dz,
        unsigned held, int facingLeft, int retained, int rangeX, int rangeY, int rangeZ) {
    int ax, ay, az, score;
    int ix = ((held & PORT_ACTION_RIGHT) != 0)-((held & PORT_ACTION_LEFT) != 0);
    int iy = ((held & PORT_ACTION_DOWN) != 0)-((held & PORT_ACTION_UP) != 0);
    if (dx < -rangeX || dx > rangeX || dy < -rangeY || dy > rangeY ||
        dz < -rangeZ || dz > rangeZ) return -1;
    ax=dx<0?-dx:dx; ay=dy<0?-dy:dy; az=dz<0?-dz:dz;
    if ((ix && dx*ix < -8*256) || (iy && dy*iy < -8*256)) return -1;
    score=ax+ay*2+az;
    if (!ix && !iy && ((facingLeft && dx>0) || (!facingLeft && dx<0))) {
        if (ax>32*256) return -1;
        score+=24*256;
    }
    if (retained) score-=4*256;
    return score<0?0:score;
}

static inline int PortActionTargetScore(int dx, int dy, int dz,
        unsigned held, int facingLeft, int retained) {
    return PortActionTargetScoreWithin(dx,dy,dz,held,facingLeft,retained,64*256,32*256,36*256);
}

/* The earned follow-up retains the target launched by the third swing.
 * This larger selection envelope never lifts Sora or bypasses collision. */
static inline int PortActionFinisherTargetScore(int dx, int dy, int dz,
        unsigned held, int facingLeft, int retained) {
    return PortActionTargetScoreWithin(dx,dy,dz,held,facingLeft,retained,96*256,48*256,80*256);
}

/* Both the per-tick velocity and whole-swing displacement are bounded. */
static inline int PortActionApproachStep(int current, int origin, int desired,
        int perTick, int total) {
    int step;
    /* Solid collision/platform motion can move the actor outside this swing's
     * travel budget. Assistance must not pull against that physical response. */
    if (current>origin+total || current<origin-total) return 0;
    if (desired > origin+total) desired=origin+total;
    if (desired < origin-total) desired=origin-total;
    step=desired-current;
    if (step>perTick) step=perTick;
    if (step<-perTick) step=-perTick;
    return step;
}
#endif
