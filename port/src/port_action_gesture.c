#include "port/port_action_gesture.h"
void PortActionMagicGestureReset(PortActionMagicGesture* g) {
    g->active=g->frames=g->cancelled=0;
}
int PortActionMagicGestureVisible(const PortActionMagicGesture* g) {
    (void)g;
    return 0;
}
unsigned PortActionMagicGestureTick(PortActionMagicGesture* g,
        unsigned pressed,unsigned held,int interrupted) {
    unsigned result=0;
    (void)held;
    PortActionMagicGestureReset(g);
    if(pressed&PORT_ACTION_SPECIAL) result|=PORT_ACTION_NEXT_MAGIC;
    if(!interrupted && (pressed&PORT_ACTION_MAGIC)) result|=PORT_ACTION_MAGIC;
    return result;
}
