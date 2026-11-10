/**
 * btl_effect.c
 * Battle Background Effects
 */

#include "display.h"
#include "battle.h"
#include "battle_bg_animations.h"
#include "malloc.h"
#include "fade.h"
#include "songs.h"
#include <stdlib.h>
#include "battle_actor.h"
#include "battle_actor_types.h"
#include "battle_work.h"
#include "bg_animation_types.h"
#include "btl_collision.h"
#include "engine_math.h"
#include "listpool.h"
#include "m4a_song.h"
#include "types.h"
#include <stddef.h>
#include "btl.h"
#include "btl_effect.h"
#include "hum.h"
#include "enemy_ids.h"
#include "gba/io_reg.h"

static const BgAnimationChunk sBgAnimationChunks[108] = {
    { gBgAnimCure00Tiles, sizeof(gBgAnimCure00Tiles) },
    { gBgAnimFire00Tiles0, sizeof(gBgAnimFire00Tiles0) },
    { gBgAnimFire00Tiles1, sizeof(gBgAnimFire00Tiles1) },
    { gBgAnimFire01Tiles, sizeof(gBgAnimFire01Tiles) },
    { gBgAnimFire02Tiles, sizeof(gBgAnimFire02Tiles) },
    { gBgAnimExplosionTiles, sizeof(gBgAnimExplosionTiles) },
    { gBgAnimFlashTiles, sizeof(gBgAnimFlashTiles) },
    { gBgAnimSoraHitTiles, sizeof(gBgAnimSoraHitTiles) },
    { gBgAnimLimitTiles, sizeof(gBgAnimLimitTiles) },
    { gBgAnimDashRingTiles, sizeof(gBgAnimDashRingTiles) },
    { gBgAnimEnemyHitTiles, sizeof(gBgAnimEnemyHitTiles) },
    { gBgAnimPotionTiles, sizeof(gBgAnimPotionTiles) },
    { gBgAnimBlizzard00Tiles0, sizeof(gBgAnimBlizzard00Tiles0) },
    { gBgAnimBlizzard00Tiles1, sizeof(gBgAnimBlizzard00Tiles1) },
    { gBgAnimBlizzard01Tiles, sizeof(gBgAnimBlizzard01Tiles) },
    { gBgAnimBlizzard02Tiles, sizeof(gBgAnimBlizzard02Tiles) },
    { gBgAnimBlizzard03Tiles0, sizeof(gBgAnimBlizzard03Tiles0) },
    { gBgAnimBlizzard03Tiles1, sizeof(gBgAnimBlizzard03Tiles1) },
    { gBgAnimThunder00Tiles, sizeof(gBgAnimThunder00Tiles) },
    { gBgAnimThunder01Tiles, sizeof(gBgAnimThunder01Tiles) },
    { gBgAnimThunder02Tiles0, sizeof(gBgAnimThunder02Tiles0) },
    { gBgAnimThunder02Tiles1, sizeof(gBgAnimThunder02Tiles1) },
    { gBgAnimThunder02Tiles2, sizeof(gBgAnimThunder02Tiles2) },
    { gBgAnimThunder03Tiles0, sizeof(gBgAnimThunder03Tiles0) },
    { gBgAnimThunder03Tiles1, sizeof(gBgAnimThunder03Tiles1) },
    { gBgAnimThunder03Tiles2, sizeof(gBgAnimThunder03Tiles2) },
    { gBgAnimCharaDefeatEndTiles, sizeof(gBgAnimCharaDefeatEndTiles) },
    { gBgAnimEnemyDeathTiles, sizeof(gBgAnimEnemyDeathTiles) },
    { gBgAnimDarkDeathTiles, sizeof(gBgAnimDarkDeathTiles) },
    { gBgAnimCure02Tiles0, sizeof(gBgAnimCure02Tiles0) },
    { gBgAnimCure02Tiles1, sizeof(gBgAnimCure02Tiles1) },
    { gBgAnimCure01Tiles0, sizeof(gBgAnimCure01Tiles0) },
    { gBgAnimCure01Tiles1, sizeof(gBgAnimCure01Tiles1) },
    { gBgAnimFire03Tiles0, sizeof(gBgAnimFire03Tiles0) },
    { gBgAnimFire03Tiles1, sizeof(gBgAnimFire03Tiles1) },
    { gBgAnimFriendHitTiles, sizeof(gBgAnimFriendHitTiles) },
    { gBgAnimStop00Tiles, sizeof(gBgAnimStop00Tiles) },
    { gBgAnimStop01Tiles, sizeof(gBgAnimStop01Tiles) },
    { gBgAnimStop02Tiles0, sizeof(gBgAnimStop02Tiles0) },
    { gBgAnimStop02Tiles1, sizeof(gBgAnimStop02Tiles1) },
    { gBgAnimSummonTiles0, sizeof(gBgAnimSummonTiles0) },
    { gBgAnimSummonTiles1, sizeof(gBgAnimSummonTiles1) },
    { gBgAnimGroundImpactTiles0, sizeof(gBgAnimGroundImpactTiles0) },
    { gBgAnimGroundImpactTiles1, sizeof(gBgAnimGroundImpactTiles1) },
    { gBgAnimBtlStartTiles0, sizeof(gBgAnimBtlStartTiles0) },
    { gBgAnimBtlStartTiles1, sizeof(gBgAnimBtlStartTiles1) },
    { gBgAnimBtlStartTiles2, sizeof(gBgAnimBtlStartTiles2) },
    { gBgAnimBtlStartTiles3, sizeof(gBgAnimBtlStartTiles3) },
    { gBgAnimBtlStartTiles4, sizeof(gBgAnimBtlStartTiles4) },
    { gBgAnimBtlStartTiles5, sizeof(gBgAnimBtlStartTiles5) },
    { gBgAnimGuardTiles, sizeof(gBgAnimGuardTiles) },
    { gBgAnimCharaDefeatTiles0, sizeof(gBgAnimCharaDefeatTiles0) },
    { gBgAnimCharaDefeatTiles1, sizeof(gBgAnimCharaDefeatTiles1) },
    { gBgAnimHumDefeatTiles0, sizeof(gBgAnimHumDefeatTiles0) },
    { gBgAnimHumDefeatTiles1, sizeof(gBgAnimHumDefeatTiles1) },
    { gBgAnimGravity00Tiles, sizeof(gBgAnimGravity00Tiles) },
    { gBgAnimGravity01Tiles0, sizeof(gBgAnimGravity01Tiles0) },
    { gBgAnimGravity01Tiles1, sizeof(gBgAnimGravity01Tiles1) },
    { gBgAnimPremireChanceTiles0, sizeof(gBgAnimPremireChanceTiles0) },
    { gBgAnimPremireChanceTiles1, sizeof(gBgAnimPremireChanceTiles1) },
    { gBgAnimGasTiles, sizeof(gBgAnimGasTiles) },
    { gBgAnimEnemySpawnTiles, sizeof(gBgAnimEnemySpawnTiles) },
    { gBgAnimBoogieKaihukuTiles, sizeof(gBgAnimBoogieKaihukuTiles) },
    { gBgAnimPcShotTiles, sizeof(gBgAnimPcShotTiles) },
    { gBgAnimGlowTiles, sizeof(gBgAnimGlowTiles) },
    { gBgAnimBossDeathTiles0, sizeof(gBgAnimBossDeathTiles0) },
    { gBgAnimBossDeathTiles1, sizeof(gBgAnimBossDeathTiles1) },
    { gBgAnimDumboSplashTiles, sizeof(gBgAnimDumboSplashTiles) },
    { gBgAnimTrinityLimitTiles, sizeof(gBgAnimTrinityLimitTiles) },
    { gBgAnimTrinityLimitChargeTiles0, sizeof(gBgAnimTrinityLimitChargeTiles0) },
    { gBgAnimTrinityLimitChargeTiles1, sizeof(gBgAnimTrinityLimitChargeTiles1) },
    { gBgAnimTrinityLimitBlastTiles, sizeof(gBgAnimTrinityLimitBlastTiles) },
    { gBgAnimRagnarokChargeTiles, sizeof(gBgAnimRagnarokChargeTiles) },
    { gBgAnimRagnarokShotTiles0, sizeof(gBgAnimRagnarokShotTiles0) },
    { gBgAnimRagnarokShotTiles1, sizeof(gBgAnimRagnarokShotTiles1) },
    { gBgAnimUrsulaBeamTiles, sizeof(gBgAnimUrsulaBeamTiles) },
    { gBgAnimJfMajinBeamTiles, sizeof(gBgAnimJfMajinBeamTiles) },
    { gBgAnimAnsemRushTiles, sizeof(gBgAnimAnsemRushTiles) },
    { gBgAnimAnsemWaveTiles, sizeof(gBgAnimAnsemWaveTiles) },
    { gBgAnimStunImpactTiles, sizeof(gBgAnimStunImpactTiles) },
    { gBgAnimUrsulaThunderTiles, sizeof(gBgAnimUrsulaThunderTiles) },
    { gBgAnimWorldSelectTiles, sizeof(gBgAnimWorldSelectTiles) },
    { gBgAnimWorldStartTiles0, sizeof(gBgAnimWorldStartTiles0) },
    { gBgAnimWorldStartTiles1, sizeof(gBgAnimWorldStartTiles1) },
    { gBgAnimXmasTiles0, sizeof(gBgAnimXmasTiles0) },
    { gBgAnimXmasTiles1, sizeof(gBgAnimXmasTiles1) },
    { gBgAnimLightPillarTiles, sizeof(gBgAnimLightPillarTiles) },
    { gBgAnimTornadoTiles, sizeof(gBgAnimTornadoTiles) },
    { gBgAnimAxcelFireWallTiles0, sizeof(gBgAnimAxcelFireWallTiles0) },
    { gBgAnimAxcelFireWallTiles1, sizeof(gBgAnimAxcelFireWallTiles1) },
    { gBgAnimMahluxiaGroundTiles0, sizeof(gBgAnimMahluxiaGroundTiles0) },
    { gBgAnimMahluxiaGroundTiles1, sizeof(gBgAnimMahluxiaGroundTiles1) },
    { gBgAnimHanabiraTiles, sizeof(gBgAnimHanabiraTiles) },
    { gBgAnimKamaTiles, sizeof(gBgAnimKamaTiles) },
    { gBgAnimDragonFireTiles0, sizeof(gBgAnimDragonFireTiles0) },
    { gBgAnimDragonFireTiles1, sizeof(gBgAnimDragonFireTiles1) },
    { gBgAnimLaxeneBeamTiles0, sizeof(gBgAnimLaxeneBeamTiles0) },
    { gBgAnimLaxeneBeamTiles1, sizeof(gBgAnimLaxeneBeamTiles1) },
    { gBgAnimAeroTiles, sizeof(gBgAnimAeroTiles) },
    { gBgAnimRikuHitTiles, sizeof(gBgAnimRikuHitTiles) },
    { gBgAnimRikuDarkModeFlashTiles0, sizeof(gBgAnimRikuDarkModeFlashTiles0) },
    { gBgAnimRikuDarkModeFlashTiles1, sizeof(gBgAnimRikuDarkModeFlashTiles1) },
    { gBgAnimLstCtrTiles0, sizeof(gBgAnimLstCtrTiles0) },
    { gBgAnimLstCtrTiles1, sizeof(gBgAnimLstCtrTiles1) },
    { gBgAnimLstCtrTiles2, sizeof(gBgAnimLstCtrTiles2) },
    { gBgAnimDsdTransitionTiles, sizeof(gBgAnimDsdTransitionTiles) },
    { gBgAnimRikuLimitFinishTiles, sizeof(gBgAnimRikuLimitFinishTiles) },
    { gBgAnimRikuDiveHitTiles, sizeof(gBgAnimRikuDiveHitTiles) },
};

BgAnimationDef gBgAnimDefCure00 = { &sBgAnimationChunks[0], gBgAnimCure00Map, gBgAnimCure00Palette, sizeof(gBgAnimCure00Palette), 58, 16, 16, 8, 6 };

BgAnimationDef gBgAnimDefFire00 = { &sBgAnimationChunks[1], gBgAnimFire00Map, gBgAnimFire00Palette, sizeof(gBgAnimFire00Palette), 56, 21, 16, 10, 3 };

BgAnimationDef gBgAnimDefRikuFire00 = { &sBgAnimationChunks[1], gBgAnimFire00Map, gBgAnimRikuFire00Palette, sizeof(gBgAnimRikuFire00Palette), 56, 21, 16, 10, 3 };

BgAnimationDef gBgAnimDefFlame = { &sBgAnimationChunks[1], gBgAnimFire00Map, gBgAnimFire00Palette, sizeof(gBgAnimFire00Palette), 56, 12, 16, 3, 4 };

BgAnimationDef gBgAnimDefFire01 = { &sBgAnimationChunks[3], gBgAnimFire01Map, gBgAnimFire01Palette, sizeof(gBgAnimFire01Palette), 41, 16, 16, 7, 4 };

static BgAnimationDef sBgAnimDefFireHit = { &sBgAnimationChunks[3], gBgAnimFire01Map, gBgAnimFire01Palette, sizeof(gBgAnimFire01Palette), 41, 16, 16, 7, 2 };

BgAnimationDef gBgAnimDefFire02 = { &sBgAnimationChunks[4], gBgAnimFire02Map, gBgAnimFire02Palette, sizeof(gBgAnimFire02Palette), 48, 16, 16, 7, 4 };

BgAnimationDef gBgAnimDefExplosion = { &sBgAnimationChunks[5], gBgAnimExplosionMap, gBgAnimExplosionPalette, sizeof(gBgAnimExplosionPalette), 33, 16, 16, 8, 6 };

BgAnimationDef gBgAnimDefFlash = { &sBgAnimationChunks[6], gBgAnimFlashMap, gBgAnimFlashPalette, sizeof(gBgAnimFlashPalette), 69, 16, 16, 1, 65535 };

BgAnimationDef gBgAnimDefSoraHit = { &sBgAnimationChunks[7], gBgAnimSoraHitMap, gBgAnimSoraHitPalette, sizeof(gBgAnimSoraHitPalette), 37, 16, 16, 4, 3 };

BgAnimationDef gBgAnimDefLimit = { &sBgAnimationChunks[8], gBgAnimLimitMap, gBgAnimLimitPalette, sizeof(gBgAnimLimitPalette), 42, 16, 16, 6, 3 };

BgAnimationDef gBgAnimDefDashRing = { &sBgAnimationChunks[9], gBgAnimDashRingMap, gBgAnimDashRingPalette, sizeof(gBgAnimDashRingPalette), 74, 16, 16, 5, 3 };

BgAnimationDef gBgAnimDefEnemyHit = { &sBgAnimationChunks[10], gBgAnimEnemyHitMap, gBgAnimEnemyHitPalette, sizeof(gBgAnimEnemyHitPalette), 36, 16, 16, 4, 4 };

BgAnimationDef gBgAnimDefEnemyHitGreen = { &sBgAnimationChunks[10], gBgAnimEnemyHitMap, gBgAnimEnemyHitGreenPalette, sizeof(gBgAnimEnemyHitGreenPalette), 36, 16, 16, 4, 4 };

BgAnimationDef gBgAnimDefEnemyHitRed = { &sBgAnimationChunks[10], gBgAnimEnemyHitMap, gBgAnimEnemyHitRedPalette, sizeof(gBgAnimEnemyHitRedPalette), 36, 16, 16, 4, 4 };

BgAnimationDef gBgAnimDefPotion = { &sBgAnimationChunks[11], gBgAnimPotionMap, gBgAnimPotionPalette, sizeof(gBgAnimPotionPalette), 46, 16, 16, 9, 5 };

BgAnimationDef gBgAnimDefBlizzard00 = { &sBgAnimationChunks[12], gBgAnimBlizzard00Map, gBgAnimBlizzard00Palette, sizeof(gBgAnimBlizzard00Palette), 66, 21, 16, 10, 3 };

BgAnimationDef gBgAnimDefFrost = { &sBgAnimationChunks[12], gBgAnimBlizzard00Map, gBgAnimBlizzard00Palette, sizeof(gBgAnimBlizzard00Palette), 66, 12, 16, 3, 4 };

BgAnimationDef gBgAnimDefBlizzard01 = { &sBgAnimationChunks[14], gBgAnimBlizzard01Map, gBgAnimBlizzard01Palette, sizeof(gBgAnimBlizzard01Palette), 37, 16, 16, 5, 7 };

static BgAnimationDef sBgAnimDefBlizzardHit = { &sBgAnimationChunks[14], gBgAnimBlizzard01Map, gBgAnimBlizzard01Palette, sizeof(gBgAnimBlizzard01Palette), 37, 16, 16, 5, 3 };

BgAnimationDef gBgAnimDefBlizzard02 = { &sBgAnimationChunks[15], gBgAnimBlizzard02Map, gBgAnimBlizzard02Palette, sizeof(gBgAnimBlizzard02Palette), 48, 16, 16, 6, 8 };

BgAnimationDef gBgAnimDefBlizzard03 = { &sBgAnimationChunks[16], gBgAnimBlizzard03Map, gBgAnimBlizzard03Palette, sizeof(gBgAnimBlizzard03Palette), 97, 16, 16, 9, 7 };

BgAnimationDef gBgAnimDefThunder00 = { &sBgAnimationChunks[18], gBgAnimThunder00Map, gBgAnimThunder00Palette, sizeof(gBgAnimThunder00Palette), 57, 16, 16, 8, 4 };

static BgAnimationDef sBgAnimDefThunderHit = { &sBgAnimationChunks[18], gBgAnimThunder00Map, gBgAnimThunder00Palette, sizeof(gBgAnimThunder00Palette), 57, 16, 16, 8, 2 };

BgAnimationDef gBgAnimDefThunder01 = { &sBgAnimationChunks[19], gBgAnimThunder01Map, gBgAnimThunder01Palette, sizeof(gBgAnimThunder01Palette), 36, 16, 21, 5, 5 };

BgAnimationDef gBgAnimDefThunder02 = { &sBgAnimationChunks[20], gBgAnimThunder02Map, gBgAnimThunder02Palette, sizeof(gBgAnimThunder02Palette), 111, 16, 19, 9, 5 };

BgAnimationDef gBgAnimDefThunder03 = { &sBgAnimationChunks[23], gBgAnimThunder03Map, gBgAnimThunder03Palette, sizeof(gBgAnimThunder03Palette), 117, 16, 19, 12, 6 };

BgAnimationDef gBgAnimDefCharaDefeatEnd = { &sBgAnimationChunks[26], gBgAnimCharaDefeatEndMap, gBgAnimCharaDefeatEndPalette, sizeof(gBgAnimCharaDefeatEndPalette), 21, 16, 16, 8, 8 };

BgAnimationDef gBgAnimDefEnemyDeath = { &sBgAnimationChunks[27], gBgAnimEnemyDeathMap, gBgAnimEnemyDeathPalette, sizeof(gBgAnimEnemyDeathPalette), 61, 16, 21, 5, 2 };

BgAnimationDef gBgAnimDefDarkDeath = { &sBgAnimationChunks[28], gBgAnimDarkDeathMap, gBgAnimDarkDeathPalette, sizeof(gBgAnimDarkDeathPalette), 64, 8, 8, 5, 3 };

BgAnimationDef gBgAnimDefCure02 = { &sBgAnimationChunks[29], gBgAnimCure02Map, gBgAnimCure02Palette, sizeof(gBgAnimCure02Palette), 79, 16, 16, 10, 6 };

BgAnimationDef gBgAnimDefCure01 = { &sBgAnimationChunks[31], gBgAnimCure01Map, gBgAnimCure01Palette, sizeof(gBgAnimCure01Palette), 80, 16, 16, 9, 6 };

BgAnimationDef gBgAnimDefFire03 = { &sBgAnimationChunks[33], gBgAnimFire03Map, gBgAnimFire03Palette, sizeof(gBgAnimFire03Palette), 68, 16, 16, 9, 6 };

BgAnimationDef gBgAnimDefRikuFire03 = { &sBgAnimationChunks[33], gBgAnimFire03Map, gBgAnimRikuFire03Palette, sizeof(gBgAnimRikuFire03Palette), 68, 16, 16, 9, 6 };

BgAnimationDef gBgAnimDefFriendHit = { &sBgAnimationChunks[35], gBgAnimFriendHitMap, gBgAnimFriendHitPalette, sizeof(gBgAnimFriendHitPalette), 61, 16, 16, 5, 3 };

BgAnimationDef gBgAnimDefStop00 = { &sBgAnimationChunks[36], gBgAnimStop00Map, gBgAnimStop00Palette, sizeof(gBgAnimStop00Palette), 33, 16, 16, 6, 5 };

