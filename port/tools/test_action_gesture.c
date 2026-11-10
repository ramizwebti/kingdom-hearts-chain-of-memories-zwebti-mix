
#include "port/port_action_gesture.h"
#include <assert.h>
#include <stdio.h>
int main(void){PortActionMagicGesture g;unsigned n;PortActionMagicGestureReset(&g);
 assert(PortActionMagicGestureTick(&g,PORT_ACTION_MAGIC,PORT_ACTION_MAGIC,0)==PORT_ACTION_MAGIC);
 for(n=0;n<300;++n)assert(!PortActionMagicGestureTick(&g,0,PORT_ACTION_MAGIC,0));
 assert(!PortActionMagicGestureTick(&g,0,0,0));assert(!PortActionMagicGestureVisible(&g));
 assert(PortActionMagicGestureTick(&g,PORT_ACTION_SPECIAL,0,0)==PORT_ACTION_NEXT_MAGIC);
 assert(PortActionMagicGestureTick(&g,PORT_ACTION_LEFT|PORT_ACTION_RIGHT,PORT_ACTION_MAGIC,0)==0);
 assert(PortActionMagicGestureTick(&g,PORT_ACTION_MAGIC,0,1)==0);
 assert(PortActionMagicGestureTick(&g,PORT_ACTION_SPECIAL,0,1)==PORT_ACTION_NEXT_MAGIC);
 puts("direct magic: press casts, hold/release do not, separate cycle works");return 0;}
