#ifndef PORT_ACTION_ROUTES_H
#define PORT_ACTION_ROUTES_H
#include <stdint.h>
uint32_t PortActionRouteFinisher(unsigned magic,unsigned airborne,unsigned directions,
                                unsigned facingLeft,unsigned charge,unsigned elemental);
uint32_t PortActionRouteMagic(unsigned magic,unsigned directions,unsigned facingLeft,
                             unsigned charge,unsigned advanced);
uint32_t PortActionRouteAssist(unsigned family,unsigned charge,unsigned special);
uint32_t PortActionRouteKeyblade(unsigned index);
unsigned PortActionRouteKeybladeCount(void);
unsigned PortActionRouteMagicCost(unsigned magic,unsigned charge,unsigned advanced);
const char* PortActionRouteAssistName(unsigned family);
#endif