BgAnimationDef gBgAnimDefStop01 = { &sBgAnimationChunks[37], gBgAnimStop01Map, gBgAnimStop01Palette, sizeof(gBgAnimStop01Palette), 52, 16, 16, 6, 5 };

BgAnimationDef gBgAnimDefStop02 = { &sBgAnimationChunks[38], gBgAnimStop02Map, gBgAnimStop02Palette, sizeof(gBgAnimStop02Palette), 90, 16, 16, 8, 5 };

BgAnimationDef gBgAnimDefSummon = { &sBgAnimationChunks[40], gBgAnimSummonMap, gBgAnimSummonPalette, sizeof(gBgAnimSummonPalette), 81, 16, 16, 8, 3 };

BgAnimationDef gBgAnimDefGroundImpact = { &sBgAnimationChunks[42], gBgAnimGroundImpactMap, gBgAnimGroundImpactPalette, sizeof(gBgAnimGroundImpactPalette), 72, 16, 16, 8, 3 };

BgAnimationDef gBgAnimDefBtlStart = { &sBgAnimationChunks[44], gBgAnimBtlStartMap, gBgAnimBtlStartPalette, sizeof(gBgAnimBtlStartPalette), 177, 16, 11, 11, 4 };

BgAnimationDef gBgAnimDefGuard = { &sBgAnimationChunks[50], gBgAnimGuardMap, gBgAnimGuardPalette, sizeof(gBgAnimGuardPalette), 41, 16, 16, 6, 3 };

BgAnimationDef gBgAnimDefCharaDefeat = { &sBgAnimationChunks[51], gBgAnimCharaDefeatMap, gBgAnimCharaDefeatPalette, sizeof(gBgAnimCharaDefeatPalette), 69, 16, 16, 11, 5 };

BgAnimationDef gBgAnimDefHumDefeat = { &sBgAnimationChunks[53], gBgAnimHumDefeatMap, gBgAnimHumDefeatPalette, sizeof(gBgAnimHumDefeatPalette), 85, 16, 16, 10, 5 };

BgAnimationDef gBgAnimDefGravity00 = { &sBgAnimationChunks[55], gBgAnimGravity00Map, gBgAnimGravity00Palette, sizeof(gBgAnimGravity00Palette), 41, 16, 16, 9, 2 };

BgAnimationDef gBgAnimDefGravity01 = { &sBgAnimationChunks[56], gBgAnimGravity01Map, gBgAnimGravity01Palette, sizeof(gBgAnimGravity01Palette), 85, 16, 22, 10, 6 };

BgAnimationDef gBgAnimDefPremireChance = { &sBgAnimationChunks[58], gBgAnimPremireChanceMap, gBgAnimPremireChancePalette, sizeof(gBgAnimPremireChancePalette), 96, 12, 14, 9, 5 };

BgAnimationDef gBgAnimDefGas = { &sBgAnimationChunks[60], gBgAnimGasMap, gBgAnimGasPalette, sizeof(gBgAnimGasPalette), 73, 16, 16, 4, 5 };

BgAnimationDef gBgAnimDefEnemySpawn = { &sBgAnimationChunks[61], gBgAnimEnemySpawnMap, gBgAnimEnemySpawnPalette, sizeof(gBgAnimEnemySpawnPalette), 46, 16, 16, 4, 2 };

BgAnimationDef gBgAnimDefBoogieKaihuku = { &sBgAnimationChunks[62], gBgAnimBoogieKaihukuMap, gBgAnimBoogieKaihukuPalette, sizeof(gBgAnimBoogieKaihukuPalette), 45, 16, 9, 3, 6 };

BgAnimationDef gBgAnimDefPcShot = { &sBgAnimationChunks[63], gBgAnimPcShotMap, gBgAnimPcShotPalette, sizeof(gBgAnimPcShotPalette), 48, 16, 16, 3, 8 };

BgAnimationDef gBgAnimDefGlow = { &sBgAnimationChunks[64], gBgAnimGlowMap, gBgAnimGlowPalette, sizeof(gBgAnimGlowPalette), 122, 16, 16, 1, 65535 };

static BgAnimationDef sBgAnimDefDarkGlow = { &sBgAnimationChunks[64], gBgAnimGlowMap, gBgAnimDarkGlowPalette, sizeof(gBgAnimDarkGlowPalette), 122, 16, 16, 1, 65535 };

BgAnimationDef gBgAnimDefBossDeath = { &sBgAnimationChunks[65], gBgAnimBossDeathMap, gBgAnimBossDeathPalette, sizeof(gBgAnimBossDeathPalette), 67, 16, 16, 8, 5 };

BgAnimationDef gBgAnimDefDumboSplash = { &sBgAnimationChunks[67], gBgAnimDumboSplashMap, gBgAnimDumboSplashPalette, sizeof(gBgAnimDumboSplashPalette), 77, 26, 16, 4, 4 };

BgAnimationDef gBgAnimDefTrinityLimit = { &sBgAnimationChunks[68], gBgAnimTrinityLimitMap, gBgAnimTrinityLimitPalette, sizeof(gBgAnimTrinityLimitPalette), 105, 16, 15, 1, 65535 };

BgAnimationDef gBgAnimDefTrinityLimitCharge = { &sBgAnimationChunks[69], gBgAnimTrinityLimitChargeMap, gBgAnimTrinityLimitChargePalette, sizeof(gBgAnimTrinityLimitChargePalette), 122, 16, 22, 8, 4 };

BgAnimationDef gBgAnimDefTrinityLimitBlast = { &sBgAnimationChunks[71], gBgAnimTrinityLimitBlastMap, gBgAnimTrinityLimitBlastPalette, sizeof(gBgAnimTrinityLimitBlastPalette), 113, 15, 15, 1, 65535 };

BgAnimationDef gBgAnimDefRagnarokCharge = { &sBgAnimationChunks[72], gBgAnimRagnarokChargeMap, gBgAnimRagnarokChargePalette, sizeof(gBgAnimRagnarokChargePalette), 64, 10, 16, 3, 8 };

static BgAnimationDef sBgAnimDefRagnarokChargePurple = { &sBgAnimationChunks[72], gBgAnimRagnarokChargeMap, gBgAnimRagnarokChargePurplePalette, sizeof(gBgAnimRagnarokChargePurplePalette), 86, 10, 16, 6, 3 };

BgAnimationDef gBgAnimDefRagnarokShot = { &sBgAnimationChunks[73], gBgAnimRagnarokShotMap, gBgAnimRagnarokShotPalette, sizeof(gBgAnimRagnarokShotPalette), 95, 10, 16, 7, 3 };

BgAnimationDef gBgAnimDefUrsulaBeam = { &sBgAnimationChunks[75], gBgAnimUrsulaBeamMap, gBgAnimUrsulaBeamPalette, sizeof(gBgAnimUrsulaBeamPalette), 122, 24, 8, 4, 8 };

BgAnimationDef gBgAnimDefJfMajinBeam = { &sBgAnimationChunks[76], gBgAnimJfMajinBeamMap, gBgAnimJfMajinBeamPalette, sizeof(gBgAnimJfMajinBeamPalette), 82, 16, 27, 3, 8 };

BgAnimationDef gBgAnimDefAnsemRush = { &sBgAnimationChunks[77], gBgAnimAnsemRushMap, gBgAnimAnsemRushPalette, sizeof(gBgAnimAnsemRushPalette), 74, 17, 16, 1, 65535 };

BgAnimationDef gBgAnimDefAnsemWave = { &sBgAnimationChunks[78], gBgAnimAnsemWaveMap, gBgAnimAnsemWavePalette, sizeof(gBgAnimAnsemWavePalette), 75, 16, 26, 3, 3 };

BgAnimationDef gBgAnimDefStunImpact = { &sBgAnimationChunks[79], gBgAnimStunImpactMap, gBgAnimStunImpactPalette, sizeof(gBgAnimStunImpactPalette), 74, 16, 18, 4, 5 };

BgAnimationDef gBgAnimDefUrsulaThunder = { &sBgAnimationChunks[80], gBgAnimUrsulaThunderMap, gBgAnimUrsulaThunderPalette, sizeof(gBgAnimUrsulaThunderPalette), 45, 16, 16, 4, 5 };

BgAnimationDef gBgAnimDefWorldSelect = { &sBgAnimationChunks[81], gBgAnimWorldSelectMap, gBgAnimWorldSelectPalette, sizeof(gBgAnimWorldSelectPalette), 64, 20, 7, 8, 6 };

BgAnimationDef gBgAnimDefWorldStart = { &sBgAnimationChunks[82], gBgAnimWorldStartMap, gBgAnimWorldStartPalette, sizeof(gBgAnimWorldStartPalette), 155, 16, 16, 6, 4 };

BgAnimationDef gBgAnimDefXmas = { &sBgAnimationChunks[84], gBgAnimXmasMap, gBgAnimXmasPalette, sizeof(gBgAnimXmasPalette), 64, 32, 32, 10, 8 };

BgAnimationDef gBgAnimDefVixenIceFall = { &sBgAnimationChunks[84], gBgAnimXmasMap, gBgAnimXmasPalette, sizeof(gBgAnimXmasPalette), 64, 32, 32, 10, 2 };

BgAnimationDef gBgAnimDefLightPillar = { &sBgAnimationChunks[86], gBgAnimLightPillarMap, gBgAnimLightPillarPalette, sizeof(gBgAnimLightPillarPalette), 115, 16, 27, 1, 65535 };

BgAnimationDef gBgAnimDefTornado = { &sBgAnimationChunks[87], gBgAnimTornadoMap, gBgAnimTornadoPalette, sizeof(gBgAnimTornadoPalette), 79, 16, 23, 3, 6 };

BgAnimationDef gBgAnimDefLimitGreen = { &sBgAnimationChunks[8], gBgAnimLimitMap, gBgAnimLimitGreenPalette, sizeof(gBgAnimLimitGreenPalette), 42, 16, 16, 6, 3 };

BgAnimationDef gBgAnimDefDashRingPurple = { &sBgAnimationChunks[9], gBgAnimDashRingMap, gBgAnimDashRingPurplePalette, sizeof(gBgAnimDashRingPurplePalette), 74, 16, 16, 5, 3 };

BgAnimationDef gBgAnimDefAxcelFireWall = { &sBgAnimationChunks[88], gBgAnimAxcelFireWallMap, gBgAnimAxcelFireWallPalette, sizeof(gBgAnimAxcelFireWallPalette), 144, 20, 30, 4, 6 };

BgAnimationDef gBgAnimDefMahluxiaGround = { &sBgAnimationChunks[90], gBgAnimMahluxiaGroundMap, gBgAnimMahluxiaGroundPalette, sizeof(gBgAnimMahluxiaGroundPalette), 74, 16, 16, 8, 6 };

static BgAnimationDef sBgAnimDefLexceusGround = { &sBgAnimationChunks[90], gBgAnimMahluxiaGroundMap, gBgAnimLexceusGroundPalette, sizeof(gBgAnimLexceusGroundPalette), 74, 16, 16, 8, 4 };

static BgAnimationDef sBgAnimDefRikuLimit = { &sBgAnimationChunks[90], gBgAnimMahluxiaGroundMap, gBgAnimMahluxiaGroundPalette, sizeof(gBgAnimMahluxiaGroundPalette), 74, 16, 16, 8, 3 };

BgAnimationDef gBgAnimDefHanabira = { &sBgAnimationChunks[92], gBgAnimHanabiraMap, gBgAnimHanabiraPalette, sizeof(gBgAnimHanabiraPalette), 64, 32, 32, 1, 65535 };

BgAnimationDef gBgAnimDefKama = { &sBgAnimationChunks[93], gBgAnimKamaMap, gBgAnimKamaPalette, sizeof(gBgAnimKamaPalette), 67, 16, 16, 1, 65535 };

BgAnimationDef gBgAnimDefDragonFire = { &sBgAnimationChunks[94], gBgAnimDragonFireMap, gBgAnimDragonFirePalette, sizeof(gBgAnimDragonFirePalette), 96, 13, 24, 5, 6 };

BgAnimationDef gBgAnimDefLaxeneBeam = { &sBgAnimationChunks[96], gBgAnimLaxeneBeamMap, gBgAnimLaxeneBeamPalette, sizeof(gBgAnimLaxeneBeamPalette), 117, 8, 16, 6, 5 };

BgAnimationDef gBgAnimDefAero = { &sBgAnimationChunks[98], gBgAnimAeroMap, gBgAnimAeroPalette, sizeof(gBgAnimAeroPalette), 81, 16, 22, 6, 6 };

BgAnimationDef gBgAnimDefRikuHit = { &sBgAnimationChunks[99], gBgAnimRikuHitMap, gBgAnimRikuHitPalette, sizeof(gBgAnimRikuHitPalette), 48, 16, 16, 4, 4 };

BgAnimationDef gBgAnimDefRikuDarkModeFlash = { &sBgAnimationChunks[100], gBgAnimRikuDarkModeFlashMap, gBgAnimRikuDarkModeFlashPalette, sizeof(gBgAnimRikuDarkModeFlashPalette), 74, 16, 21, 7, 5 };

BgAnimationDef gBgAnimDefLstCtr = { &sBgAnimationChunks[102], gBgAnimLstCtrMap, gBgAnimLstCtrPalette, sizeof(gBgAnimLstCtrPalette), 108, 16, 16, 12, 7 };

BgAnimationDef gBgAnimDefDsdTransition = { &sBgAnimationChunks[105], gBgAnimDsdTransitionMap, gBgAnimDsdTransitionPalette, sizeof(gBgAnimDsdTransitionPalette), 64, 16, 16, 8, 6 };

BgAnimationDef gBgAnimDefRikuLimitFinish = { &sBgAnimationChunks[106], gBgAnimRikuLimitFinishMap, gBgAnimRikuLimitFinishPalette, sizeof(gBgAnimRikuLimitFinishPalette), 64, 30, 30, 6, 7 };

BgAnimationDef gBgAnimDefRikuDiveHit = { &sBgAnimationChunks[107], gBgAnimRikuDiveHitMap, gBgAnimRikuDiveHitPalette, sizeof(gBgAnimRikuDiveHitPalette), 64, 16, 16, 5, 5 };

static BgFx* sBgFx;
#ifdef PLATFORM_PC
/* The background effect outlives the action that created it. Preserve its
 * independent damage owner without changing the cartridge's BgFx layout. */
static BtlObj* sBgFxAttackSource;
/* RA conditions anchor their heap-relative reads at this original global,
 * not gBtlWork. Keep access read-only; no battle/effect lifetime changes. */
const void* PortRaBgFxWork(void) { return sBgFx; }
#endif


void BgFxReset() {
#ifdef PLATFORM_PC
    sBgFxAttackSource = BtlGetAttackSource();
#endif
    if (gBtlWork->flags & BTL_FLAG_BOSS_BATTLE) {
        SetBgPriority(sBgFx->bg, 0);
    } else {
        SetBgPriority(sBgFx->bg, 1);
    }

    SetBgBlend(sBgFx->bg, 16, 16);
    sBgFx->scaleX = Q_8_8(1);
    sBgFx->scaleY = Q_8_8(1);
    sBgFx->angle = 0;
    sBgFx->update = NULL;
    sBgFx->timer = 0;
    sBgFx->z = 0;
    sBgFx->flags = BGFX_FLAG_ACTIVE;
    sBgFx->releaseFrames = 0;
}

u8 BgFxIsBlocked(u8 priority) {
    if (BgAnimIsStopped()) {
        sBgFx->priority = priority;
        return FALSE;
    }

    if (sBgFx->priority < priority) {
        return TRUE;
    }

    sBgFx->priority = priority;
    return FALSE;
}

void BgFxReleaseEarly(s16 frames) {
    u16 frame;
    u16 frameTimer;
    BgAnimGetFrameState(&frame, &frameTimer);

    if (BgAnimGetDuration(BgAnimGetCurrent()) - frame * frameTimer <= frames) {
        sBgFx->flags &= ~BGFX_FLAG_ACTIVE;
        sBgFx->priority = 2;

        if (sBgFx->flags & BGFX_FLAG_SCREEN_DIMMED) {
            sBgFx->flags &= ~BGFX_FLAG_SCREEN_DIMMED;
            FadeToOriginal(FADE_MODE_BLACK, 8);
        }
    }
}

void BgFxInit(u16 colorMode, u16 bg) {
    s32 i;
    sBgFx = EwramAlloc(sizeof(BgFx));

    for (i = 10; i < 16; i++) {
        FadeSetPaletteExcluded(i, TRUE);
    }

    if (colorMode == BGCNT_16COLOR) {
        BgAnimInit(bg, BGCNT_TXT512x512, BGCNT_16COLOR);
    } else {
        BgAnimInit(bg, BGCNT_AFF512x512, BGCNT_256COLOR);
    }

    SetBgBlend(bg, 16, 16);
    sBgFx->update = NULL;
    sBgFx->priority = 0xFF;
    sBgFx->bg = bg;
    BgFxReset();
    sBgFx->flags = 0;
}

void BgFxFree() {
#ifdef PLATFORM_PC
    sBgFxAttackSource = NULL;
#endif
    EwramFree(sBgFx);
}

void BgFxUpdate() {
    if (gBtlWork->flags & BTL_FLAG_STOP_BGFX) {
        gBtlWork->flags &= ~BTL_FLAG_STOP_BGFX;
        sBgFx->flags &= ~BGFX_FLAG_ACTIVE;
        sBgFx->update = NULL;
        BgAnimStop();
        SetBgBlend(sBgFx->bg, 16, 16);
    }

    if (sBgFx->update != NULL) {
#ifdef PLATFORM_PC
        BtlObj* previousSource = BtlSetAttackSource(sBgFxAttackSource);
#endif
        sBgFx->update();
#ifdef PLATFORM_PC
        BtlSetAttackSource(previousSource);
#endif

        if (!(gBtlWork->flags & BTL_FLAG_BOSS_BATTLE)) {
            gBtlWork->bossY = sBgFx->y;

            if (sBgFx->flags & BGFX_FLAG_ABOVE_SPRITES) {
                SetBgPriority(sBgFx->bg, 0);
            } else if (sBgFx->flags & BGFX_FLAG_BELOW_SPRITES) {
                SetBgPriority(sBgFx->bg, 1);
                gBtlWork->bossPriorityOffset = 0xFF00;
            } else {
                gBtlWork->bossPriorityOffset = 8;
            }
        }
    }

    BgAnimUpdate();
}

u8 BgFxIsActive() {
    if (sBgFx->flags & BGFX_FLAG_ACTIVE) {
        return TRUE;
    }

    return FALSE;
}

void BgFxUpdateBase() {
    s16 sx;
    s16 sy;

    if (BgAnimIsStopped()) {
        SetBgBlend(sBgFx->bg, 16, 16);
        sBgFx->update = NULL;
        sBgFx->flags &= ~BGFX_FLAG_ACTIVE;

        if (sBgFx->flags & BGFX_FLAG_SCREEN_DIMMED) {
            FadeToOriginal(FADE_MODE_BLACK, 8);
        }

        return;
    }

    if (gBtlWork->flags & BTL_FLAG_VS_BATTLE) {
        if (gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION) {
            if (gBtlWork->flags & BTL_FLAG_STOCK_SEQUENCE) {
                if (gBtlWork->stockMove < GetStockMoveCount()) {
                    BgFxReleaseEarly(sBgFx->releaseFrames);
                }
            }
        } else if (gRikuBtlWork->flags & BTL_FLAG_STOCK_SEQUENCE) {
            if (gRikuBtlWork->stockMove < GetStockMoveCount()) {
                BgFxReleaseEarly(sBgFx->releaseFrames);
            }
        }
    } else if (gBtlWork->flags & BTL_FLAG_STOCK_SEQUENCE) {
        if (gBtlWork->stockMove < GetStockMoveCount()) {
            BgFxReleaseEarly(sBgFx->releaseFrames);
        }
    }

    WorldToScreen(&sx, &sy, sBgFx->x, sBgFx->y, sBgFx->z);
    BgAnimSetPosition(sx, sy);

    if (sBgFx->flags & BGFX_FLAG_IGNORE_ZOOM) {
        BgAnimSetTransform(sBgFx->angle + gBtlWork->rotation, sBgFx->scaleX, sBgFx->scaleY);
    } else {
        s32 scaleX = sBgFx->scaleX * gBtlWork->scale >> 8;
        s32 scaleY = sBgFx->scaleY * gBtlWork->scale >> 8;

        BgAnimSetTransform(sBgFx->angle + gBtlWork->rotation, scaleX, scaleY);
    }
}

