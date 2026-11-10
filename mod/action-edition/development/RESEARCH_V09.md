# Kingdom Hearts Chain of Memories: Zwebti Mix — combat research

Research date: 2026-11-09. This audit covers only this recovered project's original GBA Sora implementation. It does not cover or change any other Kingdom Hearts project.

## Native move inventory

Primary implementation evidence: `src/btl/btl_sora.c` (the native move dispatcher), `include/card/card_ids.h`, `include/card/card_label_data.h`, and `include/player_progression.h`.

The recovered dispatcher contains **124 functional action cases**, plus two unused sleight labels that fall back to idle. Counts describe implemented dispatch entries, not proof that every entry works unchanged in the card-free adapter.

| Category | Callable entries | Names / coverage |
| --- | ---: | --- |
| Attack sleights | 9 | Sliding Dash, Stun Impact, Strike Raid, Blitz, Zantetsuken, Sonic Blade, Ars Arcanum, Ragnarok, Trinity Limit |
| Magic sleights | 38 | Listed below |
| Summon sleights | 16 | Ten named families, with the native paired/triple variants counted separately |
| Friend sleights | 15 | Nine named families, with the native paired/triple variants counted separately |
| Base magic | 7 | Fire, Blizzard, Thunder, Cure, Gravity, Stop, Aero |
| Base summons | 7 | Simba, Genie, Bambi, Dumbo, Tinker Bell, Mushu, Cloud |
| Base friends | 7 | Donald, Goofy, Aladdin, Ariel, Jack, Peter Pan, Beast |
| Weapons | 18 | Includes the unused `MOVE_KEYBLADE_1` entry; only 17 obtained-keyblade entries exist in progression |
| Items | 7 | Potion, Hi-Potion, Mega-Potion, Ether, Mega-Ether, Elixir, Megalixir |
| **Total** | **124** | **99 spell/sleight/summon/friend entries; 78 of these are sleights** |

The Sora progression list has **66 learned move families**: 9 attack, 38 magic, 10 summon, 9 friend. Paired/triple summon and friend variants account for the difference between 66 families and 78 callable sleight entries. Four other learned flags belong to Riku, and two are unused; none should be presented as working Sora actions.

Magic sleights:

- Fourteen higher tiers: Fira/Firaga, Blizzara/Blizzaga, Thundara/Thundaga, Cura/Curaga, Gravira/Graviga, Stopra/Stopga, Aerora/Aeroga.
- Seven raids: Fire Raid, Blizzard Raid, Thunder Raid, Gravity Raid, Stop Raid, Judgment, Reflect Raid.
- Seventeen other spells: Homing Fira, Firaga Break, Mega Flare, Homing Blizzara, Aqua Splash, Shock Impact, Tornado, Quake, Warpinator, Warp, Bind, Confuse, Terror, Synchro, Gifted Miracle, Teleport, Holy.

Summon sleight families: Proud Roar, Splash, Paradise, Idyll Romp, Flare Breath, Showtime, Twinkle, Cross-slash, Omnislash, Cross-slash+.

Friend sleight families: Magic, Blazing Donald, Goofy Charge, Goofy Tornado, Sandstorm, Surprise, Spiral Wave, Hummingbird, Ferocious Lunge.

Items that originally reload cards need a deliberate resource-system adaptation; listing them is not enough. Native friend, summon and cooperative moves also need their original actor, card/context and cleanup dependencies preserved or replaced safely.

## Existing animation variety and finishers

The original source already supplies ground swings 1/2/3, a wide first swing, aerial swings 1/2/3 and an aerial hop. Native combo setup chooses reversed opening order, a wide opener and a hop variant according to the target geometry. The recovered action adapter hardcodes the same three ground swings, discarding that authored variety. Reusing these definitions is the most immediate way to add coherent variation without foreign sprite imports.

The native Blitz implementation is multi-phase, and Sonic Blade has repeated dash phases. The recovered `ActionUpdateFinisher` replaces these with a shortened single sequence. Restore native multi-phase behavior, or reproduce its phases explicitly with protected timing and effects; a longer delay on the shortened sequence will not restore the missing choreography. The ordinary three-hit string should remain intact before a selected finisher.

The [GBA/manual scan](https://manuals.plus/m/b0160c551edcdcf0584308ddbce5e45d01c58b76de48ae5f115f7b715bcef1ed) establishes the original card/stocked-sleight context. The [KHWiki sleight reference](https://www.khwiki.com/Sleight) distinguishes GBA from Re:Chain of Memories: Blitz is a three-hit move; Sonic Blade can continue with six additional rushes. These descriptions support restoring sustained sequences. Re:CoM-only additions such as Lethal Frame, Raging Storm and Firaga Burst are potential new features, not missing native GBA Sora moves.

## Sprite research and import decision

| Source | Evidence | Decision |
| --- | --- | --- |
| [Original GBA sprite catalog](https://www.spriters-resource.com/game_boy_advance/khcom/) | Sora, magic, battle extras and seven summons are catalogued. | Reference source; native data is already present. A rip is not newly authored animation. |
| [Brave_Shyguy: KH2 Sora in CoM style](https://www.spriters-resource.com/custom_edited/kingdomheartscustoms/asset/23227/) | PNG, 600×3630; close target style, different costume. | Candidate for a later custom-art pass. Sheet-level restrictions and frame contents remain unverified; download was blocked. No art imported. |
| [Spikey-Valentine: FFRK Sora expanded](https://www.spriters-resource.com/custom_edited/kingdomheartscustoms/asset/188626/page-1/) | PNG, 350×157; another game's visual style. | Lower priority; not a drop-in replacement for CoM's directional animation system. |
| [DS Re:coded model catalog](https://models.spriters-resource.com/ds_dsi/kingdomheartsrecoded/) | 3D-model resource, rather than a matching CoM attack sprite sheet. | Requires a render/redraw and directional-animation pipeline, not a direct sprite swap. |

The [Spriters Resource terms](https://www.spriters-resource.com/page/tou/) allow credited noncommercial custom-art use subject to submission-specific restrictions. This does not establish rights to the underlying Kingdom Hearts characters or waive restrictions embedded in an individual sheet. No external sheet was cleared, copied into game assets or redistributed during this audit.

Recommended order: restore native swing variation and complete native sleight sequences; verify every exposed move's effect and cleanup; then review a specific custom sheet, its embedded credits and all directional frames before importing it. Extra screen effects, impact pauses and distinct native choreography can improve combat immediately without waiting for new art.
