#ifndef PORT_ACTION_CATALOG_H
#define PORT_ACTION_CATALOG_H
#include <stdint.h>

/* These are the concrete moves handled by the native Sora move dispatcher.
 * Catalog membership unlocks selection, not an animation/resource override.
 * Native geometry, immunity, effect ownership and finite summon pools remain. */
enum PortActionCatalogCategory {
    PORT_CATALOG_TECHNIQUE,
    PORT_CATALOG_MAGIC,
    PORT_CATALOG_SUMMON,
    PORT_CATALOG_FRIEND,
    PORT_CATALOG_KEYBLADE,
    PORT_CATALOG_ITEM,
    PORT_CATALOG_CATEGORY_COUNT
};
enum PortActionCatalogFlag {
    PORT_CATALOG_FINISHER = 1u << 0,
    PORT_CATALOG_GROUND_ONLY = 1u << 1,
    PORT_CATALOG_EFFECT_CHANNEL = 1u << 2,
    PORT_CATALOG_COMPANION = 1u << 3,
    PORT_CATALOG_WEAPON = 1u << 4,
    PORT_CATALOG_SUPPORT = 1u << 5
};
typedef struct PortActionCatalogEntry {
    const char* name;
    uint32_t nativeMove; /* Full native MoveId, including high-bit tier codes. */
    uint16_t flags;
    uint8_t category;
    uint8_t mpCost; /* Catalog requests are unrestricted: always zero. */
    uint8_t glyph; /* 0..6 element mark; 7 weapon, 8 summon, 9 friend, 10 item. */
} PortActionCatalogEntry;

unsigned PortActionCatalogCount(void);
const PortActionCatalogEntry* PortActionCatalogGet(unsigned index);
int PortActionCatalogFindNative(uint32_t nativeMove);
const char* PortActionCatalogCategoryName(unsigned category);
unsigned PortActionCatalogCategoryCount(unsigned category);
int PortActionCatalogValidMove(uint32_t nativeMove);
int PortActionCatalogCanUseAirborne(uint32_t nativeMove);
int PortActionCatalogUsesEffectChannel(uint32_t nativeMove);
unsigned PortActionCatalogFinisherCount(void);
const PortActionCatalogEntry* PortActionCatalogGetFinisher(unsigned index);
int PortActionCatalogFinisherEligible(uint32_t nativeMove);
#endif