void BgFxStartCure(u16 tier, s32 x, s32 y, s32 z) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->releaseFrames = 20;
    sBgFx->x = x;
    sBgFx->y = y;

    switch (tier) {
    case SPELL_TIER_BASE:
        sBgFx->z = z - 0x1400;
        break;
    case SPELL_TIER_RA:
        sBgFx->z = z;
        break;
    case SPELL_TIER_GA:
        sBgFx->z = z - 0x1000;
        break;
    }

    WorldToScreen(&sx, &sy, sBgFx->x, sBgFx->y, sBgFx->z);

    switch (tier) {
    case SPELL_TIER_BASE:
        BgAnimStart(&gBgAnimDefCure00, sx, sy);
        m4aSongNumStart(SONG_EF_CAREL00);
        break;
    case SPELL_TIER_RA:
        BgAnimStart(&gBgAnimDefCure01, sx, sy);
        m4aSongNumStart(SONG_EF_CAREL01);
        break;
    case SPELL_TIER_GA:
        BgAnimStart(&gBgAnimDefCure02, sx, sy);
        m4aSongNumStart(SONG_EF_CAREL02);
        break;
    }

    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
    sBgFx->update = BgFxUpdateBase;
}

void BgFxUpdateFadeOut() {
    SetBlendAlpha(16, 16 - sBgFx->timer);

    if (sBgFx->timer > 15) {
        BgAnimStop();
    } else {
        sBgFx->timer++;
    }

    BgFxUpdateBase();
}

void BgFxUpdateFire() {
    u16 frame;
    u16 frameTimer;
    u16 angle;
    s16 sx;
    s16 sy;
    s32 dx;

    BgAnimGetFrameState(&frame, &frameTimer);
    dx = 0;

    if (sBgFx->timer > 0) {
        if (frame > 7) {
            angle = sBgFx->angle;

            if (sBgFx->flags & BGFX_FLAG_FLIP_X) {
                ApproachAngle(&angle, GetAngle(sBgFx->x, sBgFx->y,
                                               sBgFx->targetX, sBgFx->targetY) + 64, 5);
            } else {
                ApproachAngle(&angle, GetAngle(sBgFx->x, sBgFx->y,
                                               sBgFx->targetX, sBgFx->targetY) - 64, 5);
            }

            sBgFx->angle = angle;
            ApproachValue(&sBgFx->x, sBgFx->targetX, sBgFx->timer);
            ApproachValue(&sBgFx->y, sBgFx->targetY, sBgFx->timer);
            ApproachValue(&sBgFx->z, sBgFx->targetZ, sBgFx->timer);

            if (ApplyAttackBox(sBgFx->attack, sBgFx->x, sBgFx->y,
                              sBgFx->z, 8, 8, 16)) {
                sBgFx->timer = -1;
            } else {
                sBgFx->timer--;
            }
        } else if (frame > 2) {
            if (sBgFx->flags & BGFX_FLAG_FLIP_X) {
                dx = ((7 - frame) << 8) * 7;
            } else {
                s32 framesLeft = 7 - frame;
                framesLeft *= 256;
                dx = framesLeft * -7;
            }

            if (ApplyAttackBox(sBgFx->attack, sBgFx->x + dx,
                              sBgFx->y, sBgFx->z, 8, 8, 16)) {
                sBgFx->timer = -1;
            }
        }
    }

    if (sBgFx->timer == 0) {
        sBgFx->update = BgFxUpdateFadeOut;
    } else if (sBgFx->timer == -1) {
        sBgFx->angle = 0;
        sBgFx->scaleX = Q_8_8(1);
        sBgFx->scaleY = Q_8_8(1);
        sBgFx->x += dx;
        sBgFx->releaseFrames = 20;
        WorldToScreen(&sx, &sy, sBgFx->x, sBgFx->y,
                      sBgFx->z);

        switch (sBgFx->state) {
        case SPELL_TIER_BASE:
            BgAnimStart(&gBgAnimDefFire01, sx, sy);
            m4aSongNumStart(SONG_EF_FIRE01);
            break;
        case SPELL_TIER_RA:
            BgAnimStart(&gBgAnimDefFire02, sx, sy);
            m4aSongNumStart(SONG_EF_FIRE02);
            break;
        case SPELL_TIER_GA:
            BgAnimStart(&gBgAnimDefFire03, sx, sy);
            m4aSongNumStart(SONG_EF_FIRE03);
            break;
        case SPELL_TIER_DARK:
        default:
            BgAnimStart(&gBgAnimDefRikuFire03, sx, sy);
            m4aSongNumStart(SONG_EF_FIRE03);
            break;
        }

        sBgFx->timer = -2;
    }

    BgFxUpdateBase();
}

void BgFxStartFire(u16 tier, s32 x, s32 y, s32 z, s32 targetX, s32 targetY, s32 targetZ, u8 flip, s32 attack) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    WorldToScreen(&sx, &sy, x, y, z);

    if (tier == SPELL_TIER_DARK) {
        BgAnimStart(&gBgAnimDefRikuFire00, sx, sy);
    } else {
        BgAnimStart(&gBgAnimDefFire00, sx, sy);
    }

    if ((gBtlWork->flags & BTL_FLAG_VS_BATTLE) == 0 && (gBtlWork->flags & BTL_FLAG_OPPONENT_CARD_ACTION)) {
        m4aSongNumStart(SONG_EF_MON_FIRE);
    } else {
        m4aSongNumStart(SONG_EF_FIRE00);
    }

    BgAnimSetLoopStartFrame(8);
    sBgFx->update = BgFxUpdateFire;
    sBgFx->targetX = targetX;
    sBgFx->targetY = targetY;
    sBgFx->targetZ = targetZ;
    sBgFx->timer = 15;
    sBgFx->state = tier;
    sBgFx->attack = attack;

    if (flip) {
        sBgFx->flags |= BGFX_FLAG_FLIP_X;
        sBgFx->scaleX = Q_8_8(-1);
    }

    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

void BgFxStartFireAtPlayer(s32 x, s32 y, s32 z, u8 flip, s32 unused, s32 attack, u16 timer) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefFire00, sx, sy);
    m4aSongNumStart(SONG_EF_MON_FIRE);
    BgAnimSetLoopStartFrame(8);
    sBgFx->update = BgFxUpdateFire;
    sBgFx->targetX = gBtlWork->actor->x;
    sBgFx->targetY = gBtlWork->actor->y;
    sBgFx->targetZ = gBtlWork->actor->z;
    sBgFx->timer = timer;
    sBgFx->state = SPELL_TIER_RA;
    sBgFx->attack = attack;

    if (flip) {
        sBgFx->flags |= BGFX_FLAG_FLIP_X;
        sBgFx->scaleX = Q_8_8(-1.5);
    } else {
        sBgFx->scaleX = Q_8_8(1.5);
    }

    sBgFx->scaleY = Q_8_8(1.5);
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

void BgFxUpdateBlizzard() {
    u16 frame;
    u16 frameTimer;
    u16 angle;
    s16 sx;
    s16 sy;
    s32 dx;

    BgAnimGetFrameState(&frame, &frameTimer);
    dx = 0;

    if (sBgFx->timer > 0) {
        if (frame > 7) {
            angle = sBgFx->angle;

            if (sBgFx->flags & BGFX_FLAG_FLIP_X) {
                ApproachAngle(&angle,
                    GetAngle(sBgFx->x, sBgFx->y, sBgFx->targetX,
                        sBgFx->targetY) + 64,
                    5);
            } else {
                ApproachAngle(&angle,
                    GetAngle(sBgFx->x, sBgFx->y, sBgFx->targetX,
                        sBgFx->targetY) - 64,
                    5);
            }

            sBgFx->angle = angle;
            ApproachValue(&sBgFx->x, sBgFx->targetX, sBgFx->timer);
            ApproachValue(&sBgFx->y, sBgFx->targetY, sBgFx->timer);
            ApproachValue(&sBgFx->z, sBgFx->targetZ, sBgFx->timer);

            if (TestAttackBox(sBgFx->x, sBgFx->y, sBgFx->z, 8, 16, 16)) {
                sBgFx->timer = -1;
            } else {
                sBgFx->timer--;
            }
        } else if (frame > 2) {
            if (sBgFx->flags & BGFX_FLAG_FLIP_X) {
                dx = ((7 - frame) << 8) * 7;
            } else {
                s32 framesLeft = 7 - frame;
                framesLeft *= 256;
                dx = framesLeft * -7;
            }

            if (TestAttackBox(sBgFx->x + dx, sBgFx->y, sBgFx->z, 8, 16, 16)) {
                sBgFx->timer = -1;
            }
        }
    }

    if (sBgFx->timer == 0) {
        sBgFx->timer = -1;
    } else if (sBgFx->timer == -1) {
        sBgFx->angle = 0;
        sBgFx->scaleX = Q_8_8(1);
        sBgFx->scaleY = Q_8_8(1);
        sBgFx->x += dx;
        sBgFx->releaseFrames = 20;
        WorldToScreen(&sx, &sy, sBgFx->x, sBgFx->y, sBgFx->z);

        switch (sBgFx->state) {
        case SPELL_TIER_BASE:
            BgAnimStart(&gBgAnimDefBlizzard01, sx, sy);
            m4aSongNumStart(SONG_EF_BURIZA01);
            break;
        case SPELL_TIER_RA:
            BgAnimStart(&gBgAnimDefBlizzard02, sx, sy);
            m4aSongNumStart(SONG_EF_BURIZA02);
            break;
        case SPELL_TIER_GA:
        default:
            BgAnimStart(&gBgAnimDefBlizzard03, sx, sy);
            m4aSongNumStart(SONG_EF_BURIZA03);
            break;
        }

        sBgFx->timer = -2;
    }

    if (sBgFx->timer == -2) {
        switch (sBgFx->state) {
        case SPELL_TIER_BASE:
            if (sBgFx->steps == 20) {
                ApplyAttackBox(sBgFx->attack, sBgFx->x, sBgFx->y,
                    sBgFx->z, 18, 18, 18);
            }

            break;
        case SPELL_TIER_RA:
            if (sBgFx->steps == 35) {
                ApplyAttackBox(sBgFx->attack, sBgFx->x, sBgFx->y,
                    sBgFx->z, 24, 24, 30);
            }

            break;
        case SPELL_TIER_GA:
        default:
            if (sBgFx->steps == 50) {
                ApplyAttackBox(sBgFx->attack, sBgFx->x, sBgFx->y,
                    sBgFx->z, 48, 48, 52);
            }

            break;
        }

        sBgFx->steps++;
    }

    BgFxUpdateBase();
}

void BgFxStartBlizzard(u16 tier, s32 x, s32 y, s32 z, s32 targetX, s32 targetY, s32 targetZ, u8 flip, s32 attack) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefBlizzard00, sx, sy);

    if ((gBtlWork->flags & BTL_FLAG_VS_BATTLE) == 0 && (gBtlWork->flags & BTL_FLAG_OPPONENT_CARD_ACTION)) {
        m4aSongNumStart(SONG_EF_MON_BURIZA);
    } else {
        m4aSongNumStart(SONG_EF_BURIZA00);
    }

    BgAnimSetLoopStartFrame(8);
    sBgFx->update = BgFxUpdateBlizzard;
    sBgFx->targetX = targetX;
    sBgFx->targetY = targetY;
    sBgFx->targetZ = targetZ;
    sBgFx->timer = 15;
    sBgFx->state = tier;
    sBgFx->attack = attack;
    sBgFx->steps = 0;

    if (flip) {
        sBgFx->scaleX = Q_8_8(-1);
        sBgFx->flags |= BGFX_FLAG_FLIP_X;
    }

    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

void BgFxUpdateFlash() {
    sBgFx->scaleX += sBgFx->targetX;
    sBgFx->scaleY += sBgFx->targetY;
    sBgFx->angle += 3;
    BgFxUpdateBase();

    if (sBgFx->timer > 5) {
        s16 fade = sBgFx->timer - 5;
        SetBlendAlpha(16, 16 - fade);

        if (fade > 15) {
            BgAnimStop();
        }
    }

    sBgFx->timer++;
}

void BgFxStartFlash(s32 x, s32 y, s32 z) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(3)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z - 0x1000;
    WorldToScreen(&sx, &sy, x, y, z - 0x1000);
    sBgFx->scaleX = Q_8_8(0.3);
    sBgFx->scaleY = Q_8_8(0.3);
    sBgFx->angle = 0;
    sBgFx->targetX = Q_8_8(0.1);
    sBgFx->targetY = Q_8_8(0.1);
    BgAnimStart(&gBgAnimDefFlash, sx, sy);
    BgAnimSetLoopStartFrame(0);
    sBgFx->update = BgFxUpdateFlash;
    sBgFx->timer = 0;
}

void BgFxUpdateFlashHit() {
    s16 timer = sBgFx->timer;

    sBgFx->angle += 4;
    sBgFx->scaleX += sBgFx->targetX;
    sBgFx->scaleY += sBgFx->targetY;
    SetBlendAlpha(16, 16 - timer);

    if (timer > 15) {
        BgAnimStop();
    }

    sBgFx->timer++;
    BgFxUpdateBase();
}

void BgFxStartFlashHit(s32 x, s32 y, s32 z) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(1)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z - 0x1000;
    WorldToScreen(&sx, &sy, x, y, z - 0x1000);
    sBgFx->scaleX = Q_8_8(1);
    sBgFx->scaleY = Q_8_8(1);
    sBgFx->angle = 0;
    sBgFx->targetX = Q_8_8(0.3);
    sBgFx->targetY = Q_8_8(0.3);
    BgAnimStart(&gBgAnimDefFlash, sx, sy);
    BgAnimSetLoopStartFrame(0);
    sBgFx->update = BgFxUpdateFlashHit;
    sBgFx->timer = 0;
}

void func_080135EC(s32 x, s32 y, s32 z) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z - 0x1000;
    WorldToScreen(&sx, &sy, x, y, z - 0x1000);
    sBgFx->scaleX = Q_8_8(0.5);
    sBgFx->scaleY = Q_8_8(0.5);
    sBgFx->angle = 0;
    BgAnimStart(&gBgAnimDefFlash, sx, sy);
    BgAnimSetLoopStartFrame(0);
    sBgFx->update = BgFxUpdateFlash;
    sBgFx->timer = 0;
}

void BgFxStartSoraHit(s32 x, s32 y, s32 z) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(3)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z - 0x1000;
    WorldToScreen(&sx, &sy, x, y, z - 0x1000);
    BgAnimStart(&gBgAnimDefSoraHit, sx, sy);
    sBgFx->update = BgFxUpdateBase;
}

void BgFxStartRikuHit(s32 x, s32 y, s32 z) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(3)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z - 0x1000;
    WorldToScreen(&sx, &sy, x, y, z - 0x1000);
    BgAnimStart(&gBgAnimDefRikuHit, sx, sy);
    sBgFx->update = BgFxUpdateBase;
}

void BgFxStartLimit(s32 x, s32 y, s32 z, u8 flip) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(1)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;

    if (flip) {
        sBgFx->scaleX = Q_8_8(-1);
    }

    WorldToScreen(&sx, &sy, sBgFx->x, sBgFx->y, sBgFx->z);
    BgAnimStart(&gBgAnimDefLimit, sx, sy);
    sBgFx->update = BgFxUpdateBase;
}

void BgFxStartDashRing(s32 x, s32 y, s32 z, u8 flip) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(2)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x + 0x400;
    sBgFx->y = y;
    sBgFx->z = z - 0x1000;

    if (flip) {
        sBgFx->scaleX = Q_8_8(-1);
    }

    WorldToScreen(&sx, &sy, sBgFx->x, sBgFx->y, sBgFx->z);
    BgAnimStart(&gBgAnimDefDashRing, sx, sy);
    sBgFx->update = BgFxUpdateBase;
}

void BgFxStartEnemyHit(s32 x, s32 y, s32 z) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(2)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z - 0x1000;
    WorldToScreen(&sx, &sy, x, y, z - 0x1000);
    BgAnimStart(&gBgAnimDefEnemyHit, sx, sy);
    sBgFx->update = BgFxUpdateBase;
}

void BgFxStartFireHit(s32 x, s32 y, s32 z) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(2)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z - 0x1000;
    sBgFx->scaleY = Q_8_8(2);
    WorldToScreen(&sx, &sy, x, y, z - 0x1000);
    BgAnimStart(&sBgAnimDefFireHit, sx, sy);
    sBgFx->update = BgFxUpdateBase;
}

void BgFxStartBlizzardHit(s32 x, s32 y, s32 z) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(2)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z - 0x1000;
    WorldToScreen(&sx, &sy, x, y, z - 0x1000);
    BgAnimStart(&sBgAnimDefBlizzardHit, sx, sy);
    sBgFx->update = BgFxUpdateBase;
}

void BgFxStartThunderHit(s32 x, s32 y, s32 z) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(2)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z - 0x1000;
    WorldToScreen(&sx, &sy, x, y, z - 0x1000);
    BgAnimStart(&sBgAnimDefThunderHit, sx, sy);
    sBgFx->update = BgFxUpdateBase;
}

void BgFxStartGuard(s32 x, s32 y, s32 z) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(2)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefGuard, sx, sy);
    sBgFx->update = BgFxUpdateBase;
}

void BgFxStartPotion(s32 x, s32 y, s32 z) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->releaseFrames = 20;
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z - 0x3000;
    WorldToScreen(&sx, &sy, x, y, z - 0x3000);
    BgAnimStart(&gBgAnimDefPotion, sx, sy);
    m4aSongNumStart(SONG_EF_POSION);
    sBgFx->update = BgFxUpdateBase;
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

enum BgFxWideThunderPhase {
    BGFX_WIDE_THUNDER_PHASE_CAST,
    BGFX_WIDE_THUNDER_PHASE_STRIKE
};

void BgFxUpdateWideThunder() {
    s16 sx;
    s16 sy;
    s16 sx2;
    s16 sy2;

    if (sBgFx->timer == BGFX_WIDE_THUNDER_PHASE_CAST && BgAnimIsStopped()) {
        SetBlendAlpha(16, 16);
        sBgFx->x = gBtlWork->viewX;
        sBgFx->y = (gBtlWork->yMin + gBtlWork->yMax) << 7;
        sBgFx->z = sBgFx->targetZ;
        WorldToScreen(&sx, &sy, sBgFx->x, sBgFx->y, sBgFx->z);
        sBgFx->angle = 0;
        sBgFx->releaseFrames = 20;
        sBgFx->flags |= BGFX_FLAG_IGNORE_ZOOM;
        sBgFx->scaleX = Q_8_8(2);
        sBgFx->scaleY = (sy << 8) / 40;

        if (sBgFx->scaleY < Q_8_8(1.5)) {
            sBgFx->scaleY = Q_8_8(1.5);
        }

        switch (sBgFx->state) {
        case SPELL_TIER_BASE:
            ApplyAttackBox(sBgFx->attack, sBgFx->x, sBgFx->y, sBgFx->z, 32, 256, 256);
            BgAnimStart(&gBgAnimDefThunder01, sx, sy);
            m4aSongNumStart(SONG_EF_THUND01);
            break;
        case SPELL_TIER_RA:
            ApplyAttackBox(sBgFx->attack, sBgFx->x, sBgFx->y, sBgFx->z, 256, 256, 256);
            BgAnimStart(&gBgAnimDefThunder02, sx, sy);
            m4aSongNumStart(SONG_EF_THUND02);
            break;
        case SPELL_TIER_GA:
            ApplyAttackBox(sBgFx->attack, sBgFx->x, sBgFx->y, sBgFx->z, 256, 256, 256);
            BgAnimStart(&gBgAnimDefThunder03, sx, sy);
            m4aSongNumStart(SONG_EF_THUND03);
            break;
        }

        sBgFx->timer = BGFX_WIDE_THUNDER_PHASE_STRIKE;
    } else if (sBgFx->timer == BGFX_WIDE_THUNDER_PHASE_STRIKE) {
        s32 height;

        WorldToScreen(&sx2, &sy2, sBgFx->x, sBgFx->y, sBgFx->z);
        height = sy2 << 8;
        sBgFx->scaleY = height / 40;

        if (sBgFx->scaleY < Q_8_8(1.5)) {
            sBgFx->scaleY = Q_8_8(1.5);
        }
    }

    BgFxUpdateBase();
}

