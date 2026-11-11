#ifndef PORT_ACTION_PRESSURE_H
#define PORT_ACTION_PRESSURE_H
#include <stdint.h>

/* An interrupted crowd attacker briefly retains its own attack slot. This
 * does not stop another active attack or grant Sora damage immunity. Keys are
 * compared only; the scheduler still verifies live actors in its own pool. */
enum { PORT_ACTION_PRESSURE_SLOTS = 3, PORT_ACTION_PRESSURE_HOLD_TICKS = 18 };
typedef struct PortActionPressureSlot {
    uintptr_t key;
    uint32_t frame;
} PortActionPressureSlot;
typedef struct PortActionPressure {
    PortActionPressureSlot slots[PORT_ACTION_PRESSURE_SLOTS];
} PortActionPressure;

static inline void PortActionPressureReset(PortActionPressure* state) {
    unsigned i;
    for(i=0;i<PORT_ACTION_PRESSURE_SLOTS;++i) state->slots[i].key=0;
}
static inline int PortActionPressureHeld(const PortActionPressure* state,
        uintptr_t key,uint32_t now) {
    unsigned i;
    if(!key) return 0;
    for(i=0;i<PORT_ACTION_PRESSURE_SLOTS;++i)
        if(state->slots[i].key==key &&
           (uint32_t)(now-state->slots[i].frame)<PORT_ACTION_PRESSURE_HOLD_TICKS) return 1;
    return 0;
}
static inline int PortActionPressureReserve(PortActionPressure* state,
        uintptr_t key,uint32_t now) {
    unsigned i,slot=PORT_ACTION_PRESSURE_SLOTS;
    if(!key || PortActionPressureHeld(state,key,now)) return 0;
    for(i=0;i<PORT_ACTION_PRESSURE_SLOTS;++i) {
        if(state->slots[i].key==key) { slot=i; break; }
        if(!state->slots[i].key ||
           (uint32_t)(now-state->slots[i].frame)>=PORT_ACTION_PRESSURE_HOLD_TICKS) slot=i;
    }
    /* A contact never evicts a different valid reservation. */
    if(slot==PORT_ACTION_PRESSURE_SLOTS) return 0;
    state->slots[slot].key=key;state->slots[slot].frame=now;
    return 1;
}
static inline void PortActionPressureForget(PortActionPressure* state,uintptr_t key) {
    unsigned i;
    for(i=0;i<PORT_ACTION_PRESSURE_SLOTS;++i)
        if(state->slots[i].key==key) state->slots[i].key=0;
}
#endif
