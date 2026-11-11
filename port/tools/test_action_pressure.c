#include "port/port_action_pressure.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    PortActionPressure state;
    unsigned i;
    PortActionPressureReset(&state);
    assert(!PortActionPressureHeld(&state,0,0));
    assert(!PortActionPressureReserve(&state,0,0));
    assert(PortActionPressureReserve(&state,1,100));
    assert(PortActionPressureHeld(&state,1,100));
    assert(PortActionPressureHeld(&state,1,117));
    /* Repeated contacts do not extend the earned opening indefinitely. */
    assert(!PortActionPressureReserve(&state,1,117));
    assert(!PortActionPressureHeld(&state,1,118));
    assert(PortActionPressureReserve(&state,1,118));
    assert(PortActionPressureReserve(&state,2,118));
    assert(PortActionPressureReserve(&state,3,118));
    assert(!PortActionPressureReserve(&state,4,118));
    for(i=1;i<=3;++i) assert(PortActionPressureHeld(&state,i,119));
    PortActionPressureForget(&state,2);
    assert(!PortActionPressureHeld(&state,2,119));
    assert(PortActionPressureReserve(&state,4,119));
    assert(PortActionPressureHeld(&state,1,135));
    assert(!PortActionPressureHeld(&state,1,136));
    assert(PortActionPressureHeld(&state,4,136));
    assert(!PortActionPressureHeld(&state,4,137));
    /* Global frame wrap preserves a short duration; a reset removes all keys. */
    PortActionPressureReset(&state);
    assert(PortActionPressureReserve(&state,9,UINT32_MAX-5));
    assert(PortActionPressureHeld(&state,9,11));
    assert(!PortActionPressureHeld(&state,9,12));
    PortActionPressureReset(&state);
    assert(!PortActionPressureHeld(&state,9,11));
    puts("PASS: bounded interrupted-attacker reservations, no refresh/eviction, expiry and reset");
    return 0;
}