void BgFxStartWideThunder(u16 tier, s32 x, s32 y, s32 z, s32 groundZ, s32 attack) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->targetZ = groundZ;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefThunder00, sx, sy);
    m4aSongNumStart(SONG_EF_THUND00);
    sBgFx->attack = attack;
    sBgFx->update = BgFxUpdateWideThunder;
    sBgFx->state = tier;
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

void BgFxUpdateEnemyDeath() {
    u16 frame;
    BgAnimGetFrameState(&frame, NULL);

    if (frame > 3) {
        sBgFx->scaleX += Q_8_8(0.05);
        sBgFx->scaleY += Q_8_8(0.05);
        SetBlendAlpha(sBgFx->timer, 16 - sBgFx->timer);

        if (sBgFx->timer > 15) {
            BgAnimStop();
        } else {
            sBgFx->timer += 2;
        }
    }

    BgFxUpdateBase();
}

void BgFxStartEnemyDeath(s32 x, s32 y, s32 z, s32 scale) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(1)) {
        return;
    }

    BgFxReset();
    sBgFx->scaleX = scale;
    sBgFx->scaleY = scale;
    SetBgBlend(sBgFx->bg, 0, 16);
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefEnemyDeath, sx, sy);
    BgAnimSetLoopStartFrame(4);
    m4aSongNumStart(SONG_EF_MON_DEATH);
    sBgFx->update = BgFxUpdateEnemyDeath;
    sBgFx->targetY = 0;
    sBgFx->timer = 0;
}

void BgFxStartDarkDeath(s32 x, s32 y, s32 z, s32 scale) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(1)) {
        return;
    }

    BgFxReset();
    sBgFx->scaleX = scale;
    sBgFx->scaleY = scale;
    SetBgBlend(sBgFx->bg, 0, 16);
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefDarkDeath, sx, sy);
    m4aSongNumStart(SONG_BTL_DARKDEAD);
    sBgFx->update = BgFxUpdateBase;
    sBgFx->targetY = 0;
    sBgFx->timer = 0;
}

void BgFxStartEnemySpawn(s32 x, s32 y, s32 z, s32 scale) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(1)) {
        return;
    }

    BgFxReset();
    sBgFx->scaleX = scale;
    sBgFx->scaleY = scale;
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefEnemySpawn, sx, sy);
    sBgFx->update = BgFxUpdateBase;
    sBgFx->targetY = 0;
    sBgFx->timer = 0;
}

void BgFxStartDarkDeathBlend(s32 x, s32 y, s32 scale, u16 target2, u16 target1) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(1)) {
        return;
    }

    BgFxReset();
    sBgFx->scaleX = scale;
    sBgFx->scaleY = scale;
    SetBgBlend(sBgFx->bg, target2, target1);
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = -0x1000;
    WorldToScreen(&sx, &sy, x, y, -0x1000);
    BgAnimStart(&gBgAnimDefDarkDeath, sx, sy);
    m4aSongNumStart(SONG_BTL_DARKDEAD);
    sBgFx->update = BgFxUpdateBase;
    sBgFx->targetY = 0;
    sBgFx->timer = 0;
}

void BgFxUpdateExplosion() {
    sBgFx->scaleX += Q_8_8(0.05);
    sBgFx->scaleY += Q_8_8(0.05);
    BgFxUpdateBase();
}

void BgFxStartExplosion(s32 x, s32 y, s32 z) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(1)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefExplosion, sx, sy);
    sBgFx->update = BgFxUpdateExplosion;
    m4aSongNumStart(SONG_EF_TARU_BOMB);
    sBgFx->flags |= BGFX_FLAG_ABOVE_SPRITES;
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

void BgFxGetPosition(s32* x, s32* y, s32* z) {
    *x = sBgFx->x;
    *y = sBgFx->y;
    *z = sBgFx->z;
}

void BgFxStartSummon(s32 x, s32 y, s32 z) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefSummon, sx, sy);
    sBgFx->update = BgFxUpdateBase;
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

void BgFxStartFriendHit(s32 x, s32 y, s32 z) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(2)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z - 0x1000;
    WorldToScreen(&sx, &sy, x, y, z - 0x1000);
    BgAnimStart(&gBgAnimDefFriendHit, sx, sy);
    sBgFx->update = BgFxUpdateBase;
}

void BgFxUpdateFollowActor() {
    BtlObj* actor = sBgFx->actor;
    sBgFx->x = actor->x;
    sBgFx->y = actor->y;
    sBgFx->z = actor->z - 0x800;
    BgFxUpdateBase();
}

void BgFxStartActorThunder(BtlObj* actor) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->actor = actor;
    sBgFx->x = actor->x;
    sBgFx->y = actor->y;
    sBgFx->z = actor->z - 0x800;
    sBgFx->scaleX = Q_8_8(1.2);
    sBgFx->scaleY = Q_8_8(1.2);
    WorldToScreen(&sx, &sy, sBgFx->x, sBgFx->y, sBgFx->z);
    BgAnimStart(&gBgAnimDefThunder00, sx, sy);
    BgAnimSetLoopStartFrame(4);
    sBgFx->update = BgFxUpdateFollowActor;
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

enum BgFxFallingThunderState {
    BGFX_FALLING_THUNDER_STATE_HOVER,
    BGFX_FALLING_THUNDER_STATE_FALL,
    BGFX_FALLING_THUNDER_STATE_SHRINK
};

void BgFxUpdateFallingThunder() {
    switch (sBgFx->state) {
    case BGFX_FALLING_THUNDER_STATE_HOVER:
        if (sBgFx->timer > 30) {
            sBgFx->state = BGFX_FALLING_THUNDER_STATE_FALL;
        } else {
            sBgFx->timer++;
        }

        break;
    case BGFX_FALLING_THUNDER_STATE_FALL:
        sBgFx->unk_3C += 64;
        sBgFx->z += sBgFx->unk_3C;

        if (sBgFx->z > 0) {
            sBgFx->z = 0;
            sBgFx->state = BGFX_FALLING_THUNDER_STATE_SHRINK;
            sBgFx->timer = 30;
        }

        sBgFx->x += (sBgFx->targetX - sBgFx->x) >> 5;
        sBgFx->angle += 4;
        break;
    case BGFX_FALLING_THUNDER_STATE_SHRINK:
        ApproachValue(&sBgFx->scaleX, 3, sBgFx->timer);
        ApproachValue(&sBgFx->scaleY, 3, sBgFx->timer);
        sBgFx->timer--;

        if (sBgFx->timer <= 0) {
            BgAnimStop();
            sBgFx->update = NULL;
        }

        break;
    }

    ApplyAttackBox(256, sBgFx->x, sBgFx->y, sBgFx->z, 32, 32, 32);
    BgFxUpdateBase();
}

void BgFxStartFallingThunder(s32 x, s32 y, s32 z, s32 targetX, s32 vz) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->targetX = targetX;
    sBgFx->unk_3C = vz;
    sBgFx->state = BGFX_FALLING_THUNDER_STATE_HOVER;
    sBgFx->scaleX = Q_8_8(2.6);
    sBgFx->scaleY = Q_8_8(2.6);
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefThunder00, sx, sy);
    BgAnimSetLoopStartFrame(4);
    sBgFx->update = BgFxUpdateFallingThunder;
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

enum BgFxDsdEnergyState {
    BGFX_DSD_ENERGY_STATE_CHARGE,
    BGFX_DSD_ENERGY_STATE_HOLD,
    BGFX_DSD_ENERGY_STATE_FADE_OUT,
    BGFX_DSD_ENERGY_STATE_END
};

void BgFxUpdateDsdEnergy() {
    switch (sBgFx->state) {
    case BGFX_DSD_ENERGY_STATE_CHARGE:
        ApproachValue(&sBgFx->scaleX, sBgFx->targetX, sBgFx->steps);
        sBgFx->scaleY = sBgFx->scaleX;
        ApproachValue(&sBgFx->unk_3C, 0x1000, sBgFx->steps);
        SetBlendAlpha(16, sBgFx->unk_3C >> 8);
        sBgFx->steps--;

        if (sBgFx->steps <= 0) {
            sBgFx->timer = 0;
            sBgFx->state = BGFX_DSD_ENERGY_STATE_HOLD;
        }

        break;
    case BGFX_DSD_ENERGY_STATE_HOLD:
        if (sBgFx->endSignals != 0) {
            sBgFx->state = BGFX_DSD_ENERGY_STATE_FADE_OUT;
            sBgFx->timer = 0;
            sBgFx->steps = 16;
        }

        break;
    case BGFX_DSD_ENERGY_STATE_FADE_OUT:
        ApproachValue(&sBgFx->unk_3C, 0, sBgFx->steps);
        SetBlendAlpha(16, sBgFx->unk_3C >> 8);
        sBgFx->steps--;

        if (sBgFx->steps <= 0) {
            sBgFx->timer = 0;
            sBgFx->state = BGFX_DSD_ENERGY_STATE_END;
            BgAnimStop();
        }

        break;
    }

    sBgFx->angle += sBgFx->unk_0C;
    BgFxUpdateBase();
}

void func_080144D8(s32 x, s32 y, s32 z, s32 scale, u16 steps, u16 spinSpeed) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    SetBlendAlpha(16, 0);
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->state = BGFX_DSD_ENERGY_STATE_CHARGE;
    sBgFx->scaleX = scale;
    sBgFx->scaleY = scale;
    sBgFx->targetX = scale;
    sBgFx->unk_3C = 0;
    sBgFx->endSignals = 0;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&sBgAnimDefRagnarokChargePurple, sx, sy);
    BgAnimSetLoopStartFrame(0);
    sBgFx->update = BgFxUpdateDsdEnergy;
    sBgFx->steps = steps;
    sBgFx->unk_0C = spinSpeed;
}

void BgFxStartDsdEnergy(s32 x, s32 y, s32 z, s32 scale, s32 steps, s32 spinSpeed) {
    u16 chargeSteps = steps;
    u16 spin = spinSpeed;
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    SetBlendAlpha(16, 0);
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->state = BGFX_DSD_ENERGY_STATE_CHARGE;
    sBgFx->scaleX = scale;
    sBgFx->scaleY = scale;
    sBgFx->targetX = scale;
    sBgFx->unk_3C = 0;
    sBgFx->endSignals = 0;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&sBgAnimDefDarkGlow, sx, sy);
    BgAnimSetLoopStartFrame(0);
    sBgFx->update = BgFxUpdateDsdEnergy;
    sBgFx->steps = chargeSteps;
    sBgFx->unk_0C = spin;
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

void func_08014654() {
    u16 timer;
    s16 fade;
    sBgFx->scaleX += Q_8_8(0.1);
    sBgFx->scaleY += Q_8_8(0.1);
    BgFxUpdateBase();
    timer = sBgFx->timer;

    if (sBgFx->timer > 3) {
        fade = timer - 3;
        SetBlendAlpha(16, 16 - fade);

        if (fade > 15) {
            BgAnimStop();
        }
    }

    sBgFx->timer++;
}

void func_080146A8(s32 x, s32 y, s32 z) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z - 0x1000;
    WorldToScreen(&sx, &sy, x, y, 0);
    sBgFx->scaleX = Q_8_8(2);
    sBgFx->scaleY = Q_8_8(3);
    sBgFx->angle = 0;
    BgAnimStart(&gBgAnimDefFlash, sx, sy);
    BgAnimSetLoopStartFrame(0);
    sBgFx->update = func_08014654;
    sBgFx->timer = 0;
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

void BgFxAddPosition(s32 dx, s32 dy, s32 dz) {
    sBgFx->x += dx;
    sBgFx->y += dy;
    sBgFx->z += dz;
}

void BgFxSetPosition(s32 x, s32 y, s32 z) {
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
}

void BgFxSignalEnd(u8 bit) {
    sBgFx->endSignals |= 1 << bit;
}

void BgFxSetTarget(s32 x, s32 y, s32 z) {
    sBgFx->targetX = x;
    sBgFx->targetY = y;
    sBgFx->targetZ = z;
}

void BgFxSetAngle(u8 angle) {
    sBgFx->angle = angle;
}

void BgFxSetScale(s32 scaleX, s32 scaleY) {
    sBgFx->scaleX = scaleX;
    sBgFx->scaleY = scaleY;
}

void BgFxStartGroundImpact(s32 x, s32 y) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    SetBgBlend(sBgFx->bg, 5, 16);
    sBgFx->x = x;
    sBgFx->y = y;
    WorldToScreen(&sx, &sy, x, y, 0);
    BgAnimStart(&gBgAnimDefGroundImpact, sx, sy);
    sBgFx->update = BgFxUpdateBase;
    sBgFx->flags |= BGFX_FLAG_ABOVE_SPRITES;
}

void BgFxUpdateStop() {
    switch (sBgFx->state) {
    case SPELL_TIER_BASE:
#ifdef VERSION_EU
        if (sBgFx->timer == 20) {
#else
        if (sBgFx->timer == 21) {
#endif
            ApplyAttackBox(sBgFx->attack, sBgFx->x, sBgFx->y, sBgFx->z, 16, 16, 48);
        }

        break;
    case SPELL_TIER_RA:
#ifdef VERSION_EU
        if (sBgFx->timer == 20) {
#else
        if (sBgFx->timer == 25) {
#endif
            ApplyAttackBox(sBgFx->attack, sBgFx->x, sBgFx->y, sBgFx->z, 24, 24, 48);
        }

        break;
    case SPELL_TIER_GA:
        if (sBgFx->timer == 33) {
            ApplyAttackBox(sBgFx->attack, sBgFx->x, sBgFx->y, sBgFx->z, 32, 32, 48);
        }

        break;
    }

    sBgFx->timer++;
    BgFxUpdateBase();
}

void BgFxStartStop(u16 tier, s32 x, s32 y, s32 z, s32 attack) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->releaseFrames = 20;
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->attack = attack;
    sBgFx->state = tier;
    WorldToScreen(&sx, &sy, x, y, 0);

    switch (tier) {
    case SPELL_TIER_BASE:
        BgAnimStart(&gBgAnimDefStop00, sx, sy);
        m4aSongNumStart(SONG_EF_STOP00);
        break;
    case SPELL_TIER_RA:
        BgAnimStart(&gBgAnimDefStop01, sx, sy);
        m4aSongNumStart(SONG_EF_STOP01);
        break;
    case SPELL_TIER_GA:
    default:
        BgAnimStart(&gBgAnimDefStop02, sx, sy);
        m4aSongNumStart(SONG_EF_STOP02);
        break;
    }

    sBgFx->update = BgFxUpdateStop;
}

void BgFxStartCharaDefeat(s32 x, s32 y) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    WorldToScreen(&sx, &sy, x, y, 0);
    BgAnimStart(&gBgAnimDefCharaDefeat, sx, sy);
    BgAnimSetLoopStartFrame(8);
    sBgFx->update = BgFxUpdateBase;
    sBgFx->flags |= BGFX_FLAG_ABOVE_SPRITES;
}

void BgFxStartHumDefeat(s32 x, s32 y) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    WorldToScreen(&sx, &sy, x, y, 0);
    BgAnimStart(&gBgAnimDefHumDefeat, sx, sy);
    BgAnimSetLoopStartFrame(7);
    sBgFx->update = BgFxUpdateBase;
    sBgFx->flags |= BGFX_FLAG_ABOVE_SPRITES;
}

void BgFxStartBossDeath(s32 x, s32 y) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    WorldToScreen(&sx, &sy, x, y, 0);
    m4aSongNumStart(SONG_BTL_GF_LOOP);
    BgAnimStart(&gBgAnimDefBossDeath, sx, sy);
    BgAnimSetLoopStartFrame(4);
    sBgFx->update = BgFxUpdateBase;
    sBgFx->flags |= BGFX_FLAG_ABOVE_SPRITES;
}

void BgFxStartCharaDefeatEnd(s32 x, s32 y) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    WorldToScreen(&sx, &sy, x, y, 0);
    BgAnimStart(&gBgAnimDefCharaDefeatEnd, sx, sy);
    BgAnimSetLoopStartFrame(0);
    sBgFx->update = BgFxUpdateBase;
    sBgFx->flags |= BGFX_FLAG_ABOVE_SPRITES;
}

enum BgFxGravityPhase {
    BGFX_GRAVITY_PHASE_CAST,
    BGFX_GRAVITY_PHASE_STRIKE
};

void BgFxUpdateGravity() {
    s16 sx;
    s16 sy;
    s16 pulse;
    u8 hit;

    pulse = sBgFx->timer % 8;

    if (pulse <= 3) {
        SetBlendAlpha(pulse, 16);
    } else {
        SetBlendAlpha(8 - pulse, 16);
    }

    if (sBgFx->steps == BGFX_GRAVITY_PHASE_CAST && BgAnimIsStopped()) {
        sBgFx->x = sBgFx->targetX;
        sBgFx->y = sBgFx->targetY;
        sBgFx->z = sBgFx->targetZ;
        WorldToScreen(&sx, &sy, sBgFx->x, sBgFx->x, sBgFx->z);
        sBgFx->angle = 0;
        sBgFx->releaseFrames = 20;

        switch (sBgFx->state) {
        case SPELL_TIER_BASE:
            sBgFx->targetX = Q_8_8(0.5);
            sBgFx->targetY = Q_8_8(0.5);
            BgAnimStart(&gBgAnimDefGravity01, sx, sy);
            break;
        case SPELL_TIER_RA:
            sBgFx->targetX = Q_8_8(1);
            sBgFx->targetY = Q_8_8(1);
            BgAnimStart(&gBgAnimDefGravity01, sx, sy);
            break;
        case SPELL_TIER_GA:
        default:
            sBgFx->targetX = Q_8_8(2);
            sBgFx->targetY = Q_8_8(2);
            BgAnimStart(&gBgAnimDefGravity01, sx, sy);
            break;
        }

        if (sBgFx->flags & BGFX_FLAG_FLIP_X) {
            sBgFx->targetX = -sBgFx->targetX;
        }

        m4aSongNumStart(SONG_EF_GRABI01);
        sBgFx->steps++;
    } else if (sBgFx->steps == BGFX_GRAVITY_PHASE_STRIKE) {
        if (sBgFx->timer <= 29) {
            ApproachValue(&sBgFx->scaleX, sBgFx->targetX, 30 - sBgFx->timer);
            ApproachValue(&sBgFx->scaleY, sBgFx->targetY, 30 - sBgFx->timer);
        }

        if (sBgFx->timer == 35) {
            switch (sBgFx->state) {
            case SPELL_TIER_BASE:
                hit = ApplyAttackBox(sBgFx->attack, sBgFx->x, sBgFx->y, sBgFx->z, 24, 12, 256);
                break;
            case SPELL_TIER_RA:
                hit = ApplyAttackBox(sBgFx->attack, sBgFx->x, sBgFx->y, sBgFx->z, 40, 20, 256);
                break;
            case SPELL_TIER_GA:
            default:
                hit = ApplyAttackBox(sBgFx->attack, sBgFx->x, sBgFx->y, sBgFx->z, 80, 40, 256);
                break;
            }

            if (hit) {
                m4aSongNumStart(SONG_EF_GRABI02);
            }
        }

        sBgFx->timer++;
    }

    BgFxUpdateBase();
}

void BgFxStartGravity(u16 tier, s32 x, s32 y, s32 z, s32 targetX, s32 targetY, s32 targetZ, u8 flip, s32 attack) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    SetBlendAlpha(2, 16);
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->targetX = targetX;
    sBgFx->targetY = targetY;
    sBgFx->targetZ = targetZ;
    sBgFx->attack = attack;

    if (flip) {
        sBgFx->scaleX = -sBgFx->scaleX;
        sBgFx->flags |= BGFX_FLAG_FLIP_X;
    }

    sBgFx->steps = BGFX_GRAVITY_PHASE_CAST;
    WorldToScreen(&sx, &sy, sBgFx->x, sBgFx->y, sBgFx->z);
    BgAnimStart(&gBgAnimDefGravity00, sx, sy);
    m4aSongNumStart(SONG_EF_GRABI00);
    sBgFx->state = tier;
    sBgFx->update = BgFxUpdateGravity;
}

void BgFxUpdateGravityStrike() {
    s16 pulse = sBgFx->timer % 8;

    if (pulse <= 3) {
        SetBlendAlpha(pulse, 16);
    } else {
        SetBlendAlpha(8 - pulse, 16);
    }

    if (sBgFx->timer == 0x23) {
        if (ApplyAttackBox(sBgFx->attack, sBgFx->x, sBgFx->y, sBgFx->z, 40, 20, 256)) {
            m4aSongNumStart(SONG_EF_GRABI02);
        }
    }

    BgFxUpdateBase();
    sBgFx->timer++;
}

