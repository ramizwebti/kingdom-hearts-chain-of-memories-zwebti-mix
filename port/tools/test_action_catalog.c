/* Metadata assertions/export only: this does not claim native move effects ran. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "port/port_action_catalog.h"
#include "card_ids.h"
#ifdef NDEBUG
#error Catalog checks require active assertions.
#endif

int main(int argc,char** argv) {
    unsigned i,j,total=0;
    static const unsigned counts[PORT_CATALOG_CATEGORY_COUNT]={17,36,23,23,18,7};
    assert(PortActionCatalogCount()==124);
    assert(!PortActionCatalogGet(124));
    assert(!PortActionCatalogGet(~0u));
    for(i=0;i<PORT_CATALOG_CATEGORY_COUNT;++i) {
        assert(PortActionCatalogCategoryCount(i)==counts[i]);
        assert(PortActionCatalogCategoryName(i)[0]);
        total+=counts[i];
    }
    assert(total==124 && !PortActionCatalogCategoryCount(PORT_CATALOG_CATEGORY_COUNT));
    assert(!PortActionCatalogCategoryName(PORT_CATALOG_CATEGORY_COUNT)[0]);
    for(i=0;i<PortActionCatalogCount();++i) {
        const PortActionCatalogEntry* entry=PortActionCatalogGet(i);
        assert(entry && entry->name && entry->name[0] && strlen(entry->name)<=30);
        assert(entry->mpCost==0 && entry->glyph<=10);
        assert(PortActionCatalogFindNative(entry->nativeMove)==(int)i);
        assert(PortActionCatalogValidMove(entry->nativeMove));
        for(j=0;j<i;++j) assert(PortActionCatalogGet(j)->nativeMove!=entry->nativeMove);
    }
    assert(PortActionCatalogValidMove(MOVE_KINGDOM_KEY)); /* Valid zero MoveId. */
    assert(PortActionCatalogValidMove((uint32_t)MOVE_FIRAGA)); /* Full high-bit code. */
    assert(!PortActionCatalogValidMove(MOVE_SOUL_EATER));
    assert(!PortActionCatalogValidMove(MOVE_DARK_AURA));
    assert(!PortActionCatalogValidMove(MOVE_BOSS_A));
    assert(!PortActionCatalogValidMove(MOVE_SLEIGHT_57));
    assert(!PortActionCatalogValidMove(MOVE_SLEIGHT_68));
    assert(!PortActionCatalogValidMove(0xffffffffu));
    assert(PortActionCatalogFinisherCount()==17);
    assert(!PortActionCatalogGetFinisher(17));
    for(i=0;i<17;++i) assert(PortActionCatalogFinisherEligible(PortActionCatalogGetFinisher(i)->nativeMove));
    assert(PortActionCatalogCanUseAirborne(MOVE_FIRE));
    assert(PortActionCatalogCanUseAirborne((uint32_t)MOVE_SONIC_BLADE));
    assert(!PortActionCatalogCanUseAirborne(MOVE_TRINITY_LIMIT));
    assert(PortActionCatalogUsesEffectChannel(MOVE_THUNDER));
    assert(PortActionCatalogUsesEffectChannel((uint32_t)MOVE_SONIC_BLADE));
    assert(PortActionCatalogUsesEffectChannel(MOVE_ZANTETSUKEN));
    assert(!PortActionCatalogUsesEffectChannel(MOVE_SLIDING_DASH));
    assert(!PortActionCatalogUsesEffectChannel(MOVE_BLITZ));
    assert(!PortActionCatalogUsesEffectChannel(MOVE_ARS_ARCANUM));
    assert(!PortActionCatalogUsesEffectChannel(MOVE_KINGDOM_KEY));
    if(argc==2 && !strcmp(argv[1],"--json")) {
        putchar('[');
        for(i=0;i<124;++i) {
            const PortActionCatalogEntry* e=PortActionCatalogGet(i);
            printf("%s{\"index\":%u,\"nativeMove\":%lu,\"name\":\"%s\",\"category\":%u,\"categoryName\":\"%s\",\"flags\":%u,\"glyph\":%u}",
                i?",":"",i,(unsigned long)e->nativeMove,e->name,e->category,
                PortActionCatalogCategoryName(e->category),e->flags,e->glyph);
        }
        puts("]");
    } else puts("PASS: 124 unique concrete Sora native moves, six categories, 17 selectable finishers, full-width codes and no progression gates. Metadata only; native effects require gameplay verification.");
    return 0;
}
