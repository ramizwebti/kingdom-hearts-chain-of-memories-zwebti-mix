/**
 * mode_chkbtl.c
 * Debug Battle Check
 */

#include "macros.h"
#include "mode_chkbtl.h"
#include "gba/keys.h"
#include "world_types.h"
#include "fade.h"
#include "battle_debug_types.h"
#include "battle_work.h"
#include "card_api.h"
#include "display.h"
#include "engine_math.h"
#include "game_state.h"
#include "key.h"
#include "mode.h"
#include "mode_battle_data.h"
#include "player_progression_types.h"
#include "registration_data.h"
#include <stddef.h>
#include "system_state.h"
#include "task_descriptors.h"
#include "taskpool.h"
#include "types.h"
#include "debug_text.h"
#include "battle_ids.h"
#ifdef PLATFORM_PC
#include "port/port_practice.h"
#include "enemy_types.h"
extern void PortLog(const char* fmt,...);
#endif

static ChkBtlWork sChkBtlWork;

BtlWork* gBtlWork EWRAM_COMMON(4);
u16 gVsBattleMinY EWRAM_COMMON(4);
u16 gVsBattleMaxY EWRAM_COMMON(4);
u16 gVsBattleHalfWidth EWRAM_COMMON(4);

static const ChkBtlEntry sChkBtlEntries[209] = {
    { WORLD_TRAVERSE_TOWN, 0, BATTLE_TRAVERSE_TOWN_0, NULL, "\x82\x73\x82\x76\x82\x6d\x82\x4f" },
    { WORLD_TRAVERSE_TOWN, 0, BATTLE_TRAVERSE_TOWN_1, NULL, "\x82\x73\x82\x76\x82\x6d\x82\x50" },
    { WORLD_TRAVERSE_TOWN, 0, BATTLE_TRAVERSE_TOWN_2, NULL, "\x82\x73\x82\x76\x82\x6d\x82\x51" },
    { WORLD_TRAVERSE_TOWN, 0, BATTLE_TRAVERSE_TOWN_3, NULL, "\x82\x73\x82\x76\x82\x6d\x82\x52" },
    { WORLD_TRAVERSE_TOWN, 0, BATTLE_TRAVERSE_TOWN_4, NULL, "\x82\x73\x82\x76\x82\x6d\x82\x53" },
    { WORLD_TRAVERSE_TOWN, 0, BATTLE_TRAVERSE_TOWN_5, NULL, "\x82\x73\x82\x76\x82\x6d\x82\x54" },
    { WORLD_TRAVERSE_TOWN, 0, BATTLE_TRAVERSE_TOWN_6, NULL, "\x82\x73\x82\x76\x82\x6d\x82\x55" },
    { WORLD_TRAVERSE_TOWN, 0, BATTLE_TRAVERSE_TOWN_7, NULL, "\x82\x73\x82\x76\x82\x6d\x82\x56" },
    { WORLD_TRAVERSE_TOWN, 0, BATTLE_TRAVERSE_TOWN_8, NULL, "\x82\x73\x82\x76\x82\x6d\x82\x57" },
    { WORLD_TRAVERSE_TOWN, 0, BATTLE_TRAVERSE_TOWN_9, NULL, "\x82\x73\x82\x76\x82\x6d\x82\x58" },
    { WORLD_OLYMPUS_COLISEUM, 0, BATTLE_OLYMPUS_COLISEUM_0, NULL, "\x82\x62\x82\x6e\x82\x6b\x82\x4f" },
    { WORLD_OLYMPUS_COLISEUM, 0, BATTLE_OLYMPUS_COLISEUM_1, NULL, "\x82\x62\x82\x6e\x82\x6b\x82\x50" },
    { WORLD_OLYMPUS_COLISEUM, 0, BATTLE_OLYMPUS_COLISEUM_2, NULL, "\x82\x62\x82\x6e\x82\x6b\x82\x51" },
    { WORLD_OLYMPUS_COLISEUM, 0, BATTLE_OLYMPUS_COLISEUM_3, NULL, "\x82\x62\x82\x6e\x82\x6b\x82\x52" },
    { WORLD_OLYMPUS_COLISEUM, 0, BATTLE_OLYMPUS_COLISEUM_4, NULL, "\x82\x62\x82\x6e\x82\x6b\x82\x53" },
    { WORLD_OLYMPUS_COLISEUM, 0, BATTLE_OLYMPUS_COLISEUM_5, NULL, "\x82\x62\x82\x6e\x82\x6b\x82\x54" },
    { WORLD_OLYMPUS_COLISEUM, 0, BATTLE_OLYMPUS_COLISEUM_6, NULL, "\x82\x62\x82\x6e\x82\x6b\x82\x55" },
    { WORLD_OLYMPUS_COLISEUM, 0, BATTLE_OLYMPUS_COLISEUM_7, NULL, "\x82\x62\x82\x6e\x82\x6b\x82\x56" },
    { WORLD_OLYMPUS_COLISEUM, 0, BATTLE_OLYMPUS_COLISEUM_8, NULL, "\x82\x62\x82\x6e\x82\x6b\x82\x57" },
    { WORLD_OLYMPUS_COLISEUM, 0, BATTLE_OLYMPUS_COLISEUM_9, NULL, "\x82\x62\x82\x6e\x82\x6b\x82\x58" },
    { WORLD_WONDERLAND, 0, BATTLE_WONDERLAND_0, NULL, "\x82\x76\x82\x6e\x82\x6d\x82\x4f" },
    { WORLD_WONDERLAND, 0, BATTLE_WONDERLAND_1, NULL, "\x82\x76\x82\x6e\x82\x6d\x82\x50" },
    { WORLD_WONDERLAND, 0, BATTLE_WONDERLAND_2, NULL, "\x82\x76\x82\x6e\x82\x6d\x82\x51" },
    { WORLD_WONDERLAND, 0, BATTLE_WONDERLAND_3, NULL, "\x82\x76\x82\x6e\x82\x6d\x82\x52" },
    { WORLD_WONDERLAND, 0, BATTLE_WONDERLAND_4, NULL, "\x82\x76\x82\x6e\x82\x6d\x82\x53" },
    { WORLD_WONDERLAND, 0, BATTLE_WONDERLAND_5, NULL, "\x82\x76\x82\x6e\x82\x6d\x82\x54" },
    { WORLD_WONDERLAND, 0, BATTLE_WONDERLAND_6, NULL, "\x82\x76\x82\x6e\x82\x6d\x82\x55" },
    { WORLD_WONDERLAND, 0, BATTLE_WONDERLAND_7, NULL, "\x82\x76\x82\x6e\x82\x6d\x82\x56" },
    { WORLD_WONDERLAND, 0, BATTLE_WONDERLAND_8, NULL, "\x82\x76\x82\x6e\x82\x6d\x82\x57" },
    { WORLD_WONDERLAND, 0, BATTLE_WONDERLAND_9, NULL, "\x82\x76\x82\x6e\x82\x6d\x82\x58" },
    { WORLD_MONSTRO, 0, BATTLE_MONSTRO_0, NULL, "\x82\x6c\x82\x6e\x82\x6d\x82\x4f" },
    { WORLD_MONSTRO, 0, BATTLE_MONSTRO_1, NULL, "\x82\x6c\x82\x6e\x82\x6d\x82\x50" },
    { WORLD_MONSTRO, 0, BATTLE_MONSTRO_2, NULL, "\x82\x6c\x82\x6e\x82\x6d\x82\x51" },
    { WORLD_MONSTRO, 0, BATTLE_MONSTRO_3, NULL, "\x82\x6c\x82\x6e\x82\x6d\x82\x52" },
    { WORLD_MONSTRO, 0, BATTLE_MONSTRO_4, NULL, "\x82\x6c\x82\x6e\x82\x6d\x82\x53" },
    { WORLD_MONSTRO, 0, BATTLE_MONSTRO_5, NULL, "\x82\x6c\x82\x6e\x82\x6d\x82\x54" },
    { WORLD_MONSTRO, 0, BATTLE_MONSTRO_6, NULL, "\x82\x6c\x82\x6e\x82\x6d\x82\x55" },
    { WORLD_MONSTRO, 0, BATTLE_MONSTRO_7, NULL, "\x82\x6c\x82\x6e\x82\x6d\x82\x56" },
    { WORLD_MONSTRO, 0, BATTLE_MONSTRO_8, NULL, "\x82\x6c\x82\x6e\x82\x6d\x82\x57" },
    { WORLD_MONSTRO, 0, BATTLE_MONSTRO_9, NULL, "\x82\x6c\x82\x6e\x82\x6d\x82\x58" },
    { WORLD_HALLOWEEN_TOWN, 0, BATTLE_HALLOWEEN_TOWN_0, NULL, "\x82\x67\x82\x60\x82\x6b\x82\x4f" },
    { WORLD_HALLOWEEN_TOWN, 0, BATTLE_HALLOWEEN_TOWN_1, NULL, "\x82\x67\x82\x60\x82\x6b\x82\x50" },
    { WORLD_HALLOWEEN_TOWN, 0, BATTLE_HALLOWEEN_TOWN_2, NULL, "\x82\x67\x82\x60\x82\x6b\x82\x51" },
    { WORLD_HALLOWEEN_TOWN, 0, BATTLE_HALLOWEEN_TOWN_3, NULL, "\x82\x67\x82\x60\x82\x6b\x82\x52" },
    { WORLD_HALLOWEEN_TOWN, 0, BATTLE_HALLOWEEN_TOWN_4, NULL, "\x82\x67\x82\x60\x82\x6b\x82\x53" },
    { WORLD_HALLOWEEN_TOWN, 0, BATTLE_HALLOWEEN_TOWN_5, NULL, "\x82\x67\x82\x60\x82\x6b\x82\x54" },
    { WORLD_HALLOWEEN_TOWN, 0, BATTLE_HALLOWEEN_TOWN_6, NULL, "\x82\x67\x82\x60\x82\x6b\x82\x55" },
    { WORLD_HALLOWEEN_TOWN, 0, BATTLE_HALLOWEEN_TOWN_7, NULL, "\x82\x67\x82\x60\x82\x6b\x82\x56" },
    { WORLD_HALLOWEEN_TOWN, 0, BATTLE_HALLOWEEN_TOWN_8, NULL, "\x82\x67\x82\x60\x82\x6b\x82\x57" },
    { WORLD_HALLOWEEN_TOWN, 0, BATTLE_HALLOWEEN_TOWN_9, NULL, "\x82\x67\x82\x60\x82\x6b\x82\x58" },
    { WORLD_ATLANTICA, 0, BATTLE_ATLANTICA_0, NULL, "\x82\x60\x82\x73\x82\x6b\x82\x4f" },
    { WORLD_ATLANTICA, 0, BATTLE_ATLANTICA_1, NULL, "\x82\x60\x82\x73\x82\x6b\x82\x50" },
    { WORLD_ATLANTICA, 0, BATTLE_ATLANTICA_2, NULL, "\x82\x60\x82\x73\x82\x6b\x82\x51" },
    { WORLD_ATLANTICA, 0, BATTLE_ATLANTICA_3, NULL, "\x82\x60\x82\x73\x82\x6b\x82\x52" },
    { WORLD_ATLANTICA, 0, BATTLE_ATLANTICA_4, NULL, "\x82\x60\x82\x73\x82\x6b\x82\x53" },
    { WORLD_ATLANTICA, 0, BATTLE_ATLANTICA_5, NULL, "\x82\x60\x82\x73\x82\x6b\x82\x54" },
    { WORLD_ATLANTICA, 0, BATTLE_ATLANTICA_6, NULL, "\x82\x60\x82\x73\x82\x6b\x82\x55" },
    { WORLD_ATLANTICA, 0, BATTLE_ATLANTICA_7, NULL, "\x82\x60\x82\x73\x82\x6b\x82\x56" },
    { WORLD_ATLANTICA, 0, BATTLE_ATLANTICA_8, NULL, "\x82\x60\x82\x73\x82\x6b\x82\x57" },
    { WORLD_ATLANTICA, 0, BATTLE_ATLANTICA_9, NULL, "\x82\x60\x82\x73\x82\x6b\x82\x58" },
    { WORLD_AGRABAH, 0, BATTLE_AGRABAH_0, NULL, "\x82\x60\x82\x66\x82\x71\x82\x4f" },
    { WORLD_AGRABAH, 0, BATTLE_AGRABAH_1, NULL, "\x82\x60\x82\x66\x82\x71\x82\x50" },
    { WORLD_AGRABAH, 0, BATTLE_AGRABAH_2, NULL, "\x82\x60\x82\x66\x82\x71\x82\x51" },
    { WORLD_AGRABAH, 0, BATTLE_AGRABAH_3, NULL, "\x82\x60\x82\x66\x82\x71\x82\x52" },
    { WORLD_AGRABAH, 0, BATTLE_AGRABAH_4, NULL, "\x82\x60\x82\x66\x82\x71\x82\x53" },
    { WORLD_AGRABAH, 0, BATTLE_AGRABAH_5, NULL, "\x82\x60\x82\x66\x82\x71\x82\x54" },
    { WORLD_AGRABAH, 0, BATTLE_AGRABAH_6, NULL, "\x82\x60\x82\x66\x82\x71\x82\x55" },
    { WORLD_AGRABAH, 0, BATTLE_AGRABAH_7, NULL, "\x82\x60\x82\x66\x82\x71\x82\x56" },
    { WORLD_AGRABAH, 0, BATTLE_AGRABAH_8, NULL, "\x82\x60\x82\x66\x82\x71\x82\x57" },
    { WORLD_AGRABAH, 0, BATTLE_AGRABAH_9, NULL, "\x82\x60\x82\x66\x82\x71\x82\x58" },
    { WORLD_NEVER_LAND, 0, BATTLE_NEVER_LAND_0, NULL, "\x82\x6d\x82\x64\x82\x75\x82\x4f" },
    { WORLD_NEVER_LAND, 0, BATTLE_NEVER_LAND_1, NULL, "\x82\x6d\x82\x64\x82\x75\x82\x50" },
    { WORLD_NEVER_LAND, 0, BATTLE_NEVER_LAND_2, NULL, "\x82\x6d\x82\x64\x82\x75\x82\x51" },
    { WORLD_NEVER_LAND, 0, BATTLE_NEVER_LAND_3, NULL, "\x82\x6d\x82\x64\x82\x75\x82\x52" },
    { WORLD_NEVER_LAND, 0, BATTLE_NEVER_LAND_4, NULL, "\x82\x6d\x82\x64\x82\x75\x82\x53" },
    { WORLD_NEVER_LAND, 0, BATTLE_NEVER_LAND_5, NULL, "\x82\x6d\x82\x64\x82\x75\x82\x54" },
    { WORLD_NEVER_LAND, 0, BATTLE_NEVER_LAND_6, NULL, "\x82\x6d\x82\x64\x82\x75\x82\x55" },
    { WORLD_NEVER_LAND, 0, BATTLE_NEVER_LAND_7, NULL, "\x82\x6d\x82\x64\x82\x75\x82\x56" },
    { WORLD_NEVER_LAND, 0, BATTLE_NEVER_LAND_8, NULL, "\x82\x6d\x82\x64\x82\x75\x82\x57" },
    { WORLD_NEVER_LAND, 0, BATTLE_NEVER_LAND_9, NULL, "\x82\x6d\x82\x64\x82\x75\x82\x58" },
    { WORLD_HOLLOW_BASTION, 0, BATTLE_HOLLOW_BASTION_0, NULL, "\x82\x67\x82\x6e\x82\x6b\x82\x4f" },
    { WORLD_HOLLOW_BASTION, 0, BATTLE_HOLLOW_BASTION_1, NULL, "\x82\x67\x82\x6e\x82\x6b\x82\x50" },
    { WORLD_HOLLOW_BASTION, 0, BATTLE_HOLLOW_BASTION_2, NULL, "\x82\x67\x82\x6e\x82\x6b\x82\x51" },
    { WORLD_HOLLOW_BASTION, 0, BATTLE_HOLLOW_BASTION_3, NULL, "\x82\x67\x82\x6e\x82\x6b\x82\x52" },
    { WORLD_HOLLOW_BASTION, 0, BATTLE_HOLLOW_BASTION_4, NULL, "\x82\x67\x82\x6e\x82\x6b\x82\x53" },
    { WORLD_HOLLOW_BASTION, 0, BATTLE_HOLLOW_BASTION_5, NULL, "\x82\x67\x82\x6e\x82\x6b\x82\x54" },
    { WORLD_HOLLOW_BASTION, 0, BATTLE_HOLLOW_BASTION_6, NULL, "\x82\x67\x82\x6e\x82\x6b\x82\x55" },
    { WORLD_HOLLOW_BASTION, 0, BATTLE_HOLLOW_BASTION_7, NULL, "\x82\x67\x82\x6e\x82\x6b\x82\x56" },
    { WORLD_HOLLOW_BASTION, 0, BATTLE_HOLLOW_BASTION_8, NULL, "\x82\x67\x82\x6e\x82\x6b\x82\x57" },
    { WORLD_HOLLOW_BASTION, 0, BATTLE_HOLLOW_BASTION_9, NULL, "\x82\x67\x82\x6e\x82\x6b\x82\x58" },
    { WORLD_TWILIGHT_TOWN, 0, BATTLE_TWILIGHT_TOWN_0, NULL, "\x82\x73\x82\x76\x82\x68\x82\x4f" },
    { WORLD_TWILIGHT_TOWN, 0, BATTLE_TWILIGHT_TOWN_1, NULL, "\x82\x73\x82\x76\x82\x68\x82\x50" },
    { WORLD_TWILIGHT_TOWN, 0, BATTLE_TWILIGHT_TOWN_2, NULL, "\x82\x73\x82\x76\x82\x68\x82\x51" },
    { WORLD_TWILIGHT_TOWN, 0, BATTLE_TWILIGHT_TOWN_3, NULL, "\x82\x73\x82\x76\x82\x68\x82\x52" },
    { WORLD_TWILIGHT_TOWN, 0, BATTLE_TWILIGHT_TOWN_4, NULL, "\x82\x73\x82\x76\x82\x68\x82\x53" },
    { WORLD_TWILIGHT_TOWN, 0, BATTLE_TWILIGHT_TOWN_5, NULL, "\x82\x73\x82\x76\x82\x68\x82\x54" },
    { WORLD_TWILIGHT_TOWN, 0, BATTLE_TWILIGHT_TOWN_6, NULL, "\x82\x73\x82\x76\x82\x68\x82\x55" },
    { WORLD_TWILIGHT_TOWN, 0, BATTLE_TWILIGHT_TOWN_7, NULL, "\x82\x73\x82\x76\x82\x68\x82\x56" },
    { WORLD_TWILIGHT_TOWN, 0, BATTLE_TWILIGHT_TOWN_8, NULL, "\x82\x73\x82\x76\x82\x68\x82\x57" },
    { WORLD_TWILIGHT_TOWN, 0, BATTLE_TWILIGHT_TOWN_9, NULL, "\x82\x73\x82\x76\x82\x68\x82\x58" },
    { WORLD_DESTINY_ISLANDS, 0, BATTLE_DESTINY_ISLANDS_0, NULL, "\x82\x63\x82\x64\x82\x72\x82\x4f" },
    { WORLD_DESTINY_ISLANDS, 0, BATTLE_DESTINY_ISLANDS_1, NULL, "\x82\x63\x82\x64\x82\x72\x82\x50" },
    { WORLD_DESTINY_ISLANDS, 0, BATTLE_DESTINY_ISLANDS_2, NULL, "\x82\x63\x82\x64\x82\x72\x82\x51" },
    { WORLD_DESTINY_ISLANDS, 0, BATTLE_DESTINY_ISLANDS_3, NULL, "\x82\x63\x82\x64\x82\x72\x82\x52" },
    { WORLD_DESTINY_ISLANDS, 0, BATTLE_DESTINY_ISLANDS_4, NULL, "\x82\x63\x82\x64\x82\x72\x82\x53" },
    { WORLD_DESTINY_ISLANDS, 0, BATTLE_DESTINY_ISLANDS_5, NULL, "\x82\x63\x82\x64\x82\x72\x82\x54" },
    { WORLD_DESTINY_ISLANDS, 0, BATTLE_DESTINY_ISLANDS_6, NULL, "\x82\x63\x82\x64\x82\x72\x82\x55" },
    { WORLD_DESTINY_ISLANDS, 0, BATTLE_DESTINY_ISLANDS_7, NULL, "\x82\x63\x82\x64\x82\x72\x82\x56" },
    { WORLD_DESTINY_ISLANDS, 0, BATTLE_DESTINY_ISLANDS_8, NULL, "\x82\x63\x82\x64\x82\x72\x82\x57" },
    { WORLD_DESTINY_ISLANDS, 0, BATTLE_DESTINY_ISLANDS_9, NULL, "\x82\x63\x82\x64\x82\x72\x82\x58" },
    { WORLD_CASTLE_OBLIVION, 0, BATTLE_CASTLE_OBLIVION_0, NULL, "\x82\x62\x82\x72\x82\x6b\x82\x4f" },
    { WORLD_CASTLE_OBLIVION, 0, BATTLE_CASTLE_OBLIVION_1, NULL, "\x82\x62\x82\x72\x82\x6b\x82\x50" },
    { WORLD_CASTLE_OBLIVION, 0, BATTLE_CASTLE_OBLIVION_2, NULL, "\x82\x62\x82\x72\x82\x6b\x82\x51" },
    { WORLD_CASTLE_OBLIVION, 0, BATTLE_CASTLE_OBLIVION_3, NULL, "\x82\x62\x82\x72\x82\x6b\x82\x52" },
    { WORLD_CASTLE_OBLIVION, 0, BATTLE_CASTLE_OBLIVION_4, NULL, "\x82\x62\x82\x72\x82\x6b\x82\x53" },
    { WORLD_CASTLE_OBLIVION, 0, BATTLE_CASTLE_OBLIVION_5, NULL, "\x82\x62\x82\x72\x82\x6b\x82\x54" },
    { WORLD_CASTLE_OBLIVION, 0, BATTLE_CASTLE_OBLIVION_6, NULL, "\x82\x62\x82\x72\x82\x6b\x82\x55" },
    { WORLD_CASTLE_OBLIVION, 0, BATTLE_CASTLE_OBLIVION_7, NULL, "\x82\x62\x82\x72\x82\x6b\x82\x56" },
    { WORLD_CASTLE_OBLIVION, 0, BATTLE_CASTLE_OBLIVION_8, NULL, "\x82\x62\x82\x72\x82\x6b\x82\x57" },
    { WORLD_CASTLE_OBLIVION, 0, BATTLE_CASTLE_OBLIVION_9, NULL, "\x82\x62\x82\x72\x82\x6b\x82\x58" },
    { WORLD_WONDERLAND, 0, BATTLE_CARD_SOLDIERS, NULL, "\x82\x73\x82\x71\x82\x74\x82\x6c\x82\x6f" },
    { WORLD_MONSTRO, 0, BATTLE_SHADOW_100, NULL, "\x82\x72\x82\x67\x82\x60\x82\x63\x82\x6e\x82\x76\x82\x50\x82\x4f\x82\x4f" },
    { WORLD_HALLOWEEN_TOWN, 0, BATTLE_EVENT_HALLOWEEN_TOWN, NULL, "\x82\x64\x82\x75\x82\x73\x81\x7c\x82\x67\x82\x60\x82\x6b" },
    { WORLD_AGRABAH, 0, BATTLE_EVENT_AGRABAH_1, NULL, "\x82\x64\x82\x75\x82\x73\x81\x7c\x82\x60\x82\x66\x82\x71\x82\x50" },
    { WORLD_AGRABAH, 0, BATTLE_EVENT_AGRABAH_2, NULL, "\x82\x64\x82\x75\x82\x73\x81\x7c\x82\x60\x82\x66\x82\x71\x82\x51" },
    { WORLD_NEVER_LAND, 0, BATTLE_BARREL_0, NULL, "\x82\x61\x82\x60\x82\x71\x82\x71\x82\x64\x82\x6b\x82\x4f" },
    { WORLD_NEVER_LAND, 0, BATTLE_BARREL_1, NULL, "\x82\x61\x82\x60\x82\x71\x82\x71\x82\x64\x82\x6b\x82\x50" },
    { WORLD_NEVER_LAND, 0, BATTLE_BARREL_2, NULL, "\x82\x61\x82\x60\x82\x71\x82\x71\x82\x64\x82\x6b\x82\x51" },
    { WORLD_HOLLOW_BASTION, 0, BATTLE_WHITE_MUSHROOM_0, NULL, "\x82\x76\x81\x5c\x82\x6c\x82\x74\x82\x72\x82\x67\x82\x71\x82\x6e\x82\x6e\x82\x6c\x82\x4f" },
    { WORLD_HOLLOW_BASTION, 0, BATTLE_WHITE_MUSHROOM_1, NULL, "\x82\x76\x81\x5c\x82\x6c\x82\x74\x82\x72\x82\x67\x82\x71\x82\x6e\x82\x6e\x82\x6c\x82\x50" },
    { WORLD_HOLLOW_BASTION, 0, BATTLE_WHITE_MUSHROOM_2, NULL, "\x82\x76\x81\x5c\x82\x6c\x82\x74\x82\x72\x82\x67\x82\x71\x82\x6e\x82\x6e\x82\x6c\x82\x51" },
    { WORLD_AGRABAH, 0, BATTLE_BLACK_FUNGUS_0, NULL, "\x82\x61\x81\x7c\x82\x65\x82\x74\x82\x6d\x82\x66\x82\x74\x82\x72\x82\x4f" },
    { WORLD_AGRABAH, 0, BATTLE_BLACK_FUNGUS_1, NULL, "\x82\x61\x81\x7c\x82\x65\x82\x74\x82\x6d\x82\x66\x82\x74\x82\x72\x82\x50" },
    { WORLD_AGRABAH, 0, BATTLE_BLACK_FUNGUS_2, NULL, "\x82\x61\x81\x7c\x82\x65\x82\x74\x82\x6d\x82\x66\x82\x74\x82\x72\x82\x51" },
    { WORLD_HOLLOW_BASTION, 0, BATTLE_RIKU_HOLLOW_BASTION_0, NULL, "\x82\x71\x82\x6a\x81\x40\x82\x67\x82\x6e\x82\x6b\x82\x4f" },
    { WORLD_HOLLOW_BASTION, 0, BATTLE_RIKU_HOLLOW_BASTION_1, NULL, "\x82\x71\x82\x6a\x81\x40\x82\x67\x82\x6e\x82\x6b\x82\x50" },
    { WORLD_HOLLOW_BASTION, 0, BATTLE_RIKU_HOLLOW_BASTION_2, NULL, "\x82\x71\x82\x6a\x81\x40\x82\x67\x82\x6e\x82\x6b\x82\x51" },
    { WORLD_HOLLOW_BASTION, 0, BATTLE_RIKU_HOLLOW_BASTION_3, NULL, "\x82\x71\x82\x6a\x81\x40\x82\x67\x82\x6e\x82\x6b\x82\x52" },
    { WORLD_HOLLOW_BASTION, 0, BATTLE_RIKU_HOLLOW_BASTION_4, NULL, "\x82\x71\x82\x6a\x81\x40\x82\x67\x82\x6e\x82\x6b\x82\x53" },
    { WORLD_HOLLOW_BASTION, 0, BATTLE_RIKU_HOLLOW_BASTION_5, NULL, "\x82\x71\x82\x6a\x81\x40\x82\x67\x82\x6e\x82\x6b\x82\x54" },
    { WORLD_HOLLOW_BASTION, 0, BATTLE_RIKU_HOLLOW_BASTION_6, NULL, "\x82\x71\x82\x6a\x81\x40\x82\x67\x82\x6e\x82\x6b\x82\x55" },
    { WORLD_HOLLOW_BASTION, 0, BATTLE_RIKU_HOLLOW_BASTION_7, NULL, "\x82\x71\x82\x6a\x81\x40\x82\x67\x82\x6e\x82\x6b\x82\x56" },
    { WORLD_HOLLOW_BASTION, 0, BATTLE_RIKU_HOLLOW_BASTION_8, NULL, "\x82\x71\x82\x6a\x81\x40\x82\x67\x82\x6e\x82\x6b\x82\x57" },
    { WORLD_HOLLOW_BASTION, 0, BATTLE_RIKU_HOLLOW_BASTION_9, NULL, "\x82\x71\x82\x6a\x81\x40\x82\x67\x82\x6e\x82\x6b\x82\x58" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy00, "\x82\x64\x82\x6c\x82\x78\x82\x4f\x82\x4f" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy01, "\x82\x64\x82\x6c\x82\x78\x82\x4f\x82\x50" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy02, "\x82\x64\x82\x6c\x82\x78\x82\x4f\x82\x51" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy03, "\x82\x64\x82\x6c\x82\x78\x82\x4f\x82\x52" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy04, "\x82\x64\x82\x6c\x82\x78\x82\x4f\x82\x53" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy06, "\x82\x64\x82\x6c\x82\x78\x82\x4f\x82\x55" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy07, "\x82\x64\x82\x6c\x82\x78\x82\x4f\x82\x56" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy08, "\x82\x64\x82\x6c\x82\x78\x82\x4f\x82\x57" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy14, "\x82\x64\x82\x6c\x82\x78\x82\x50\x82\x53" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy15, "\x82\x64\x82\x6c\x82\x78\x82\x50\x82\x54" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy16, "\x82\x64\x82\x6c\x82\x78\x82\x50\x82\x55" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy18, "\x82\x64\x82\x6c\x82\x78\x82\x50\x82\x57" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy19, "\x82\x64\x82\x6c\x82\x78\x82\x50\x82\x58" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy21, "\x82\x64\x82\x6c\x82\x78\x82\x51\x82\x50" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy22, "\x82\x64\x82\x6c\x82\x78\x82\x51\x82\x51" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy23, "\x82\x64\x82\x6c\x82\x78\x82\x51\x82\x52" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy25, "\x82\x64\x82\x6c\x82\x78\x82\x51\x82\x54" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy26, "\x82\x64\x82\x6c\x82\x78\x82\x51\x82\x55" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy27, "\x82\x64\x82\x6c\x82\x78\x82\x51\x82\x56" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy28, "\x82\x64\x82\x6c\x82\x78\x82\x51\x82\x57" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy29, "\x82\x64\x82\x6c\x82\x78\x82\x51\x82\x58" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy30, "\x82\x64\x82\x6c\x82\x78\x82\x52\x82\x4f" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy31, "\x82\x64\x82\x6c\x82\x78\x82\x52\x82\x50" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy37, "\x82\x64\x82\x6c\x82\x78\x82\x52\x82\x56" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy38, "\x82\x64\x82\x6c\x82\x78\x82\x52\x82\x57" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy39, "\x82\x64\x82\x6c\x82\x78\x82\x52\x82\x58" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy41, "\x82\x64\x82\x6c\x82\x78\x82\x53\x82\x50" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy44, "\x82\x64\x82\x6c\x82\x78\x82\x53\x82\x53" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy81, "\x82\x64\x82\x6c\x82\x78\x82\x57\x82\x50" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy82, "\x82\x64\x82\x6c\x82\x78\x82\x57\x82\x51" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmy83, "\x82\x64\x82\x6c\x82\x78\x82\x57\x82\x52" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmyTrumpH, "\x82\x67\x82\x64\x82\x60\x82\x71\x82\x73" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmyTrumpS, "\x82\x72\x82\x6f\x82\x60\x82\x63\x82\x64" },
    { WORLD_NEVER_LAND, 2, BATTLE_HOOK, NULL, "\x82\x67\x82\x6e\x82\x6e\x82\x6a\x81\x69\x82\x67\x82\x74\x82\x6c\x81\x6a" },
    { WORLD_OLYMPUS_COLISEUM, 2, BATTLE_CLOUD, NULL, "\x82\x62\x82\x6b\x82\x6e\x82\x74\x82\x63\x81\x69\x82\x67\x82\x74\x82\x6c\x81\x6a" },
    { WORLD_OLYMPUS_COLISEUM, 2, BATTLE_HADES, NULL, "\x82\x67\x82\x60\x82\x63\x82\x64\x82\x72\x81\x69\x82\x67\x82\x74\x82\x6c\x81\x6a" },
    { WORLD_CASTLE_OBLIVION, 2, BATTLE_RIKU_1, NULL, "\x82\x71\x82\x68\x82\x6a\x82\x74\x82\x50\x81\x69\x82\x67\x82\x74\x82\x6c\x81\x6a" },
    { WORLD_CASTLE_OBLIVION, 2, BATTLE_RIKU_2, NULL, "\x82\x71\x82\x68\x82\x6a\x82\x74\x82\x51\x81\x69\x82\x67\x82\x74\x82\x6c\x81\x6a" },
    { WORLD_CASTLE_OBLIVION, 2, BATTLE_RIKU_3, NULL, "\x82\x71\x82\x68\x82\x6a\x82\x74\x82\x52\x81\x69\x82\x67\x82\x74\x82\x6c\x81\x6a" },
    { WORLD_CASTLE_OBLIVION, 2, BATTLE_RIKU_4, NULL, "\x82\x71\x82\x68\x82\x6a\x82\x74\x82\x53\x81\x69\x82\x67\x82\x74\x82\x6c\x81\x6a" },
    { WORLD_CASTLE_OBLIVION, 2, BATTLE_RIKU_5, NULL, "\x82\x71\x82\x68\x82\x6a\x82\x74\x82\x54\x81\x69\x82\x67\x82\x74\x82\x6c\x81\x6a" },
    { WORLD_CASTLE_OBLIVION, 2, BATTLE_RIKU_6, NULL, "\x82\x71\x82\x68\x82\x6a\x82\x74\x82\x55\x81\x69\x82\x67\x82\x74\x82\x6c\x81\x6a" },
    { WORLD_CASTLE_OBLIVION, 2, BATTLE_AXEL_1, NULL, "\x82\x60\x82\x77\x82\x64\x82\x6b\x82\x50\x81\x69\x82\x67\x82\x74\x82\x6c\x81\x6a" },
    { WORLD_CASTLE_OBLIVION, 2, BATTLE_AXEL_2, NULL, "\x82\x60\x82\x77\x82\x64\x82\x6b\x82\x51\x81\x69\x82\x67\x82\x74\x82\x6c\x81\x6a" },
    { WORLD_CASTLE_OBLIVION, 2, BATTLE_LARXENE_1, NULL, "\x82\x6b\x82\x60\x82\x71\x82\x77\x82\x64\x82\x6d\x82\x60\x82\x50\x81\x69\x82\x67\x82\x74\x82\x6c\x81\x6a" },
    { WORLD_CASTLE_OBLIVION, 2, BATTLE_LARXENE_2, NULL, "\x82\x6b\x82\x60\x82\x71\x82\x77\x82\x64\x82\x6d\x82\x60\x82\x51\x81\x69\x82\x67\x82\x74\x82\x6c\x81\x6a" },
    { WORLD_CASTLE_OBLIVION, 2, BATTLE_VEXEN_1, NULL, "\x82\x75\x82\x64\x82\x77\x82\x64\x82\x6d\x82\x50\x81\x69\x82\x67\x82\x74\x82\x6c\x81\x6a" },
    { WORLD_CASTLE_OBLIVION, 2, BATTLE_VEXEN_2, NULL, "\x82\x75\x82\x64\x82\x77\x82\x64\x82\x6d\x82\x51\x81\x69\x82\x67\x82\x74\x82\x6c\x81\x6a" },
    { WORLD_CASTLE_OBLIVION, 2, BATTLE_VEXEN_3, NULL, "\x82\x75\x82\x64\x82\x77\x82\x64\x82\x6d\x82\x52\x81\x69\x82\x67\x82\x74\x82\x6c\x81\x6a" },
    { WORLD_CASTLE_OBLIVION, 2, BATTLE_MARLUXIA, NULL, "\x82\x6c\x82\x60\x82\x71\x82\x6b\x82\x74\x82\x77\x82\x68\x82\x60\x81\x69\x82\x67\x82\x74\x82\x6c\x81\x6a" },
    { WORLD_CASTLE_OBLIVION, 2, BATTLE_ANSEM_1, NULL, "\x82\x60\x82\x6d\x82\x72\x82\x64\x82\x6c\x82\x50\x81\x69\x82\x67\x82\x74\x82\x6c\x81\x6a" },
    { WORLD_CASTLE_OBLIVION, 2, BATTLE_ANSEM_2, NULL, "\x82\x60\x82\x6d\x82\x72\x82\x64\x82\x6c\x82\x51\x81\x69\x82\x67\x82\x74\x82\x6c\x81\x6a" },
    { WORLD_CASTLE_OBLIVION, 2, BATTLE_LEXAEUS, NULL, "\x82\x6b\x82\x64\x82\x77\x82\x60\x82\x64\x82\x74\x82\x72\x81\x69\x82\x67\x82\x74\x82\x6c\x81\x6a" },
    { WORLD_MONSTRO, 2, BATTLE_PARASITE_CAGE, NULL, "\x82\x6f\x82\x60\x82\x71\x82\x60\x82\x72\x82\x68\x82\x73\x82\x64\x82\x62\x82\x60\x82\x66\x82\x64" },
    { WORLD_DESTINY_ISLANDS, 2, BATTLE_DARKSIDE, NULL, "\x82\x63\x82\x60\x82\x71\x82\x6a\x82\x72\x82\x68\x82\x63\x82\x64" },
    { WORLD_TRAVERSE_TOWN, 2, BATTLE_GUARD_ARMOR, NULL, "\x82\x66\x82\x74\x82\x60\x82\x71\x82\x63\x82\x60\x82\x71\x82\x6e\x82\x6c\x82\x6e\x82\x71" },
    { WORLD_WONDERLAND, 2, BATTLE_TRICKMASTER, NULL, "\x82\x73\x82\x71\x82\x68\x82\x62\x82\x6a\x82\x6c\x82\x60\x82\x72\x82\x73\x82\x64\x82\x71" },
    { WORLD_HALLOWEEN_TOWN, 2, BATTLE_OOGIE_BOOGIE, NULL, "\x82\x61\x82\x6e\x82\x6e\x82\x66\x82\x68\x82\x64" },
    { WORLD_HOLLOW_BASTION, 2, BATTLE_DRAGON_MALEFICENT, NULL, "\x82\x6c\x81\x7c\x82\x63\x82\x71\x82\x60\x82\x66\x82\x6e\x82\x6d" },
    { WORLD_ATLANTICA, 2, BATTLE_URSULA, NULL, "\x82\x74\x82\x71\x82\x72\x82\x74\x82\x6b\x82\x60" },
    { WORLD_AGRABAH, 2, BATTLE_JAFAR, NULL, "\x82\x69\x82\x60\x82\x65\x82\x60\x82\x71" },
    { WORLD_CASTLE_OBLIVION, 2, BATTLE_MARLUXIA_2, NULL, "\x82\x6c\x82\x60\x82\x71\x82\x6b\x82\x74\x82\x77\x82\x68\x82\x60\x81\x7c\x82\x51" },
    { WORLD_CASTLE_OBLIVION, 1, BATTLE_ENEMY_TEST, &gTaskDescEmyTest, "\x82\x73\x82\x64\x82\x72\x82\x73" },
    { WORLD_TRAVERSE_TOWN, 1, BATTLE_TUTORIAL_0, NULL, "\x82\x73\x82\x74\x82\x73\x82\x6e\x82\x71\x82\x68\x82\x60\x82\x6b\x82\x4f" },
    { WORLD_TRAVERSE_TOWN, 1, BATTLE_TUTORIAL_1, NULL, "\x82\x73\x82\x74\x82\x73\x82\x6e\x82\x71\x82\x68\x82\x60\x82\x6b\x82\x50" },
};