void BgFxStartGravityStrike(s32 x, s32 y, s32 z, s32 attack) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    SetBlendAlpha(2, 16);
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->attack = attack;
    sBgFx->steps = 0;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefGravity01, sx, sy);
    m4aSongNumStart(SONG_EF_GRABI01);
    sBgFx->update = BgFxUpdateGravityStrike;
}

void BgFxUpdateShockwave() {
    sBgFx->scaleX += Q_8_8(0.5);
    sBgFx->scaleY += Q_8_8(0.5);
    sBgFx->angle += 3;

    if (sBgFx->flags & BGFX_FLAG_FLIP_X) {
        sBgFx->x += -0x300;
    } else {
        sBgFx->x += 0x300;
    }

    BgFxUpdateBase();
    SetBlendAlpha(16, 8 - sBgFx->steps);

    if (sBgFx->steps > 7) {
        BgAnimStop();
    }

    if (sBgFx->timer % 5 == 0) {
        sBgFx->steps++;
    }

    sBgFx->timer++;
}

void BgFxStartShockwave(s32 x, s32 y, u8 flip) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y - 0x1000;
    SetBlendAlpha(16, 8);

    if (flip) {
        sBgFx->flags |= BGFX_FLAG_FLIP_X;
    }

    WorldToScreen(&sx, &sy, sBgFx->x, sBgFx->y, 0);
    sBgFx->scaleX = Q_8_8(0.5);
    sBgFx->scaleY = Q_8_8(0.5);
    sBgFx->angle = 0;
    sBgFx->steps = 0;
    BgAnimStart(&gBgAnimDefFlash, sx, sy);
    BgAnimSetLoopStartFrame(0);
    sBgFx->update = BgFxUpdateShockwave;
    sBgFx->timer = 0;
    sBgFx->flags |= BGFX_FLAG_ABOVE_SPRITES;
}

void BgFxUpdateGas() {
    if (sBgFx->timer > 19) {
        sBgFx->steps = (sBgFx->timer - 20) / 2;
        SetBlendAlpha(16, 16 - sBgFx->steps);

        if (sBgFx->steps > 15) {
            BgAnimStop();
        }
    }

    sBgFx->timer++;
    BgFxUpdateBase();
}

void BgFxStartGas(s32 x, s32 y, s32 z, u8 facingLeft) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    SetBlendAlpha(16, 16);
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->scaleX = Q_8_8(1);
    sBgFx->scaleY = Q_8_8(1);
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefGas, sx, sy);
    sBgFx->update = BgFxUpdateGas;
    m4aSongNumStart(SONG_EF_BFG_GASS);

    if (!facingLeft) {
        sBgFx->flags |= BGFX_FLAG_FLIP_X;
    }

    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
    BgAnimSetLoopStartFrame(3);
}

enum BgFxFadeInOutState {
    BGFX_FADE_IN_OUT_STATE_FADE_IN,
    BGFX_FADE_IN_OUT_STATE_HOLD,
    BGFX_FADE_IN_OUT_STATE_FADE_OUT
};

void BgFxUpdateFadeInOut() {
    switch (sBgFx->state) {
    case BGFX_FADE_IN_OUT_STATE_FADE_IN:
        SetBlendAlpha(16, sBgFx->timer);

        if (sBgFx->timer > 15) {
            sBgFx->timer = 0;
            sBgFx->state = BGFX_FADE_IN_OUT_STATE_HOLD;
        } else {
            sBgFx->timer++;
        }

        break;
    case BGFX_FADE_IN_OUT_STATE_HOLD:
        if (sBgFx->timer > sBgFx->steps) {
            sBgFx->timer = 0;
            sBgFx->state = BGFX_FADE_IN_OUT_STATE_FADE_OUT;
        } else {
            sBgFx->timer++;
        }

        break;
    case BGFX_FADE_IN_OUT_STATE_FADE_OUT:
        SetBlendAlpha(16, 16 - sBgFx->timer);

        if (sBgFx->timer > 15) {
            BgAnimStop();
        } else {
            sBgFx->timer++;
        }

        break;
    }

    BgFxUpdateBase();
}

void BgFxStartBoogieKaihuku(s32 x, s32 y, s32 z, s32 scale) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    SetBlendAlpha(16, 0);
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    WorldToScreen(&sx, &sy, x, y, z);
    sBgFx->scaleX = scale;
    sBgFx->scaleY = scale;
    sBgFx->state = BGFX_FADE_IN_OUT_STATE_FADE_IN;
    sBgFx->steps = 30;
    BgAnimStart(&gBgAnimDefBoogieKaihuku, sx, sy);
    BgAnimSetLoopStartFrame(0);
    sBgFx->update = BgFxUpdateFadeInOut;
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

enum BgFxBossDeathFlashState {
    BGFX_BOSS_DEATH_FLASH_STATE_GLOW,
    BGFX_BOSS_DEATH_FLASH_STATE_WHITE_OUT
};

void BgFxUpdateBossDeathFlash() {
    switch (sBgFx->state) {
    case BGFX_BOSS_DEATH_FLASH_STATE_GLOW:
        if (sBgFx->timer > 30) {
            FadeStartOut(FADE_MODE_ADD_WHITE, 60);
            FadeLock();
            sBgFx->timer = 0;
            sBgFx->state = BGFX_BOSS_DEATH_FLASH_STATE_WHITE_OUT;
        } else {
            sBgFx->timer++;
        }

        break;
    case BGFX_BOSS_DEATH_FLASH_STATE_WHITE_OUT:
        if (!FadeIsActive()) {
            BgAnimStop();
            FadeStartIn(FADE_MODE_ADD_WHITE, 120);
            FadeLock();
            sBgFx->update = NULL;
            sBgFx->flags &= ~BGFX_FLAG_ACTIVE;
        }

        break;
    }

    BgAnimSetTransform(0, sBgFx->scaleX, sBgFx->scaleY);
    sBgFx->scaleX += 20;
    sBgFx->scaleY += 20;
}

void BgFxStartBossDeathFlash() {
    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->state = BGFX_BOSS_DEATH_FLASH_STATE_GLOW;
    sBgFx->scaleX = Q_8_8(0.1);
    sBgFx->scaleY = Q_8_8(0.1);
    BgAnimStart(&gBgAnimDefGlow, 0x78, 0x50);
    BgAnimSetLoopStartFrame(0);
    m4aSongNumStart(SONG_EF_DBOSS_DEAD);
    sBgFx->update = BgFxUpdateBossDeathFlash;
    sBgFx->flags |= BGFX_FLAG_ABOVE_SPRITES;
}

enum BgFxPcShotState {
    BGFX_PC_SHOT_STATE_GROW,
    BGFX_PC_SHOT_STATE_FLY,
    BGFX_PC_SHOT_STATE_FADE_OUT
};

void BgFxUpdatePcShot() {
    s16 timer;
    s16 halfSize;

    switch (sBgFx->state) {
    case BGFX_PC_SHOT_STATE_GROW:
        timer = sBgFx->timer;
        SetBlendAlpha(16, timer);
        ApproachValueHalfSteps(&sBgFx->scaleX, Q_8_8(1), 17 - timer);
        sBgFx->scaleY = sBgFx->scaleX;

        if (timer > 15) {
            sBgFx->timer = sBgFx->steps;
            sBgFx->state = BGFX_PC_SHOT_STATE_FLY;
        } else {
            sBgFx->timer++;
        }

        break;
    case BGFX_PC_SHOT_STATE_FLY:
        ApproachValue(&sBgFx->x, sBgFx->targetX, sBgFx->timer);
        ApproachValue(&sBgFx->y, sBgFx->targetY, sBgFx->timer);
        ApproachValue(&sBgFx->z, sBgFx->targetZ, sBgFx->timer);
        ApproachValue(&sBgFx->scaleX, sBgFx->unk_3C, sBgFx->timer);
        sBgFx->scaleY = sBgFx->scaleX;
        halfSize = (sBgFx->scaleX * 3) >> 6;

        if (sBgFx->timer <= 0) {
            sBgFx->timer = 0;
            sBgFx->state = BGFX_PC_SHOT_STATE_FADE_OUT;
        } else if (ApplyAttackBox(sBgFx->attack, sBgFx->x, sBgFx->y, sBgFx->z, halfSize, halfSize, halfSize)) {
            m4aSongNumStart(SONG_BTL_AN_WAVEHIT);
            sBgFx->timer = 0;
            sBgFx->state = BGFX_PC_SHOT_STATE_FADE_OUT;
        } else {
            sBgFx->timer--;
        }

        break;
    case BGFX_PC_SHOT_STATE_FADE_OUT:
        timer = sBgFx->timer;
        SetBlendAlpha(16, 16 - timer);

        if (timer > 15) {
            BgAnimStop();
        } else {
            sBgFx->scaleX += 7;
            sBgFx->scaleY += 7;
            sBgFx->z -= sBgFx->scaleX;
            sBgFx->timer++;
        }

        break;
    }

    BgFxUpdateBase();
}

void BgFxStartPcShot(s32 x, s32 y, s32 z, s32 targetX, s32 targetY, s32 targetZ, s32 attack, u16 steps, s32 targetScale) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    SetBlendAlpha(16, 0);
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->targetX = targetX;
    sBgFx->targetY = targetY;
    sBgFx->targetZ = targetZ;
    sBgFx->steps = steps;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefPcShot, sx, sy);
    BgAnimSetLoopStartFrame(0);
    sBgFx->scaleX = Q_8_8(0.1);
    sBgFx->scaleY = Q_8_8(0.1);
    sBgFx->update = BgFxUpdatePcShot;
    sBgFx->state = BGFX_PC_SHOT_STATE_GROW;
    sBgFx->attack = attack;
    sBgFx->unk_3C = targetScale;
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

void BgFxStartThunderStrike(s32 x, s32 y, s32 z, s32 attack) {
    s16 sx;
    s16 sy;
    s32 height;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    WorldToScreen(&sx, &sy, x, y, z);
    height = sy << 8;
    sBgFx->scaleX = Q_8_8(1.5);
    sBgFx->scaleY = height / 40;

    if (sBgFx->scaleY < Q_8_8(1.5)) {
        sBgFx->scaleY = Q_8_8(1.5);
    }

    BgAnimStart(&gBgAnimDefThunder01, sx, sy);
    m4aSongNumStart(SONG_EF_THUND01);
    ApplyAttackBox(attack, sBgFx->x, sBgFx->y, sBgFx->z, 16, 16, 256);
    sBgFx->update = BgFxUpdateBase;
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

enum BgFxThunderPhase {
    BGFX_THUNDER_PHASE_CAST,
    BGFX_THUNDER_PHASE_STRIKE
};

void BgFxUpdateThunder() {
    s16 sx;
    s16 sy;
    s16 sx2;
    s16 sy2;

    if (sBgFx->timer == BGFX_THUNDER_PHASE_CAST && BgAnimIsStopped()) {
        SetBlendAlpha(16, 16);
        sBgFx->x = sBgFx->targetX;
        sBgFx->y = sBgFx->targetY;
        sBgFx->z = sBgFx->targetZ;
        WorldToScreen(&sx, &sy, sBgFx->x, sBgFx->y, sBgFx->z);
        sBgFx->angle = 0;
        sBgFx->scaleX = Q_8_8(1.5);
        sBgFx->scaleY = (sy << 8) / 40;

        if (sBgFx->scaleY < Q_8_8(1.5)) {
            sBgFx->scaleY = Q_8_8(1.5);
        }

        sBgFx->flags |= BGFX_FLAG_IGNORE_ZOOM;

        switch (sBgFx->state) {
        case SPELL_TIER_BASE:
            ApplyAttackBox(sBgFx->attack, sBgFx->x, sBgFx->y, sBgFx->z, 16, 16, 256);
            BgAnimStart(&gBgAnimDefThunder01, sx, sy);
            m4aSongNumStart(SONG_EF_THUND01);
            break;
        case SPELL_TIER_RA:
            ApplyAttackBox(sBgFx->attack, sBgFx->x, sBgFx->y, sBgFx->z, 40, 40, 256);
            BgAnimStart(&gBgAnimDefThunder02, sx, sy);
            m4aSongNumStart(SONG_EF_THUND02);
            break;
        case SPELL_TIER_GA:
            ApplyAttackBox(sBgFx->attack, sBgFx->x, sBgFx->y, sBgFx->z, 64, 64, 256);
            BgAnimStart(&gBgAnimDefThunder03, sx, sy);
            m4aSongNumStart(SONG_EF_THUND03);
            break;
        }

        sBgFx->timer = BGFX_THUNDER_PHASE_STRIKE;
    } else if (sBgFx->timer == BGFX_THUNDER_PHASE_STRIKE) {
        s32 height;

        WorldToScreen(&sx2, &sy2, sBgFx->x, sBgFx->y, sBgFx->z);
        height = sy2 << 8;
        sBgFx->scaleY = height / 40;

        if (sBgFx->scaleY < Q_8_8(1.5)) {
            sBgFx->scaleY = Q_8_8(1.5);
        }
    }

    BgFxUpdateBase();
}

void BgFxStartThunder(u16 tier, s32 x, s32 y, s32 z, s32 targetX, s32 targetY, s32 targetZ, s32 attack) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->targetX = targetX;
    sBgFx->targetY = targetY;
    sBgFx->targetZ = targetZ;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefThunder00, sx, sy);
    m4aSongNumStart(SONG_EF_THUND00);
    sBgFx->attack = attack;
    sBgFx->update = BgFxUpdateThunder;
    sBgFx->state = tier;
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

enum BgFxDumboSplashState {
    BGFX_DUMBO_SPLASH_STATE_EXTEND,
    BGFX_DUMBO_SPLASH_STATE_SWAY,
    BGFX_DUMBO_SPLASH_STATE_RETRACT,
    BGFX_DUMBO_SPLASH_STATE_END = 99
};

void BgFxUpdateDumboSplash() {
    s32 reach;
    u16 timer;

    switch (sBgFx->state) {
    case BGFX_DUMBO_SPLASH_STATE_EXTEND:
        if (sBgFx->timer == 0) {
            sBgFx->steps = 40;
        }

        ApproachValue(&sBgFx->unk_3C, 0x1000, sBgFx->steps);
        ApproachValue(&sBgFx->scaleX, sBgFx->targetX, sBgFx->steps);
        ApproachValue(&sBgFx->scaleY, Q_8_8(1), sBgFx->steps);
        ApproachValue(&sBgFx->angleFixed, 0, sBgFx->steps);
        sBgFx->angle = sBgFx->angleFixed >> 8;
        reach = sBgFx->scaleX;

        if (reach < 0) {
            reach = -reach;
        }

        reach *= 44;

        if (sBgFx->flags & BGFX_FLAG_FLIP_X) {
            ApplyAttackBox(sBgFx->attack, sBgFx->x + reach, sBgFx->y,
                sBgFx->z, reach << 8 >> 16, 24, 24);
        } else {
            ApplyAttackBox(sBgFx->attack, sBgFx->x - reach, sBgFx->y,
                sBgFx->z, reach << 8 >> 16, 24, 24);
        }

        SetBlendAlpha(16, sBgFx->unk_3C >> 8);

        if (--sBgFx->steps <= 0) {
            sBgFx->timer = 0;
            sBgFx->state = BGFX_DUMBO_SPLASH_STATE_SWAY;
        } else {
            sBgFx->timer++;
        }

        break;
    case BGFX_DUMBO_SPLASH_STATE_SWAY:
        timer = sBgFx->timer;

        if (sBgFx->timer > 50) {
            sBgFx->timer = 0;
            sBgFx->state = BGFX_DUMBO_SPLASH_STATE_RETRACT;
            break;
        }

        reach = sBgFx->scaleX;

        if (reach < 0) {
            reach = -reach;
        }

        reach *= 44;

        if (sBgFx->flags & BGFX_FLAG_FLIP_X) {
            sBgFx->angleFixed = -SIN(timer * 4) * 6;
            ApplyAttackBox(sBgFx->attack, sBgFx->x + reach, sBgFx->y,
                sBgFx->z, reach << 8 >> 16, 24, 24);
        } else {
            sBgFx->angleFixed = SIN(timer * 4) * 6;
            ApplyAttackBox(sBgFx->attack, sBgFx->x - reach, sBgFx->y,
                sBgFx->z, reach << 8 >> 16, 24, 24);
        }

        sBgFx->angle = sBgFx->angleFixed >> 8;
        sBgFx->timer++;
        break;
    case BGFX_DUMBO_SPLASH_STATE_RETRACT:
        if (sBgFx->timer == 0) {
            sBgFx->steps = 20;
        }

        ApproachValue(&sBgFx->unk_3C, 0, sBgFx->steps);
        ApproachValue(&sBgFx->scaleY, Q_8_8(0.5), sBgFx->steps);

        if (sBgFx->flags & BGFX_FLAG_FLIP_X) {
            ApproachValue(&sBgFx->scaleX, Q_8_8(-0.5), sBgFx->steps);
            ApproachValue(&sBgFx->angleFixed, 0x800, sBgFx->steps);
        } else {
            ApproachValue(&sBgFx->scaleX, Q_8_8(0.5), sBgFx->steps);
            ApproachValue(&sBgFx->angleFixed, -0x800, sBgFx->steps);
        }

        sBgFx->angle = sBgFx->angleFixed >> 8;
        SetBlendAlpha(16, sBgFx->unk_3C >> 8);

        if (--sBgFx->steps <= 0) {
            BgAnimStop();
            sBgFx->state = BGFX_DUMBO_SPLASH_STATE_END;
        } else {
            sBgFx->timer++;
        }

        break;
    }

    BgFxUpdateBase();
}

void BgFxStartDumboSplash(u16 level, s32 x, s32 y, s32 z, u8 flip, s32 attack) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    SetBlendAlpha(16, 0);

    if (flip) {
        sBgFx->flags |= BGFX_FLAG_FLIP_X;
    }

    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->attack = attack;
    WorldToScreen(&sx, &sy, x, y, z);
    sBgFx->scaleY = Q_8_8(0.1);

    switch (level) {
    case SUMMON_LEVEL_SINGLE:
        sBgFx->targetX = Q_8_8(1);
        break;
    case SUMMON_LEVEL_PAIR:
        sBgFx->targetX = Q_8_8(1.5);
        break;
    case SUMMON_LEVEL_TRIPLE:
        sBgFx->targetX = Q_8_8(2);
        break;
    }

    if (sBgFx->flags & BGFX_FLAG_FLIP_X) {
        sBgFx->scaleX = Q_8_8(-0.1);
        sBgFx->angleFixed = 2048;
        sBgFx->targetX = -sBgFx->targetX;
    } else {
        sBgFx->scaleX = Q_8_8(0.1);
        sBgFx->angleFixed = -2048;
    }

    sBgFx->state = BGFX_DUMBO_SPLASH_STATE_EXTEND;
    sBgFx->unk_3C = 0;
    BgAnimStart(&gBgAnimDefDumboSplash, sx, sy);
    BgAnimSetLoopStartFrame(0);
    sBgFx->update = BgFxUpdateDumboSplash;
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

void func_08015C80(s32 x, s32 y, s32 z) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    WorldToScreen(&sx, &sy, x, y, z);
    sBgFx->scaleX = Q_8_8(0.3);
    sBgFx->scaleY = Q_8_8(0.3);
    sBgFx->targetX = Q_8_8(0.05);
    sBgFx->targetY = Q_8_8(0.05);
    BgAnimStart(&gBgAnimDefFlash, sx, sy);
    BgAnimSetLoopStartFrame(0);
    sBgFx->update = BgFxUpdateFlash;
}

void BgFxUpdateTrinityLimit() {
    if (sBgFx->timer <= 16) {
        SetBlendAlpha(16, sBgFx->timer);
        sBgFx->timer++;
    }

    BgFxUpdateBase();
}

void BgFxStartTrinityLimit(s32 x, s32 y, s32 z) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->scaleX = Q_8_8(1);
    sBgFx->scaleY = Q_8_8(1);
    WorldToScreen(&sx, &sy, x, y, z);
    sBgFx->flags |= BGFX_FLAG_BELOW_SPRITES;
    SetBlendAlpha(16, 0);
    sBgFx->state = 0;
    BgAnimStart(&gBgAnimDefTrinityLimit, sx, sy);
    BgAnimSetLoopStartFrame(0);
    sBgFx->update = BgFxUpdateTrinityLimit;
}

void BgFxStartTrinityLimitCharge(s32 x, s32 y, s32 z) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    WorldToScreen(&sx, &sy, x, y, z);
    sBgFx->scaleX = Q_8_8(2);
    sBgFx->scaleY = Q_8_8(2);
    BgAnimSetLoopStartFrame(7);
    BgAnimStart(&gBgAnimDefTrinityLimitCharge, sx, sy);
    sBgFx->update = BgFxUpdateBase;
}

