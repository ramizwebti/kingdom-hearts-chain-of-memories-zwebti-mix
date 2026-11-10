#ifndef PORT_ACTION_GESTURE_H
#define PORT_ACTION_GESTURE_H
#include "port/port_action.h"
/* Direct controls: magic casts on press, the former special button cycles
 * spells. No timed hold/release gesture or movement-dependent selection. */
typedef struct PortActionMagicGesture {
    uint8_t active, frames, cancelled;
} PortActionMagicGesture;
void PortActionMagicGestureReset(PortActionMagicGesture* gesture);
unsigned PortActionMagicGestureTick(PortActionMagicGesture* gesture,
    unsigned pressed, unsigned held, int interrupted);
int PortActionMagicGestureVisible(const PortActionMagicGesture* gesture);
#endif