static const ChkBtlWorld sChkBtlWorlds[13] = {
    { BATTLE_STAGE_TWILIGHT_TOWN, "\x82\x73\x82\x76\x82\x68" },
    { BATTLE_STAGE_TRAVERSE_TOWN, "\x82\x73\x82\x76\x82\x6d" },
    { BATTLE_STAGE_WONDERLAND, "\x82\x76\x82\x6e\x82\x6d" },
    { BATTLE_STAGE_GARDEN, "\x82\x66\x82\x71\x82\x63" },
    { BATTLE_STAGE_OLYMPUS_COLISEUM, "\x82\x62\x82\x6e\x82\x6b" },
    { BATTLE_STAGE_MONSTRO, "\x82\x6c\x82\x6e\x82\x6d" },
    { BATTLE_STAGE_NEVER_LAND, "\x82\x6d\x82\x64\x82\x75" },
    { BATTLE_STAGE_HALLOWEEN_TOWN, "\x82\x67\x82\x60\x82\x6b" },
    { BATTLE_STAGE_AGRABAH, "\x82\x60\x82\x66\x82\x71" },
    { BATTLE_STAGE_ATLANTICA, "\x82\x60\x82\x73\x82\x6b" },
    { BATTLE_STAGE_HOLLOW_BASTION, "\x82\x67\x82\x6e\x82\x6b" },
    { BATTLE_STAGE_DESTINY_ISLANDS, "\x82\x63\x82\x64\x82\x72" },
    { BATTLE_STAGE_CASTLE_OBLIVION, "\x82\x62\x82\x72\x82\x6b" },
};