enum BgFxTrinityLimitBlastState {
    BGFX_TRINITY_LIMIT_BLAST_STATE_FADE_IN,
    BGFX_TRINITY_LIMIT_BLAST_STATE_EXPAND,
    BGFX_TRINITY_LIMIT_BLAST_STATE_FADE_OUT
};

void BgFxUpdateTrinityLimitBlast() {
    switch (sBgFx->state) {
    case BGFX_TRINITY_LIMIT_BLAST_STATE_FADE_IN: {
        s16 timer = sBgFx->timer;

        if (timer <= 64) {
            SetBlendAlpha(16, timer >> 2);
            sBgFx->timer++;
        } else {
            sBgFx->timer = 0;
            sBgFx->state++;
        }

        sBgFx->scaleX += Q_8_8(0.1);
        sBgFx->scaleY += Q_8_8(0.1);
        break;
    }
    case BGFX_TRINITY_LIMIT_BLAST_STATE_EXPAND: {
        u16 timer;

        sBgFx->scaleX += Q_8_8(1);
        sBgFx->scaleY += Q_8_8(1);
        timer = sBgFx->timer;

        if ((s16)timer > 32) {
            ApplyAttackBox(87, sBgFx->x, sBgFx->y, 0, 0x100, 0x100, 0x100);
            SetBattleZoom(1, Q_8_8(1), gBtlWork->x2, gBtlWork->y2);
            FadeStartIn(FADE_MODE_ADD_WHITE, 60);
            sBgFx->timer = 0;
            sBgFx->state++;
        } else {
            sBgFx->timer = timer + 1;
        }

        break;
    }
    case BGFX_TRINITY_LIMIT_BLAST_STATE_FADE_OUT: {
        u16 timer = sBgFx->timer;

        if ((s16)timer <= 15) {
            SetBlendAlpha(16, 16 - timer);
            sBgFx->timer++;
        } else {
            BgAnimStop();
        }

        break;
    }
    }

    sBgFx->angle++;
    BgFxUpdateBase();
}

void BgFxStartTrinityLimitBlast(s32 x, s32 y, s32 z) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    SetBlendAlpha(16, 0);
    WorldToScreen(&sx, &sy, sBgFx->x, sBgFx->y, sBgFx->z);
    sBgFx->scaleX = Q_8_8(3);
    sBgFx->scaleY = Q_8_8(3);
    BgAnimStart(&gBgAnimDefTrinityLimitBlast, sx, sy);
    BgAnimSetLoopStartFrame(0);
    sBgFx->timer = 8;
    sBgFx->state = BGFX_TRINITY_LIMIT_BLAST_STATE_FADE_IN;
    sBgFx->flags |= BGFX_FLAG_ABOVE_SPRITES;
    sBgFx->update = BgFxUpdateTrinityLimitBlast;
    FadeStartOut(FADE_MODE_ADD_WHITE, 40);
    SetBattleZoom(80, Q_8_8(0.8), x, y + z + 0x2000);
}

void BgFxUpdateRagnarokCharge() {
    if (sBgFx->timer > 0) {
        ApproachValueHalfSteps(&sBgFx->scaleX, Q_8_8(1), sBgFx->timer);
        ApproachValueHalfSteps(&sBgFx->scaleY, Q_8_8(1), sBgFx->timer);
        sBgFx->timer--;
    }

    BgFxUpdateBase();
}

void BgFxStartRagnarokCharge(s32 x, s32 y, s32 z) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    WorldToScreen(&sx, &sy, x, y, z);
    sBgFx->scaleX = Q_8_8(0.1);
    sBgFx->scaleY = Q_8_8(0.1);
    m4aSongNumStart(SONG_EF_RAGNA01);
    BgAnimStart(&gBgAnimDefRagnarokCharge, sx, sy);
    BgAnimSetLoopStartFrame(0);
    sBgFx->timer = 0x23;
    sBgFx->update = BgFxUpdateRagnarokCharge;
}

enum BgFxRagnarokShotState {
    BGFX_RAGNAROK_SHOT_STATE_HOLD,
    BGFX_RAGNAROK_SHOT_STATE_FLY,
    BGFX_RAGNAROK_SHOT_STATE_STRETCH
};

void BgFxUpdateRagnarokShot() {
    u16 frame;
    u16 frameTimer;
    s16 alpha;

    BgAnimGetFrameState(&frame, &frameTimer);

    switch (frame) {
    case 4:
        ApplyAttackBox(91, sBgFx->x + sBgFx->scaleX * 40, sBgFx->y, sBgFx->z, 32, 32, 50);
        break;
    case 5:
        if (frameTimer == 0) {
            sBgFx->state = BGFX_RAGNAROK_SHOT_STATE_FLY;
        }

        break;
    case 6:
        if (frameTimer == 0) {
            sBgFx->state = BGFX_RAGNAROK_SHOT_STATE_STRETCH;
        }

        break;
    }

    switch (sBgFx->state) {
    case BGFX_RAGNAROK_SHOT_STATE_FLY:
        sBgFx->unk_3C += 51;

        if (sBgFx->flags & BGFX_FLAG_FLIP_X) {
            sBgFx->x -= sBgFx->unk_3C;
        } else {
            sBgFx->x += sBgFx->unk_3C;
        }

        break;
    case BGFX_RAGNAROK_SHOT_STATE_STRETCH:
        sBgFx->unk_3C += 51;

        if (sBgFx->flags & BGFX_FLAG_FLIP_X) {
            sBgFx->x -= sBgFx->unk_3C;
            sBgFx->scaleX -= Q_8_8(0.2);
        } else {
            sBgFx->x += sBgFx->unk_3C;
            sBgFx->scaleX += Q_8_8(0.2);
        }

        ApplyAttackBox(91, sBgFx->x + sBgFx->scaleX * 40, sBgFx->y, sBgFx->z, 32, 32, 50);

        if (sBgFx->timer > 20) {
            alpha = 36 - sBgFx->timer;
            SetBlendAlpha(16, alpha);

            if (alpha <= 0) {
                BgAnimStop();
            }
        }

        sBgFx->timer++;
        break;
    }

    BgFxUpdateBase();
}

void BgFxStartRagnarokShot(s32 x, s32 y, s32 z, u8 flip) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    WorldToScreen(&sx, &sy, x, y, z);
    sBgFx->state = BGFX_RAGNAROK_SHOT_STATE_HOLD;

    if (flip) {
        sBgFx->flags |= BGFX_FLAG_FLIP_X;
        sBgFx->scaleX = Q_8_8(-1);
    }

    sBgFx->unk_3C = 0;
    m4aSongNumStart(SONG_EF_RAGNA02);
    BgAnimStart(&gBgAnimDefRagnarokShot, sx, sy);
    BgAnimSetLoopStartFrame(6);
    sBgFx->update = BgFxUpdateRagnarokShot;
}

void BgFxStartGlow(s32 x, s32 y, s32 z, s32 scale) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(2)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefGlow, sx, sy);
    sBgFx->scaleX = scale;
    sBgFx->scaleY = scale;
    sBgFx->update = BgFxUpdateFadeOut;
}

BtlObj* BgFxGetSyncTarget() {
    if (gBtlWork->flags & BTL_FLAG_VS_BATTLE) {
        if (gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION) {
            return gBtlWork->actor;
        }

        return gRikuBtlWork->actor;
    }

    if (gBtlWork->actor2 != NULL) {
        return gBtlWork->actor2;
    }

    return ListPoolFirst(&gBtlWork->pool);
}

void BgFxApplySyncHp(s16 hp) {
    BtlObj* obj;

    if (gBtlWork->flags & BTL_FLAG_VS_BATTLE) {
        if (GetRandom() % 5) {
            if (gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION) {
                CreateBtlPopTask(gBtlWork->actor, BTL_POP_MISS);
            } else {
                CreateBtlPopTask(gRikuBtlWork->actor, BTL_POP_MISS);
            }
        } else {
            obj = gRikuBtlWork->actor;
            obj->hp = hp;

            if (hp > obj->maxHp) {
                obj->hp = obj->maxHp;
            }

            obj = gBtlWork->actor;
            obj->hp = hp;

            if (hp > obj->maxHp) {
                obj->hp = obj->maxHp;
            }
        }
    } else {
        obj = ListPoolFirst(&gBtlWork->pool);

        while (obj != NULL) {
            if (obj->flags & BTLOBJ_FLAG_BOSS) {
                CreateBtlPopTask(obj, BTL_POP_GUARD);
            } else {
                obj->hp = hp;

                if (hp > obj->maxHp) {
                    obj->hp = obj->maxHp;
                }
            }

            obj = ListPoolNext(&obj->node);
        }
    }
}

enum BgFxSyncState {
    BGFX_SYNC_STATE_RISE,
    BGFX_SYNC_STATE_HOVER,
    BGFX_SYNC_STATE_APPROACH,
    BGFX_SYNC_STATE_BURST
};

void BgFxUpdateSync() {
    BtlObj* target;

    switch (sBgFx->state) {
    case BGFX_SYNC_STATE_RISE:
        sBgFx->scaleX = (SIN((u16)sBgFx->timer * 4) >> 3) + Q_8_8(0.35);

        if (sBgFx->steps > 0) {
            ApproachValueHalfSteps(&sBgFx->z, sBgFx->targetZ - 0x2000, sBgFx->steps);
            sBgFx->steps--;
        } else {
            sBgFx->state = BGFX_SYNC_STATE_HOVER;
            sBgFx->steps = 60;
        }

        break;
    case BGFX_SYNC_STATE_HOVER:
        sBgFx->scaleX = (SIN((u16)sBgFx->timer * 4) >> 3) + Q_8_8(0.35);
        sBgFx->steps--;

        if (sBgFx->steps <= 0) {
            sBgFx->state = BGFX_SYNC_STATE_APPROACH;
            sBgFx->steps = 50;
        }

        break;
    case BGFX_SYNC_STATE_APPROACH:
        sBgFx->scaleX = (SIN((u16)sBgFx->timer * 4) >> 3) + Q_8_8(0.35);
        target = BgFxGetSyncTarget();

        if (target != NULL) {
            ApproachValueHalfSteps(&sBgFx->x, target->x, sBgFx->steps);
            ApproachValueHalfSteps(&sBgFx->y, target->y, sBgFx->steps);
            ApproachValueHalfSteps(&sBgFx->z, target->z - (target->centerHeight << 8), sBgFx->steps);
        }

        sBgFx->steps--;

        if (target != NULL) {
            if (sBgFx->steps > 0) {
                break;
            }

            BgFxApplySyncHp(target->hp);
        }

        sBgFx->state = BGFX_SYNC_STATE_BURST;
        m4aSongNumStart(SONG_EF_SYNC2);
        sBgFx->steps = 16;
        break;
    case BGFX_SYNC_STATE_BURST:
        sBgFx->scaleX += Q_8_8(0.65);
        sBgFx->scaleY = sBgFx->scaleX;
        SetBlendAlpha(16, sBgFx->steps);
        sBgFx->steps--;

        if (sBgFx->steps <= 0) {
            BgAnimStop();
        }

        break;
    }

    sBgFx->scaleY = sBgFx->scaleX;
    sBgFx->timer++;
    BgFxUpdateBase();
}

void BgFxStartSync(s32 x, s32 y, s32 z) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->targetZ = z;
    sBgFx->state = BGFX_SYNC_STATE_RISE;
    sBgFx->steps = 50;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefGlow, sx, sy);
    sBgFx->scaleX = Q_8_8(0.35);
    sBgFx->scaleY = Q_8_8(0.35);
    sBgFx->update = BgFxUpdateSync;
    m4aSongNumStart(SONG_EF_SYNC1);
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

void BgFxStartStunImpact(s32 x, s32 y, s32 z) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefStunImpact, sx, sy);
    sBgFx->scaleX = Q_8_8(2);
    sBgFx->scaleY = Q_8_8(2);
    sBgFx->update = BgFxUpdateBase;
}

void BgFxUpdateZantetsuken() {
    sBgFx->scaleX += sBgFx->targetX;
    sBgFx->scaleY += sBgFx->targetY;
    BgFxUpdateBase();

    if (sBgFx->timer > 5) {
        s16 fade = sBgFx->timer - 5;
        SetBlendAlpha(16, 16 - fade);

        if (fade > 15) {
            BgAnimStop();
        }
    }

    sBgFx->timer++;
}

void BgFxStartZantetsuken(s32 x, s32 y, s32 z, u8 facingRight) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefGlow, sx, sy);
    sBgFx->scaleX = Q_8_8(0.15);
    sBgFx->scaleY = Q_8_8(1);
    sBgFx->targetX = 7;
    sBgFx->targetY = Q_8_8(0.6);

    if (facingRight) {
        sBgFx->angle += 40;
    } else {
        sBgFx->angle -= 40;
    }

    sBgFx->update = BgFxUpdateZantetsuken;
}

enum BgFxUrsulaBeamState {
    BGFX_URSULA_BEAM_STATE_GROW,
    BGFX_URSULA_BEAM_STATE_HOLD,
    BGFX_URSULA_BEAM_STATE_FADE_OUT
};

void BgFxUpdateUrsulaBeam() {
    switch (sBgFx->state) {
    case BGFX_URSULA_BEAM_STATE_GROW:
        SetBlendAlpha(16, (sBgFx->timer >> 1) + 8);
        ApproachValueHalfSteps(&sBgFx->scaleX, sBgFx->targetX, 17 - sBgFx->timer);
        ApproachValueHalfSteps(&sBgFx->scaleY, sBgFx->targetY, 17 - sBgFx->timer);

        if (sBgFx->timer > 15) {
            sBgFx->timer = 0;
            sBgFx->state = BGFX_URSULA_BEAM_STATE_HOLD;
        } else {
            sBgFx->timer++;
        }

        break;
    case BGFX_URSULA_BEAM_STATE_HOLD:
        if (sBgFx->timer > sBgFx->steps) {
            sBgFx->timer = 0;
            sBgFx->state = BGFX_URSULA_BEAM_STATE_FADE_OUT;
        } else {
            sBgFx->timer++;
        }

        break;
    case BGFX_URSULA_BEAM_STATE_FADE_OUT:
        SetBlendAlpha(16, 16 - sBgFx->timer);

        if (sBgFx->timer > 15) {
            BgAnimStop();
        } else {
            sBgFx->timer++;
        }

        break;
    }

    BgFxUpdateBase();
}

void BgFxStartUrsulaBeam(s32 x, s32 y, s32 z, u8 facingLeft, s32 targetScale, u16 steps) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->steps = steps;
    sBgFx->state = BGFX_URSULA_BEAM_STATE_GROW;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefUrsulaBeam, sx, sy);

    if (facingLeft) {
        sBgFx->targetX = targetScale;
        sBgFx->scaleX = Q_8_8(0.3);
    } else {
        sBgFx->targetX = -targetScale;
        sBgFx->scaleX = Q_8_8(-0.3);
    }

    sBgFx->targetY = targetScale;
    sBgFx->scaleY = Q_8_8(0.3);
    m4aSongNumStart(SONG_EF_UR_BEEM);
    sBgFx->update = BgFxUpdateUrsulaBeam;
    BgAnimSetLoopStartFrame(0);
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

void BgFxStartAnsemRush(s32 x, s32 y, s32 z, u8 flip) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->state = BGFX_FADE_IN_OUT_STATE_FADE_IN;
    sBgFx->steps = 45;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefAnsemRush, sx, sy);

    if (flip) {
        sBgFx->scaleX = Q_8_8(-1);
    } else {
        sBgFx->scaleX = Q_8_8(1);
    }

    sBgFx->scaleY = Q_8_8(1);
    sBgFx->update = BgFxUpdateFadeInOut;
    BgAnimSetLoopStartFrame(0);
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

void BgFxUpdateAnsemWave() {
    if (sBgFx->flags & BGFX_FLAG_FLIP_X) {
        sBgFx->x += 0x700;
    } else {
        sBgFx->x += -0x700;
    }

    if (sBgFx->x < (gBtlWork->xMin - 0x40) << 8 || sBgFx->x > (gBtlWork->xMax + 0x40) << 8) {
        BgAnimStop();
    } else if (ApplyAttackBox(sBgFx->attack, sBgFx->x, sBgFx->y, sBgFx->z, 16, 16, 48)) {
        m4aSongNumStart(SONG_BTL_AN_WAVEHIT);
    }

    BgFxUpdateBase();
}

void BgFxStartAnsemWave(s32 x, s32 y, s32 z, u8 facingLeft, s32 attack) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->attack = attack;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefAnsemWave, sx, sy);

    if (facingLeft) {
        sBgFx->scaleX = Q_8_8(1);
    } else {
        sBgFx->scaleX = Q_8_8(-1);
        sBgFx->flags |= BGFX_FLAG_FLIP_X;
    }

    sBgFx->scaleY = Q_8_8(1);
    m4aSongNumStart(SONG_EF_AN_WAVE);
    sBgFx->update = BgFxUpdateAnsemWave;
    BgAnimSetLoopStartFrame(0);
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

void func_08016BCC(s32 x, s32 y) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(2)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    WorldToScreen(&sx, &sy, x, y, 0);
    BgAnimStart(&gBgAnimDefFriendHit, sx, sy);
    sBgFx->scaleX = Q_8_8(9);
    sBgFx->scaleY = Q_8_8(9);
    sBgFx->update = BgFxUpdateBase;
}

void BgFxStartJfMajinBeam(s32 x, s32 y, s32 z, s32 scaleY, u8 angle, u16 steps) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->steps = steps;
    sBgFx->state = BGFX_FADE_IN_OUT_STATE_FADE_IN;
    sBgFx->angle = angle;
    WorldToScreen(&sx, &sy, sBgFx->x, sBgFx->y, sBgFx->z);
    BgAnimStart(&gBgAnimDefJfMajinBeam, sx, sy);
    sBgFx->scaleY = scaleY;
    sBgFx->update = BgFxUpdateFadeInOut;
    BgAnimSetLoopStartFrame(0);
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

void BgFxUpdateFireBurst() {
    u16 frame;
    u16 frameTimer;
    u16 angle;
    s16 sx;
    s16 sy;
    s32 dx;

    BgAnimGetFrameState(&frame, &frameTimer);
    dx = 0;

    if (sBgFx->timer > 0) {
        if (frame > 7) {
            angle = sBgFx->angle;

            if (sBgFx->flags & BGFX_FLAG_FLIP_X) {
                ApproachAngle(&angle, GetAngle(sBgFx->x, sBgFx->y,
                                               sBgFx->targetX, sBgFx->targetY) + 64, 5);
            } else {
                ApproachAngle(&angle, GetAngle(sBgFx->x, sBgFx->y,
                                               sBgFx->targetX, sBgFx->targetY) - 64, 5);
            }

            sBgFx->angle = angle;
            ApproachValue(&sBgFx->x, sBgFx->targetX, sBgFx->timer);
            ApproachValue(&sBgFx->y, sBgFx->targetY, sBgFx->timer);
            ApproachValue(&sBgFx->z, sBgFx->targetZ, sBgFx->timer);

            if (TestAttackBox(sBgFx->x, sBgFx->y, sBgFx->z,
                              8, 8, 16)) {
                sBgFx->timer = -1;
            } else {
                sBgFx->timer--;
            }
        } else if (frame > 2) {
            if (sBgFx->flags & BGFX_FLAG_FLIP_X) {
                dx = ((7 - frame) << 8) * 7;
            } else {
                s32 framesLeft = 7 - frame;
                framesLeft *= 256;
                dx = framesLeft * -7;
            }

            if (TestAttackBox(sBgFx->x + dx, sBgFx->y,
                              sBgFx->z, 8, 8, 16)) {
                sBgFx->timer = -1;
            }
        }
    }

    switch (sBgFx->timer) {
    case 0:
        sBgFx->update = BgFxUpdateFadeOut;
        break;
    case -1:
        m4aSongNumStart(SONG_EF_DRHEET);
        SetBlendAlpha(16, 11);
        sBgFx->angle = 0;
        sBgFx->scaleX = Q_8_8(5);
        sBgFx->scaleY = Q_8_8(5);
        sBgFx->x += dx;
        sBgFx->releaseFrames = 20;
        WorldToScreen(&sx, &sy, sBgFx->x, sBgFx->y,
                      sBgFx->z);
        BgAnimStart(&gBgAnimDefExplosion, sx, sy);
        m4aSongNumStart(SONG_EF_FIRE03);
        sBgFx->timer = -2;
        sBgFx->steps = 0;
        break;
    case -2:
        if (sBgFx->steps == 7) {
            ApplyAttackBox(sBgFx->attack, sBgFx->x, sBgFx->y,
                          sBgFx->z, 256, 256, 256);
        }

        sBgFx->scaleX += Q_8_8(0.2);
        sBgFx->scaleY += Q_8_8(0.2);
        sBgFx->steps++;
        break;
    }

    BgFxUpdateBase();
}

