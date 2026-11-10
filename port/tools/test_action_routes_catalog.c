/* Execute production routing functions against the complete native catalog.
 * No actor stubs, ROM, saves, or renderer are involved in this pure bridge. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "port/port_action_routes.h"
#include "port/port_action_catalog.h"
#include "port/port_action.h"
#include "card_ids.h"

static unsigned char sAll[124], sMagic[124], sFinish[124], sAssist[124], sWeapon[124];
static void record(unsigned char* set, uint32_t move) {
    int index=PortActionCatalogFindNative(move);
    assert(index>=0 && index<124);
    set[index]=sAll[index]=1;
}
static unsigned count(const unsigned char* set) {
    unsigned i,n=0;
    for(i=0;i<124;++i) n+=set[i]!=0;
    return n;
}
int main(void) {
    const unsigned directions[4]={PORT_ACTION_LEFT,PORT_ACTION_RIGHT,PORT_ACTION_UP,PORT_ACTION_DOWN};
    const unsigned charges[3]={0,PORT_ACTION_CHARGE_MIDDLE,PORT_ACTION_CHARGE_FULL};
    unsigned magic,air,mask,left,tier,advanced,family,index;
    assert(PortActionCatalogCount()==124);
    for(magic=0;magic<7;++magic) for(air=0;air<2;++air)
    for(mask=0;mask<16;++mask) for(left=0;left<2;++left)
    for(tier=0;tier<3;++tier) for(advanced=0;advanced<2;++advanced) {
        unsigned i,held=0;
        for(i=0;i<4;++i) if(mask&(1u<<i)) held|=directions[i];
        record(sFinish,PortActionRouteFinisher(magic,air,held,left,charges[tier],advanced));
        record(sMagic,PortActionRouteMagic(magic,held,left,charges[tier],advanced));
    }
    for(family=0;family<14;++family) for(tier=0;tier<3;++tier)
    for(advanced=0;advanced<2;++advanced)
        record(sAssist,PortActionRouteAssist(family,charges[tier],advanced));
    assert(PortActionRouteKeybladeCount()==17);
    for(index=0;index<17;++index) record(sWeapon,PortActionRouteKeyblade(index));
    assert(count(sMagic)==36);
    assert(count(sFinish)==18); /* 17 Sora techniques plus native Trinity Limit. */
    assert(count(sAssist)==45); /* 14 native families x3, plus3 native combos. */
    assert(count(sWeapon)==17);
    assert(count(sAll)==116);
    for(index=0;index<124;++index) {
        const PortActionCatalogEntry* entry=PortActionCatalogGet(index);
        int excluded=entry->category==PORT_CATALOG_ITEM || entry->nativeMove==MOVE_KEYBLADE_1;
        assert(sAll[index]==!excluded);
        if(entry->category==PORT_CATALOG_MAGIC) assert(sMagic[index]);
    }
    puts("PASS: production routes return only valid native moves; all36 magic,18 finishers,45 assist variants and17 meaningful keyblades reachable;116 distinct native entries covered, seven card-reload items and one unused legacy keyblade explicitly excluded.");
    return 0;
}