const char gWhitePalette[32] = "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff";

static void StartSelectedBattle(void) {
        SeedRandom(gFrameCounter);
        gGameState.battleStage = sChkBtlWorlds[gChkBtlWork->bg].world;
        gGameState.floor = gChkBtlWork->floor;
        gGameState.hp = gChkBtlWork->hp;
        gGameState.progression.maxHp = gChkBtlWork->hp;
        gGameState.world = sChkBtlEntries[gChkBtlWork->enemy].world;

        if ((GetKeysHeld() & L_BUTTON)
#ifdef PLATFORM_PC
            && !PortPracticeActive()
#endif
        ) {
            gGameState.flags |= GAME_FLAG_FIRST_STRIKE;
        } else {
            gGameState.flags &= ~GAME_FLAG_FIRST_STRIKE;
        }

        if (gGameState.flags & GAME_FLAG_RIKU) {
            InitRikuDeckForWorld(gChkBtlWork->floor);
        }

        ModeRequest(&gModeBattle, sChkBtlEntries[gChkBtlWork->enemy].battleId);
}

void mode_chkbtl_0() {
    FadeStartIn(FADE_MODE_BLACK, 8);
    SetBgMode0();
    SetupBg(0, 0, 15, 0);
    EnableBg(0);
    DebugTextInit(0, 0x5400, 0x500);
    DebugTextLoadPalette(0, gWhitePalette, sizeof(gWhitePalette), 0x0F);
#ifdef PLATFORM_PC
    if (!PortPracticeActive()) {
#endif
    DebugTextPrint(0, 0, 2, "\x82\x63\x82\x64\x82\x62\x82\x6a\x81\x7c\x82\x72\x82\x64\x82\x6b\x82\x64\x82\x62\x82\x73\x81\x40\x82\x61\x82\x74\x82\x73\x82\x73\x82\x6e\x82\x6d");
    DebugTextPrint(24, 32, 2, "\x82\x64\x82\x6d\x82\x6c\x81\x46");
    DebugTextPrint(24, 44, 2, "\x82\x61\x82\x66\x81\x40\x81\x46");
    DebugTextPrint(24, 56, 2, "\x82\x65\x82\x6b\x81\x40\x81\x46");
    DebugTextPrint(24, 68, 2, "\x82\x67\x82\x6f\x81\x40\x81\x46");
    DebugTextPrint(62, 32, 2, sChkBtlEntries[gChkBtlWork->enemy].name);

    if (sChkBtlEntries[gChkBtlWork->enemy].kind == 2) {
        DebugTextPrint(62, 44, 2, "\x81\x5c\x81\x5c");
    } else {
        DebugTextPrint(62, 44, 2, sChkBtlWorlds[gChkBtlWork->bg].name);
    }

    DebugTextPrintNumber(62, 56, 2, gChkBtlWork->floor + 1);
    DebugTextPrintNumber(62, 68, 2, gChkBtlWork->hp);

#ifdef PLATFORM_PC
    }
    if (PortPracticeActive()) {
        PortPracticeEnterMenu();
        PortLog("practice: menu frame=%u",(unsigned)gFrameCounter);
    }
#endif

    if (!(gDebugFlags & DEBUG_FLAG_CHKBTL)) {
        func_08085FB0();
        InitDebugDecks();
#ifdef PLATFORM_PC
        {
            /* test harness: KHCOM_BATTLE_DECK=0..2 picks the debug deck (2: the mixed one, magic and items) */
            extern char* getenv(const char*);
            extern int atoi(const char*);
            void SetActiveDeckIndex(u8 index);
            const char* deck = getenv("KHCOM_BATTLE_DECK");
            if (deck)
                SetActiveDeckIndex((u8)atoi(deck));
        }
#endif
        gGameState.progression.cp = 9999;
        gDebugFlags |= DEBUG_FLAG_CHKBTL;
        gGameState.progression.friendFlags = 0xFFFF;
    }
}