void BgFxStartFireBurst(s32 x, s32 y, s32 z, s32 targetX, s32 targetY, s32 targetZ, u8 flip, s32 attack) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefFire00, sx, sy);
    m4aSongNumStart(SONG_EF_DRHEET);
    BgAnimSetLoopStartFrame(8);
    sBgFx->update = BgFxUpdateFireBurst;
    sBgFx->targetX = targetX;
    sBgFx->targetY = targetY;
    sBgFx->targetZ = targetZ;
    sBgFx->timer = 15;
    sBgFx->state = 3;
    sBgFx->attack = attack;

    if (flip) {
        sBgFx->flags |= BGFX_FLAG_FLIP_X;
        sBgFx->scaleX = Q_8_8(-1);
    }

    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

void BgFxStartFireExplosion(s32 x, s32 y, s32 z) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->scaleX = Q_8_8(1.5);
    sBgFx->scaleY = Q_8_8(1.5);
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefFire03, sx, sy);
    sBgFx->update = BgFxUpdateBase;
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

enum BgFxFullscreenState {
    BGFX_FULLSCREEN_STATE_FADE_IN,
    BGFX_FULLSCREEN_STATE_HOLD,
    BGFX_FULLSCREEN_STATE_FADE_OUT
};

void BgFxUpdateFullscreen() {
    switch (sBgFx->state) {
    case BGFX_FULLSCREEN_STATE_FADE_IN:
        SetBlendAlpha(16, sBgFx->timer);

        if (sBgFx->timer > 15) {
            sBgFx->timer = 0;
            sBgFx->state = BGFX_FULLSCREEN_STATE_HOLD;
        } else {
            sBgFx->timer++;
        }

        break;
    case BGFX_FULLSCREEN_STATE_HOLD:
        if (sBgFx->timer > sBgFx->steps) {
            sBgFx->timer = 0;
            sBgFx->state = BGFX_FULLSCREEN_STATE_FADE_OUT;
        } else {
            sBgFx->timer++;
        }

        break;
    case BGFX_FULLSCREEN_STATE_FADE_OUT:
        SetBlendAlpha(16, 16 - sBgFx->timer);

        if (sBgFx->timer > 15) {
            BgAnimStop();
            sBgFx->update = NULL;
            sBgFx->flags &= ~BGFX_FLAG_ACTIVE;
        } else {
            sBgFx->timer++;
        }

        break;
    }
}

void BgFxStartXmas(u16 steps) {
    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->steps = steps;
    sBgFx->state = BGFX_FULLSCREEN_STATE_FADE_IN;
    BgAnimStart(&gBgAnimDefXmas, 120, 80);
    sBgFx->update = BgFxUpdateFullscreen;
    BgAnimSetLoopStartFrame(0);
    sBgFx->flags |= BGFX_FLAG_ABOVE_SPRITES;
}

void BgFxUpdateVixenIceFall() {
    u16 wobble;
    s32 scaleY;
    wobble = (SIN(sBgFx->unk_0C / 3) * 10240) >> 16;
    scaleY = ((abs(gSineTable[(u8)sBgFx->unk_0C]) >> 1) + Q_8_8(1)) * Q_8_8(1.2) >> 8;
    BgAnimSetTransform(wobble + 15, Q_8_8(1.2), scaleY);
    BgFxUpdateFullscreen();
    sBgFx->unk_0C++;
}

void BgFxStartVixenIceFall(u16 steps) {
    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->steps = steps;
    sBgFx->unk_0C = 0;
    sBgFx->state = BGFX_FULLSCREEN_STATE_FADE_IN;
    BgAnimStart(&gBgAnimDefVixenIceFall, 120, 80);
    BgAnimSetTransform(10, Q_8_8(1.2), Q_8_8(1.2));
    sBgFx->update = BgFxUpdateVixenIceFall;
    BgAnimSetLoopStartFrame(0);
    sBgFx->flags |= BGFX_FLAG_ABOVE_SPRITES;
}

void BgFxStartFlame(s32 x, s32 y, s32 z, s32 scale) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->scaleX = scale;
    sBgFx->scaleY = scale;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefFlame, sx, sy);
    sBgFx->update = BgFxUpdateBase;
    BgAnimSetLoopStartFrame(0);
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

void BgFxStartFrost(s32 x, s32 y, s32 z, s32 scale) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->scaleX = scale;
    sBgFx->scaleY = scale;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefFrost, sx, sy);
    sBgFx->update = BgFxUpdateBase;
    BgAnimSetLoopStartFrame(0);
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

void BgFxStartUrsulaThunder(s32 x, s32 y, s32 z) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefUrsulaThunder, sx, sy);
    sBgFx->update = BgFxUpdateBase;
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
}

enum BgFxHolyState {
    BGFX_HOLY_STATE_FADE_IN,
    BGFX_HOLY_STATE_ATTACK,
    BGFX_HOLY_STATE_FADE_OUT
};

void BgFxUpdateHoly() {
    if (sBgFx->unk_0C > 0) {
        ApproachValueHalfSteps(&sBgFx->scaleX, Q_8_8(2), sBgFx->unk_0C);
        sBgFx->unk_0C--;
    }

    if (sBgFx->scaleYSteps > 0) {
        ApproachValue(&sBgFx->scaleY, Q_8_8(1.5), sBgFx->scaleYSteps);
        sBgFx->scaleYSteps--;
    }

    switch (sBgFx->state) {
    case BGFX_HOLY_STATE_FADE_IN: {
        u16 timer;

        SetBlendAlpha(16, sBgFx->timer);
        timer = sBgFx->timer;

        if ((s16)timer > 15) {
            sBgFx->timer = 0;
            sBgFx->state = BGFX_HOLY_STATE_ATTACK;
        } else {
            sBgFx->timer = timer + 1;
        }

        break;
    }
    case BGFX_HOLY_STATE_ATTACK: {
        u16 timer;

        ApplyAttackBox(sBgFx->attack, sBgFx->x, sBgFx->y, sBgFx->z, 32, 16, 256);
        timer = sBgFx->timer;

        if ((s16)timer > sBgFx->steps) {
            sBgFx->timer = 0;
            sBgFx->state = BGFX_HOLY_STATE_FADE_OUT;
        } else {
            sBgFx->timer = timer + 1;
        }

        break;
    }
    case BGFX_HOLY_STATE_FADE_OUT: {
        u16 fade = sBgFx->timer;
        u16 timer;

        SetBlendAlpha(16, 16 - ((s16)fade >> 1));
        ApproachValue(&sBgFx->scaleX, 10, 33 - sBgFx->timer);
        timer = sBgFx->timer;

        if ((s16)timer > 31) {
            BgAnimStop();
        } else {
            sBgFx->timer = timer + 1;
        }

        break;
    }
    }

    BgFxUpdateBase();
}

void BgFxStartHoly(s32 x, s32 y, s32 z, s32 attack) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->steps = 120;
    sBgFx->unk_0C = 60;
    sBgFx->scaleYSteps = 20;
    sBgFx->state = BGFX_HOLY_STATE_FADE_IN;
    sBgFx->attack = attack;
    sBgFx->scaleX = 10;
    sBgFx->scaleY = 10;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefLightPillar, sx, sy);
    sBgFx->update = BgFxUpdateHoly;
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

void BgFxTornadoLiftBtlObj(BtlObj* source, BtlObj* target, u8 spin, u8 init) {
    s32 liftZ;
    s32 dx;
    s32 dy;
    s32 radius;
    s32 orbitX;
    s32 orbitY;

    if (init) {
        target->angle = GetAngle(source->x, source->y, target->x, target->y);
        target->knockbackSpeed = 0;
    }

    dx = target->x - sBgFx->x;

    if (dx >= 0 ? dx <= 0x4FFF : sBgFx->x - target->x <= 0x4FFF) {
        dy = target->y - sBgFx->y;

        if (dy >= 0 ? dy <= 0x27FF : sBgFx->y - target->y <= 0x27FF) {
            if (sBgFx->flags & BGFX_FLAG_FLIP_X) {
                liftZ = target->knockbackSpeed - ((sBgFx->x - target->x) >> 1);
            } else {
                liftZ = target->knockbackSpeed + ((sBgFx->x - target->x) >> 1);
            }

            if (liftZ > 0) {
                liftZ = 0;
            }

            radius = -(liftZ >> 9);
            orbitX = sBgFx->x + SIN(target->angle + spin) * (s16)radius;
            orbitY = sBgFx->y + -COS(target->angle + spin) * ((s16)radius >> 1);

            if (target->x < orbitX) {
                target->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
            } else {
                target->flags |= BTLOBJ_FLAG_FACING_LEFT;
            }

            target->x += (orbitX - target->x) >> 3;
            target->y += (orbitY - target->y) >> 3;
            target->z += (liftZ - target->z) >> 2;
            target->knockbackSpeed -= 110;
        }
    }
}

void BgFxTornadoLiftOpponents(u8 spin, u8 init) {
    BtlObj* source;
    BtlObj* target;

    if (gBtlWork->flags & BTL_FLAG_VS_BATTLE) {
        if (gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION) {
            source = gBtlWork->actor;
            target = gRikuBtlWork->actor;
        } else {
            source = gRikuBtlWork->actor;
            target = gBtlWork->actor;
        }

        BgFxTornadoLiftBtlObj(source, target, spin, init);
    } else {
        source = gBtlWork->actor;
        target = ListPoolFirst(&gBtlWork->pool);

        while (target != NULL) {
            if (!(target->flags & BTLOBJ_FLAG_BOSS) && target->kind != ENEMY_CREEPER_PLANT) {
                BgFxTornadoLiftBtlObj(source, target, spin, init);
            }

            target = ListPoolNext(&target->node);
        }
    }
}

enum BgFxTornadoState {
    BGFX_TORNADO_STATE_GROW,
    BGFX_TORNADO_STATE_LIFT,
    BGFX_TORNADO_STATE_FADE_OUT
};

void BgFxUpdateTornado() {
    u16 alpha;
    s16 v;
    u32 timer;

    switch (sBgFx->state) {
    case BGFX_TORNADO_STATE_GROW:
        v = 17 - sBgFx->timer;

        if (sBgFx->flags & BGFX_FLAG_FLIP_X) {
            ApproachValueHalfSteps(&sBgFx->scaleX, Q_8_8(-1), v);
        } else {
            ApproachValueHalfSteps(&sBgFx->scaleX, Q_8_8(1), v);
        }

        ApproachValueHalfSteps(&sBgFx->scaleY, Q_8_8(1), v);
        SetBlendAlpha(16, sBgFx->timer);

        if (sBgFx->timer > 15) {
            sBgFx->timer = 0;
            sBgFx->state = BGFX_TORNADO_STATE_LIFT;
        } else {
            sBgFx->timer++;
        }

        break;
    case BGFX_TORNADO_STATE_LIFT:
        timer = (u16)sBgFx->timer;
        sBgFx->scaleY = abs(gSineTable[(u8)sBgFx->timer] >> 1) + Q_8_8(1);

        if (sBgFx->flags & BGFX_FLAG_FLIP_X) {
            if ((u16)timer == 0) {
                BgFxTornadoLiftOpponents(-timer * 8, TRUE);
            } else {
                BgFxTornadoLiftOpponents(-timer * 8, FALSE);
            }
        } else {
            if ((u16)timer == 0) {
                BgFxTornadoLiftOpponents(timer * 8, TRUE);
            } else {
                BgFxTornadoLiftOpponents(timer * 8, FALSE);
            }
        }

        if (sBgFx->timer > sBgFx->steps) {
            sBgFx->timer = 0;
            sBgFx->state = BGFX_TORNADO_STATE_FADE_OUT;
            ApplyAttackBox(sBgFx->attack, sBgFx->x, sBgFx->y,
                          sBgFx->z, 40, 20, 256);
        } else {
            sBgFx->timer++;
        }

        break;
    case BGFX_TORNADO_STATE_FADE_OUT:
        alpha = 16;
        v = sBgFx->timer;
        timer = v;
        alpha -= timer;
        SetBlendAlpha(16, alpha);

        if (sBgFx->flags & BGFX_FLAG_FLIP_X) {
            ApproachValue(&sBgFx->scaleX, -10, 17 - timer);
        } else {
            ApproachValue(&sBgFx->scaleX, 10, 17 - timer);
        }

        ApproachValue(&sBgFx->scaleY, Q_8_8(3), 17 - v);

        if (v > 15) {
            BgAnimStop();
        } else {
            sBgFx->timer++;
        }

        break;
    }

    if (sBgFx->flags & BGFX_FLAG_FLIP_X) {
        sBgFx->x -= 102;
    } else {
        sBgFx->x += 102;
    }

    sBgFx->unk_3C = 0;
    ApplyBattleBounds(&sBgFx->x, &sBgFx->y, &sBgFx->z,
                  &sBgFx->unk_3C);
    ClampBattlePosition(&sBgFx->x, &sBgFx->y, -16, 0);
    BgFxUpdateBase();
}

void BgFxStartTornado(s32 x, s32 y, s32 z, s32 attack, u8 flip) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->steps = 220;
    sBgFx->unk_0C = 0;
    sBgFx->state = BGFX_TORNADO_STATE_GROW;
    sBgFx->attack = attack;
    sBgFx->scaleY = 10;

    if (flip) {
        sBgFx->scaleX = -10;
        sBgFx->flags |= BGFX_FLAG_FLIP_X;
    } else {
        sBgFx->scaleX = 10;
    }

    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefTornado, sx, sy);
    sBgFx->update = BgFxUpdateTornado;
    BgAnimSetLoopStartFrame(0);
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

enum BgFxBindState {
    BGFX_BIND_STATE_WIDEN,
    BGFX_BIND_STATE_FLASH,
    BGFX_BIND_STATE_FADE_OUT
};

void BgFxUpdateBind() {
    s16 timer;
    s16 fade;

    switch (sBgFx->state) {
    case BGFX_BIND_STATE_WIDEN:
        sBgFx->scaleX += Q_8_8(0.5);

        if (sBgFx->timer > 60) {
            sBgFx->timer = 0;
            sBgFx->state = BGFX_BIND_STATE_FLASH;
        } else {
            sBgFx->timer++;
        }

        break;
    case BGFX_BIND_STATE_FLASH:
        timer = sBgFx->timer;
        SetBlendAlpha(16, timer + 8);

        if (timer > 7) {
#ifdef VERSION_EU
            ApplyAttackBox(sBgFx->attack, sBgFx->x, sBgFx->y + 0x1000, 0, 0x100, 0x100, 0x100);
#else
            ApplyAttackBox(sBgFx->attack, sBgFx->x, sBgFx->y + 0x1000, 0, 0x100, 0x100, 8);
#endif
            sBgFx->timer = 0;
            sBgFx->state = BGFX_BIND_STATE_FADE_OUT;
        } else {
            sBgFx->timer++;
        }

        break;
    case BGFX_BIND_STATE_FADE_OUT:
        fade = sBgFx->timer >> 2;
        SetBlendAlpha(16, 16 - fade);

        if (fade > 15) {
            BgAnimStop();
        } else {
            sBgFx->timer++;
        }

        break;
    }

    BgFxUpdateBase();
}

void BgFxStartBind(s32 x, s32 attack) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = gBtlWork->yMin << 8;
    sBgFx->z = 0;
    sBgFx->attack = attack;
    sBgFx->scaleX = 10;
    sBgFx->scaleY = -((gBtlWork->yMax - gBtlWork->yMin) << 8) / 96;
    WorldToScreen(&sx, &sy, x, sBgFx->y, 0);
    BgAnimStart(&gBgAnimDefLightPillar, sx, sy);
    sBgFx->flags |= BGFX_FLAG_BELOW_SPRITES;
    sBgFx->state = BGFX_BIND_STATE_WIDEN;
    SetBlendAlpha(16, 8);
    sBgFx->update = BgFxUpdateBind;
    BgAnimSetLoopStartFrame(0);
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

enum BgFxAxcelFireWallState {
    BGFX_AXCEL_FIRE_WALL_STATE_ADVANCE,
    BGFX_AXCEL_FIRE_WALL_STATE_BURN,
    BGFX_AXCEL_FIRE_WALL_STATE_FADE_OUT
};

void BgFxUpdateAxcelFireWall() {
    switch (sBgFx->state) {
    case BGFX_AXCEL_FIRE_WALL_STATE_ADVANCE:
        ApproachValue(&sBgFx->x, sBgFx->targetX, sBgFx->steps);

        if (sBgFx->flags & BGFX_FLAG_FLIP_X) {
            ApproachValue(&sBgFx->scaleX, Q_8_8(-1), sBgFx->steps);
        } else {
            ApproachValue(&sBgFx->scaleX, Q_8_8(1), sBgFx->steps);
        }

        sBgFx->scaleY = abs(sBgFx->scaleX);
        sBgFx->steps--;

        if (sBgFx->steps <= 0) {
            sBgFx->state = BGFX_AXCEL_FIRE_WALL_STATE_BURN;
            sBgFx->steps = 16;
        }

        break;
    case BGFX_AXCEL_FIRE_WALL_STATE_BURN:
        if (sBgFx->flags & BGFX_FLAG_FLIP_X) {
            if (ApplyAttackBox(sBgFx->attack, sBgFx->x - 0x1000, sBgFx->y + 0x2000, 0, 20, 32, 64) || ApplyAttackBox(sBgFx->attack, sBgFx->x + 0x1000, sBgFx->y - 0x2000, 0, 20, 32, 64)) {
                sBgFx->state = BGFX_AXCEL_FIRE_WALL_STATE_FADE_OUT;
                m4aSongNumStart(SONG_EF_FIRE01);
            }
        } else if (ApplyAttackBox(sBgFx->attack, sBgFx->x - 0x1000, sBgFx->y - 0x2000, 0, 20, 32, 64)) {
            sBgFx->state = BGFX_AXCEL_FIRE_WALL_STATE_FADE_OUT;
            m4aSongNumStart(SONG_EF_FIRE01);
        } else if (ApplyAttackBox(sBgFx->attack, sBgFx->x + 0x1000, sBgFx->y + 0x2000, 0, 20, 32, 64)) {
            sBgFx->state = BGFX_AXCEL_FIRE_WALL_STATE_FADE_OUT;
            m4aSongNumStart(SONG_EF_FIRE01);
        }

        if (sBgFx->flags & BGFX_FLAG_FLIP_X) {
            if (gBtlWork->actor->x < sBgFx->x) {
                sBgFx->state = BGFX_AXCEL_FIRE_WALL_STATE_FADE_OUT;
            }
        } else {
            if (gBtlWork->actor->x > sBgFx->x) {
                sBgFx->state = BGFX_AXCEL_FIRE_WALL_STATE_FADE_OUT;
            }
        }

        break;
    case BGFX_AXCEL_FIRE_WALL_STATE_FADE_OUT:
        SetBlendAlpha(16, sBgFx->steps);

        if (sBgFx->steps <= 0) {
            BgAnimStop();
        } else {
            sBgFx->steps--;
        }

        break;
    }

    BgFxUpdateBase();
}

void BgFxStartAxcelFireWall(s32 x, u8 facingLeft, s32 attack) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->state = BGFX_AXCEL_FIRE_WALL_STATE_ADVANCE;
    sBgFx->x = x;
    sBgFx->y = (gBtlWork->yMin + gBtlWork->yMax) << 7;
    sBgFx->z = 0;
    sBgFx->attack = attack;
    sBgFx->steps = 20;
    WorldToScreen(&sx, &sy, sBgFx->x, sBgFx->y, 0);
    BgAnimStart(&gBgAnimDefAxcelFireWall, sx, sy);

    if (facingLeft) {
        sBgFx->scaleX = 10;
        sBgFx->targetX = x - 0x3700;
    } else {
        sBgFx->scaleX = -10;
        sBgFx->targetX = x + 0x3700;
        sBgFx->flags |= BGFX_FLAG_FLIP_X;
    }

    sBgFx->scaleY = 10;
    sBgFx->flags |= BGFX_FLAG_BELOW_SPRITES;
    sBgFx->update = BgFxUpdateAxcelFireWall;
    BgAnimSetLoopStartFrame(0);
}

void BgFxUpdateGround() {
    u16 frame;
    u16 halfSize;
    ApproachValue(&sBgFx->scaleX, Q_8_8(3), sBgFx->steps);
    sBgFx->scaleY = sBgFx->scaleX;
    sBgFx->steps--;
    BgAnimGetFrameState(&frame, NULL);

    if (frame <= 4) {
        halfSize = (sBgFx->scaleX * 5) >> 5;

        if (ApplyAttackBox(0x13D, sBgFx->x, sBgFx->y, sBgFx->z, halfSize, (s16)halfSize >> 1, 1)) {
            m4aSongNumStart(SONG_BTL_MARL_GROUNDHIT);
        }
    }

    BgFxUpdateBase();
}

void BgFxStartMahluxiaGround(s32 x, s32 y, s32 z, s32 attack) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->attack = attack;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefMahluxiaGround, sx, sy);
    sBgFx->scaleX = Q_8_8(0.5);
    sBgFx->scaleY = Q_8_8(0.5);
    sBgFx->steps = BgAnimGetDuration(BgAnimGetCurrent());
    sBgFx->flags |= BGFX_FLAG_BELOW_SPRITES;
    sBgFx->update = BgFxUpdateGround;
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

void BgFxStartLexceusGround(s32 x, s32 y, s32 z, s32 attack) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->attack = attack;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&sBgAnimDefLexceusGround, sx, sy);
    sBgFx->scaleX = Q_8_8(0.5);
    sBgFx->scaleY = Q_8_8(0.5);
    sBgFx->steps = BgAnimGetDuration(BgAnimGetCurrent());
    sBgFx->flags |= BGFX_FLAG_BELOW_SPRITES;
    sBgFx->update = BgFxUpdateGround;
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

enum BgFxHanabiraState {
    BGFX_HANABIRA_STATE_SWIRL,
    BGFX_HANABIRA_STATE_STRIKE,
    BGFX_HANABIRA_STATE_FADE_OUT
};

void BgFxUpdateHanabira() {
    switch (sBgFx->state) {
    case BGFX_HANABIRA_STATE_SWIRL:
        if (sBgFx->timer > 50) {
            sBgFx->timer = 0;
            sBgFx->state = BGFX_HANABIRA_STATE_STRIKE;
        } else {
            sBgFx->timer++;
        }

        break;
    case BGFX_HANABIRA_STATE_STRIKE:
        sBgFx->timer = 0;
        sBgFx->state = BGFX_HANABIRA_STATE_FADE_OUT;

        if (ApplyAttackBox(sBgFx->attack, sBgFx->x, sBgFx->y, sBgFx->z, 0x100, 0x100, 0x100)) {
            m4aSongNumStart(SONG_BTL_ETC_HIT05);
        }

        break;
    case BGFX_HANABIRA_STATE_FADE_OUT:
        SetBlendAlpha(16, 16 - sBgFx->timer);

        if (sBgFx->timer > 15) {
            BgAnimStop();
        } else {
            sBgFx->timer++;
        }

        break;
    }

    sBgFx->targetX += 30;
    sBgFx->angle += sBgFx->targetX >> 8;
    sBgFx->scaleX += 10;
    sBgFx->scaleY = sBgFx->scaleX;
    BgFxUpdateBase();
}

void BgFxStartHanabira(s32 x, s32 y, s32 z, s32 attack) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->attack = attack;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefHanabira, sx, sy);
    sBgFx->state = BGFX_HANABIRA_STATE_SWIRL;
    sBgFx->scaleX = 5;
    sBgFx->scaleY = 5;
    sBgFx->targetX = 256;
    sBgFx->flags |= BGFX_FLAG_ABOVE_SPRITES;
    m4aSongNumStart(SONG_EF_MARL_HANABIRA);
    sBgFx->update = BgFxUpdateHanabira;
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

enum BgFxKamaState {
    BGFX_KAMA_STATE_FADE_IN,
    BGFX_KAMA_STATE_HOLD,
    BGFX_KAMA_STATE_FADE_OUT
};

void BgFxUpdateKama() {
    if (gBtlWork->hitStop != 0) {
        BgFxUpdateBase();
        return;
    }

    if (sBgFx->flags & BGFX_FLAG_FLIP_X) {
        sBgFx->angle += 3;
    } else {
        sBgFx->angle -= 3;
    }

    sBgFx->x = sBgFx->targetX + ((gSineTable[sBgFx->angle] * sBgFx->unk_3C) >> 8);
    sBgFx->z = sBgFx->targetZ + ((-gSineTable[sBgFx->angle + 64] * sBgFx->unk_3C) >> 8);

    if (ApplyAttackBox(sBgFx->attack, sBgFx->x, sBgFx->y, sBgFx->z, 10, 5, 72)) {
        m4aSongNumStart(SONG_BTL_MARL_EFEHIT);
    }

    switch (sBgFx->state) {
    case BGFX_KAMA_STATE_FADE_IN:
        SetBlendAlpha(16, (u16)sBgFx->timer * 2);

        if (sBgFx->timer > 7) {
            sBgFx->timer = 0;
            sBgFx->state = BGFX_KAMA_STATE_HOLD;
        } else {
            sBgFx->timer++;
        }

        break;
    case BGFX_KAMA_STATE_HOLD:
        if (sBgFx->timer > 30) {
            sBgFx->timer = 0;
            sBgFx->state = BGFX_KAMA_STATE_FADE_OUT;
        } else {
            sBgFx->timer++;
        }

        break;
    case BGFX_KAMA_STATE_FADE_OUT:
        SetBlendAlpha(16, 16 - (u16)sBgFx->timer * 2);

        if (sBgFx->timer > 7) {
            BgAnimStop();
        } else {
            sBgFx->timer++;
        }

        break;
    }

    BgFxUpdateBase();
}

void BgFxStartKama(s32 x, s32 y, s32 z, s32 dx, s32 attack) {
    s16 sx;
    s16 sy;
    s32 radius;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->attack = attack;

    if (dx > 0) {
        sBgFx->scaleX = Q_8_8(-1.5);
        sBgFx->flags |= BGFX_FLAG_FLIP_X;
        x -= 0x4000;
        dx += 0x4000;
    } else {
        sBgFx->scaleX = Q_8_8(1.5);
        x += 0x4000;
        dx -= 0x4000;
    }

    sBgFx->targetX = x;
    sBgFx->targetZ = z;
    radius = abs(dx);
    sBgFx->unk_3C = radius;
    sBgFx->x = x + ((gSineTable[0] * radius) >> 8);
    sBgFx->z = z + ((-gSineTable[64] * radius) >> 8);
    sBgFx->y = y;
    sBgFx->scaleY = Q_8_8(1.5);
    SetBlendAlpha(16, 0);
    m4aSongNumStart(SONG_EF_MARL_KAMAEF);
    WorldToScreen(&sx, &sy, sBgFx->x, sBgFx->y, sBgFx->z);
    BgAnimStart(&gBgAnimDefKama, sx, sy);
    sBgFx->state = BGFX_KAMA_STATE_FADE_IN;
    sBgFx->update = BgFxUpdateKama;
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

void BgFxUpdateRikuLimit() {
    s16 fade = sBgFx->timer >> 1;
    SetBlendAlpha(16, 16 - fade);

    if (fade > 15) {
        BgAnimStop();
    }

    sBgFx->timer++;
    BgFxUpdateBase();
}

void BgFxStartRikuLimit(s32 x, s32 y, s32 z, u8 angle) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(3)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->angle = angle;
    WorldToScreen(&sx, &sy, sBgFx->x, sBgFx->y, sBgFx->z);
    BgAnimStart(&sBgAnimDefRikuLimit, sx, sy);
    sBgFx->flags |= BGFX_FLAG_BELOW_SPRITES;
    sBgFx->update = BgFxUpdateRikuLimit;
}

void BgFxStartDragonFire(s32 x, s32 y, s32 z, s32 scale) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->scaleX = scale;
    sBgFx->scaleY = scale;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefDragonFire, sx, sy);
    sBgFx->update = BgFxUpdateBase;
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

enum BgFxLaxeneBeamState {
    BGFX_LAXENE_BEAM_STATE_EXTEND,
    BGFX_LAXENE_BEAM_STATE_FADE_OUT
};

void BgFxUpdateLaxeneBeam() {
    s32 x;
    s32 y;
    s32 z;
    s16 reach;
    u8 angle;
    BtlObj* player;

    switch (sBgFx->state) {
    case BGFX_LAXENE_BEAM_STATE_EXTEND:
        if (sBgFx->flags & BGFX_FLAG_FLIP_X) {
            ApproachValueHalfSteps(&sBgFx->scaleX, Q_8_8(-0.8), sBgFx->steps);
        } else {
            ApproachValueHalfSteps(&sBgFx->scaleX, Q_8_8(0.8), sBgFx->steps);
        }

        sBgFx->steps--;

        if (sBgFx->steps <= 0) {
            sBgFx->timer = 0;
            sBgFx->state = BGFX_LAXENE_BEAM_STATE_FADE_OUT;
        } else {
            sBgFx->timer++;
        }

        break;
    case BGFX_LAXENE_BEAM_STATE_FADE_OUT:
        SetBlendAlpha(16, 16 - sBgFx->timer);

        if (sBgFx->timer > 15) {
            BgAnimStop();
        } else {
            sBgFx->timer++;
        }

        break;
    }

    reach = (abs(sBgFx->scaleX) * 5) >> 4;

    if (sBgFx->flags & BGFX_FLAG_FLIP_X) {
        angle = sBgFx->angle + 192;
    } else {
        angle = sBgFx->angle + 64;
    }

    x = sBgFx->x + gSineTable[angle] * reach;
    z = sBgFx->z + -gSineTable[angle + 64] * reach;
    y = sBgFx->y;
    ApplyAttackBox(sBgFx->attack, x, y, z, 32, 16, 16);
    player = gBtlWork->actor;

    if (player->flags & BTLOBJ_FLAG_HURT) {
        player->x += (x - player->x) >> 2;
        player->y += (y - player->y) >> 2;
        player->z += (z - player->z) >> 1;
    }

    BgFxUpdateBase();
}

void BgFxStartLaxeneBeam(s32 x, s32 y, s32 z, u8 flip, s32 attack) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->state = BGFX_LAXENE_BEAM_STATE_EXTEND;
    sBgFx->steps = 80;
    sBgFx->attack = attack;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefLaxeneBeam, sx, sy);

    if (flip) {
        sBgFx->flags |= BGFX_FLAG_FLIP_X;
        sBgFx->angle = 248;
    } else {
        sBgFx->angle = 8;
    }

    if (sBgFx->flags & BGFX_FLAG_FLIP_X) {
        if (sBgFx->x < gBtlWork->actor->x) {
            sBgFx->state = BGFX_LAXENE_BEAM_STATE_FADE_OUT;
        } else {
            sBgFx->scaleX = -(((sBgFx->x - gBtlWork->actor->x) << 8) / 19200);
        }
    } else {
        if (sBgFx->x > gBtlWork->actor->x) {
            sBgFx->state = BGFX_LAXENE_BEAM_STATE_FADE_OUT;
        } else {
            sBgFx->scaleX = ((gBtlWork->actor->x - sBgFx->x) << 8) / 19200;
        }
    }

    BgAnimSetLoopStartFrame(3);
    sBgFx->update = BgFxUpdateLaxeneBeam;
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

void BgFxUpdateAero() {
    s16 halfSize;

    switch (sBgFx->state) {
    case SPELL_TIER_BASE:
        sBgFx->scaleX += 2;
        sBgFx->scaleY += 2;
        break;
    case SPELL_TIER_RA:
        sBgFx->scaleX += 5;
        sBgFx->scaleY += 5;
        break;
    case SPELL_TIER_GA:
        sBgFx->scaleX += 10;
        sBgFx->scaleY += 10;
        break;
    }

    if (sBgFx->timer == 10) {
        halfSize = (sBgFx->scaleX * 3) >> 4;

        if (ApplyAttackBox(sBgFx->attack, sBgFx->x, sBgFx->y, sBgFx->z, halfSize, halfSize, 100)) {
            m4aSongNumStart(SONG_EF_DS_ANKOKUPUNCH);
        }
    }

    sBgFx->timer++;
    BgFxUpdateBase();
}

void BgFxStartAero(u16 tier, s32 x, s32 y, s32 z, s32 attack) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->attack = attack;
    sBgFx->state = tier;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefAero, sx, sy);
    sBgFx->update = BgFxUpdateAero;
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

void BgFxStartRikuDarkModeFlash(s32 x, s32 y, s32 z) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(1)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefRikuDarkModeFlash, sx, sy);
    sBgFx->update = BgFxUpdateBase;
}

void BgFxStartLstCtr(s32 x, s32 y, s32 z, s32 scale) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->scaleX = scale;
    sBgFx->scaleY = scale;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefLstCtr, sx, sy);
    sBgFx->update = BgFxUpdateBase;
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

void BgFxStartLstCtrFlipped(s32 x, s32 y, s32 z, s32 scale) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->scaleX = -scale;
    sBgFx->scaleY = scale;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefLstCtr, sx, sy);
    sBgFx->update = BgFxUpdateBase;
    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
    sBgFx->flags |= BGFX_FLAG_SCREEN_DIMMED;
}

void BgFxStartDsdTransition(s32 x, s32 y, s32 z, s32 scale) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(1)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->scaleX = scale;
    sBgFx->scaleY = scale;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefDsdTransition, sx, sy);
    sBgFx->update = BgFxUpdateBase;
}

enum BgFxRikuDarkModeState {
    BGFX_RIKU_DARK_MODE_STATE_GROW,
    BGFX_RIKU_DARK_MODE_STATE_PEAK,
    BGFX_RIKU_DARK_MODE_STATE_FADE_OUT,
    BGFX_RIKU_DARK_MODE_STATE_END
};

void BgFxUpdateRikuDarkMode() {
    switch (sBgFx->state) {
    case BGFX_RIKU_DARK_MODE_STATE_GROW:
        ApproachValue(&sBgFx->unk_3C, 0xA00, sBgFx->steps);
        ApproachValueHalfSteps(&sBgFx->scaleX, Q_8_8(1.8), sBgFx->steps);
        ApproachValueHalfSteps(&sBgFx->scaleY, Q_8_8(2), sBgFx->steps);
        SetBlendAlpha(16, sBgFx->unk_3C >> 8);
        sBgFx->steps--;

        if (sBgFx->steps <= 0) {
            sBgFx->timer = 0;
            sBgFx->state = BGFX_RIKU_DARK_MODE_STATE_PEAK;
        }

        break;
    case BGFX_RIKU_DARK_MODE_STATE_PEAK:
        sBgFx->state = BGFX_RIKU_DARK_MODE_STATE_FADE_OUT;
        sBgFx->timer = 0;
        sBgFx->steps = 25;
        break;
    case BGFX_RIKU_DARK_MODE_STATE_FADE_OUT:
        ApproachValue(&sBgFx->unk_3C, 0, sBgFx->steps);
        SetBlendAlpha(16, sBgFx->unk_3C >> 8);
        sBgFx->steps--;

        if (sBgFx->steps <= 0) {
            sBgFx->timer = 0;
            sBgFx->state = BGFX_RIKU_DARK_MODE_STATE_END;
            BgAnimStop();
        }

        break;
    }

    BgFxUpdateBase();
}

void BgFxStartRikuDarkMode(s32 x, s32 y, s32 z) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    SetBlendAlpha(16, 0);
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    sBgFx->state = BGFX_RIKU_DARK_MODE_STATE_GROW;
    sBgFx->scaleX = 10;
    sBgFx->scaleY = 10;
    sBgFx->unk_3C = 0;
    sBgFx->endSignals = 0;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&sBgAnimDefDarkGlow, sx, sy);
    BgAnimSetLoopStartFrame(0);
    sBgFx->update = BgFxUpdateRikuDarkMode;
    sBgFx->steps = 43;
}

enum BgFxRikuLimitFinishVariant {
    BGFX_RIKU_LIMIT_FINISH_VARIANT_NORMAL,
    BGFX_RIKU_LIMIT_FINISH_VARIANT_FLIPPED
};

void BgFxUpdateRikuLimitFinish() {
    if (sBgFx->timer > 10) {
        switch (sBgFx->state) {
        case BGFX_RIKU_LIMIT_FINISH_VARIANT_NORMAL:
            ApplyAttackBox(11, sBgFx->x - 0x6000, sBgFx->y - 0x1000, sBgFx->z,
                14, 14, 14);
            ApplyAttackBox(11, sBgFx->x, sBgFx->y + 0x2000, sBgFx->z,
                14, 14, 14);
            ApplyAttackBox(11, sBgFx->x + 0x7000, sBgFx->y - 0x1000, sBgFx->z,
                14, 14, 14);
            ApplyAttackBox(11, sBgFx->x - 0x3000, sBgFx->y + 0x2000, sBgFx->z,
                14, 14, 14);
            ApplyAttackBox(11, sBgFx->x + 0x3000, sBgFx->y + 0x2800, sBgFx->z,
                14, 14, 14);
            ApplyAttackBox(11, sBgFx->x - 0x6000, sBgFx->y + 0x6000, sBgFx->z,
                14, 14, 14);
            ApplyAttackBox(11, sBgFx->x + 0x800, sBgFx->y + 0x7800, sBgFx->z,
                14, 14, 14);
            ApplyAttackBox(11, sBgFx->x + 0x3000, sBgFx->y + 0x2800, sBgFx->z,
                14, 14, 14);
            ApplyAttackBox(11, sBgFx->x + 0x6800, sBgFx->y + 0x6800, sBgFx->z,
                14, 14, 14);
            break;
        case BGFX_RIKU_LIMIT_FINISH_VARIANT_FLIPPED:
            ApplyAttackBox(11, sBgFx->x + 0x6000, sBgFx->y - 0x1000, sBgFx->z,
                14, 14, 14);
            ApplyAttackBox(11, sBgFx->x, sBgFx->y + 0x2000, sBgFx->z,
                14, 14, 14);
            ApplyAttackBox(11, sBgFx->x - 0x7000, sBgFx->y - 0x1000, sBgFx->z,
                14, 14, 14);
            ApplyAttackBox(11, sBgFx->x + 0x3000, sBgFx->y + 0x2000, sBgFx->z,
                14, 14, 14);
            ApplyAttackBox(11, sBgFx->x - 0x3000, sBgFx->y + 0x2800, sBgFx->z,
                14, 14, 14);
            ApplyAttackBox(11, sBgFx->x + 0x6000, sBgFx->y + 0x6000, sBgFx->z,
                14, 14, 14);
            ApplyAttackBox(11, sBgFx->x - 0x800, sBgFx->y + 0x7800, sBgFx->z,
                14, 14, 14);
            ApplyAttackBox(11, sBgFx->x - 0x3000, sBgFx->y + 0x2800, sBgFx->z,
                14, 14, 14);
            ApplyAttackBox(11, sBgFx->x - 0x6800, sBgFx->y + 0x6800, sBgFx->z,
                14, 14, 14);
            break;
        }
    }

    sBgFx->timer++;
    BgFxUpdateBase();
}

void BgFxStartRikuLimitFinish(s32 x, s32 y, s32 z) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(0)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    m4aSongNumStart(SONG_SND_705);
    WorldToScreen(&sx, &sy, sBgFx->x, sBgFx->y, sBgFx->z);
    sBgFx->state = GetRandom() % 2;

    switch (sBgFx->state) {
    case BGFX_RIKU_LIMIT_FINISH_VARIANT_NORMAL:
        BgAnimStart(&gBgAnimDefRikuLimitFinish, sx, sy);
        break;
    case BGFX_RIKU_LIMIT_FINISH_VARIANT_FLIPPED:
        BgAnimStart(&gBgAnimDefRikuLimitFinish, sx, sy);
        sBgFx->flags |= BGFX_FLAG_FLIP_X;
        sBgFx->scaleX = Q_8_8(-1);
        break;
    }

    sBgFx->update = BgFxUpdateRikuLimitFinish;
}

void BgFxStartRikuDiveHit(s32 x, s32 y, s32 z) {
    s16 sx;
    s16 sy;

    if (BgFxIsBlocked(2)) {
        return;
    }

    BgFxReset();
    sBgFx->x = x;
    sBgFx->y = y;
    sBgFx->z = z;
    WorldToScreen(&sx, &sy, x, y, z);
    BgAnimStart(&gBgAnimDefRikuDiveHit, sx, sy);
    sBgFx->update = BgFxUpdateBase;
}