void mode_chkbtl_1() {
    s32 i;
#ifdef PLATFORM_PC
    if (PortPracticeActive()) {
        u16 keys=GetKeysPressed();
        unsigned command=PortPracticeMenuInput(
            ((keys&DPAD_UP)?PORT_PRACTICE_UP:0)|((keys&DPAD_DOWN)?PORT_PRACTICE_DOWN:0)|
            ((keys&(A_BUTTON|START_BUTTON))?PORT_PRACTICE_START:0)|
            ((keys&B_BUTTON)?PORT_PRACTICE_BACK:0));
        if(command==PORT_PRACTICE_START) {
            gChkBtlWork->enemy=PortPracticeEntry();
            gChkBtlWork->floor=0;gChkBtlWork->hp=560;
            PortLog("practice: start frame=%u preset=%u entry=%d name=%s",(unsigned)gFrameCounter,
                PortPracticeSelection(),PortPracticeEntry(),PortPracticeName());
            PortPracticeLaunch();
            StartSelectedBattle();
            PortLog("practice: rules first_strike=%u hp=%u",(unsigned)((gGameState.flags&GAME_FLAG_FIRST_STRIKE)!=0),
                (unsigned)gGameState.hp);
        } else if(command==PORT_PRACTICE_BACK) {
            PortLog("practice: title frame=%u",(unsigned)gFrameCounter);
            PortPracticeLeave();
            gDebugFlags&=~DEBUG_FLAG_CHKBTL;
            ModeRequest(&gModeTitle,0);
        }
        return;
    }
#endif

    if (GetKeysRepeat() & DPAD_UP) {
        gChkBtlWork->cursor--;
    }

    if (GetKeysRepeat() & DPAD_DOWN) {
        gChkBtlWork->cursor++;
    }

    if (GetKeysRepeat() & (DPAD_UP | DPAD_DOWN)) {
        for (i = 0; i < 4; i++) {
            DebugTextPrint(12, i * 12 + 32, 2, "\x81\x40");
        }
    }

    switch (gChkBtlWork->cursor) {
    case 0:
        if (GetKeysRepeat() & DPAD_LEFT) {
            gChkBtlWork->enemy--;
        }

        if (GetKeysRepeat() & DPAD_RIGHT) {
            gChkBtlWork->enemy++;
        }

        if (gChkBtlWork->enemy < 0) {
            gChkBtlWork->enemy = 0xD0;
        } else if ((u16)gChkBtlWork->enemy > 0xD0) {
            gChkBtlWork->enemy = 0;
        }

        break;
    case 1:
        if (GetKeysRepeat() & DPAD_LEFT) {
            gChkBtlWork->bg--;
        }

        if (GetKeysRepeat() & DPAD_RIGHT) {
            gChkBtlWork->bg++;
        }

        if (gChkBtlWork->bg < 0) {
            gChkBtlWork->bg = 12;
        } else if ((u8)gChkBtlWork->bg > 12) {
            gChkBtlWork->bg = 0;
        }

        break;
    case 2:
        if (GetKeysRepeat() & DPAD_LEFT) {
            gChkBtlWork->floor--;
        }

        if (GetKeysRepeat() & DPAD_RIGHT) {
            gChkBtlWork->floor++;
        }

        if (gChkBtlWork->floor < 0) {
            gChkBtlWork->floor = 0;
        }

        if (gGameState.flags & GAME_FLAG_RIKU) {
            if (gChkBtlWork->floor > 11) {
                gChkBtlWork->floor = 11;
            }
        } else {
            if (gChkBtlWork->floor > 12) {
                gChkBtlWork->floor = 12;
            }
        }

        break;
    case 3:
        if (GetKeysRepeat() & DPAD_LEFT) {
            gChkBtlWork->hp--;
        }

        if (GetKeysRepeat() & DPAD_RIGHT) {
            gChkBtlWork->hp++;
        }

        if (gChkBtlWork->hp <= 0) {
            gChkBtlWork->hp = 1;
        } else if (gChkBtlWork->hp > 560) {
            gChkBtlWork->hp = 560;
        }

        break;
    case 4:
        gChkBtlWork->cursor = 0;
        break;
    case -1:
        gChkBtlWork->cursor = 3;
        break;
    }

    DebugTextPrint(12, gChkBtlWork->cursor * 12 + 32, 2, "\x81\x84");

    if (GetKeysRepeat() & (DPAD_RIGHT | DPAD_LEFT)) {
        switch (gChkBtlWork->cursor) {
        case 0:
        case 1:
            DebugTextPrint(62, 32, 2, "\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40");
            DebugTextPrint(62, 32, 2, sChkBtlEntries[gChkBtlWork->enemy].name);
            DebugTextPrint(62, 44, 2, "\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40");

            if (sChkBtlEntries[gChkBtlWork->enemy].kind == 2) {
                DebugTextPrint(62, 44, 2, "\x81\x5c\x81\x5c");
            } else {
                DebugTextPrint(62, 44, 2, sChkBtlWorlds[gChkBtlWork->bg].name);
            }

            break;
        case 2:
            DebugTextPrintNumber(62, 56, 2, gChkBtlWork->floor + 1);
            break;
        case 3:
            DebugTextPrintNumber(62, 68, 2, gChkBtlWork->hp);
            break;
        }
    }

    if (GetKeysPressed() & SELECT_BUTTON) {
        ModeRequest(&gModeDeck, 0);
    } else if (GetKeysPressed() & (A_BUTTON | START_BUTTON)) {
        StartSelectedBattle();
    } else if (GetKeysPressed() & B_BUTTON) {
        ModeRequest(&gModeDebug, 0);
        return;
    }

    DebugTextDraw(0);
    DebugTextClear();
}

void mode_chkbtl_2() {
    DebugTextDestroy();
}

void ChkBtlSpawnEnemy() {
    const ChkBtlEntry* entry;
    ChkBtlPos pos;

    entry = &sChkBtlEntries[gChkBtlWork->enemy];

    if (entry->battleId == BATTLE_ENEMY_TEST) {
        pos.x = 0x15000;
        pos.y = 0x16000;
        pos.z = 0;
#ifdef PLATFORM_PC
        if(PortPracticeActive() && PortPracticeSelection()==0 && gBtlWork->actor) {
            Task* task;
            pos.x=gBtlWork->actor->x+48*256;pos.y=gBtlWork->actor->y;
            task=TaskCreate(&gBtlWork->taskPools[0],entry->taskDesc,&pos);
            if(task && task->work) {
                EmyWork* enemy=task->work;
                enemy->actor.hp=enemy->actor.maxHp=10000;
            }
        } else
#endif
        TaskCreate(&gBtlWork->taskPools[0], entry->taskDesc, &pos);
    }
}

void ChkBtlReset() {
    gChkBtlWork->cursor = 0;
    gChkBtlWork->bg = 0;
    gChkBtlWork->enemy = 0;
    gChkBtlWork->floor = 0;
    gChkBtlWork->hp = gGameState.progression.maxHp;
    gDebugFlags &= ~DEBUG_FLAG_CHKBTL;
    gVsBattleHalfWidth = 0x98;
    gVsBattleMinY = 0x160;
    gVsBattleMaxY = 0x1A2;
}

ChkBtlWork* gChkBtlWork = &sChkBtlWork;

Mode gModeChkbtl = { "mode_chkbtl", mode_chkbtl_0, mode_chkbtl_1, mode_chkbtl_2 };
