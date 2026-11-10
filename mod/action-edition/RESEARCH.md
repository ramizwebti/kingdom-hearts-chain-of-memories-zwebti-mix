# Kingdom Hearts Chain of Memories: Zwebti Mix — research, 9 November 2026

The development base remains [ReKHCoM](https://github.com/MatyMeatBoy/ReKHCoM), rather than a fresh engine. It already supplies native rendering, widescreen, remappable controls, one-button dodge, HD menu/dialogue text, audio improvements and ROM importing. Those existing improvements are reused directly.

Targeted searches covered GitHub repositories, releases and indexed forks/issues; Nexus; Romhack Plaza/ROMhacking references; GameBanana; GBAtemp; KH-Vids; RetroAchievements; itch.io; and English/Japanese variations for cardless combat, action combat, movement, cheats and QoL. No verified publicly released GBA/ReKHCoM overhaul implementing the complete requested system was located. This does not prove no private or unindexed project exists.

| Work | What exists | Practical use here |
|---|---|---|
| [ReKHCoM](https://github.com/MatyMeatBoy/ReKHCoM) | Native GBA source port with substantial presentation and QoL work, CC0 source | Our existing foundation; reuse its renderer, controls, fonts and port infrastructure |
| [KH_COM_VITA](https://github.com/ggdrn/KH_COM_VITA) | Same GBA decompilation; one-button dodge, unstock, right-stick controls, practice features and other QoL | Useful MIT-code reference; retains card combat; many improvements already overlap our base |
| [KH1 Style Combat in ReCoM](https://www.nexusmods.com/kingdomheartsrechainofmemories/mods/84?tab=description) | Released 5 November 2026, still WIP; author advertises KH1 movement, guard, combos, command/shortcut menus, MP and progression | Closest design precedent, but targets the 3D PC Re:CoM remaster with OpenKH/Panacea. Not compatible with GBA. Modification/assets require permission; conversion is prohibited in the stated terms. No code/assets copied |
| [GBA-art KH Survivors author post](https://www.reddit.com/r/decomps/comments/1xcckf6/i_vibecoded_a_kh_chain_of_memories_survivors_game/) | Author describes a separate browser/native arena game with many Heartless, GBA sprites and spells/sleights | Crowd presentation reference only. Its performance claims do not establish limits for our engine; source and reuse permission were not verified |
| [Instant card recovery — JLHack7](https://kh-vids.net/threads/hacking-tutorial.25803/?post=827169) | Historical 2007 raw cheat makes Sora recover cards like Riku | Genuine combat-friction precedent, but retains cards and is not a native-engine module. Raw ARM ROM addresses cannot simply be patched into the PC executable |
| [Archipelago KHCOM](https://github.com/gaithernOrg/ArchipelagoKHCOM) | Public GBA randomizer source with card/world/progression and emulator-connector changes | Progression reference, not direct action combat. Repository MIT exceptions require file-by-file license checks before reuse |
| [Level 1 subset patch](https://retroachievements.org/game/768?set=40403) | Released patch prevents EXP gain, credited to DanB in the subset thread | Challenge/balance reference; not a combat-system replacement |
| [Undub — Swosho](https://romhackplaza.org/romhacks/kingdom-hearts-chain-of-memories-undub-game-boy-advance/) | Complete v1.0a, Japanese voices with corrected volume, US/EU support | Optional audio presentation work; does not change combat. Native port ROM hash validation means binary-patch compatibility is not automatic |
| [Card-as-equipment idea](https://gbatemp.net/threads/kingdom-heart-chain-of-memories-hack-idea.593114/) | 2021 proposal to turn normal cards into equipment and enemy cards into abilities | Similar ambition, but no release/source evidence in the examined thread |

A misleading result is Nexus's [Re Com with no cards hard mode](https://www.nexusmods.com/kingdomheartsrechainofmemories/mods/77?tab=posts): it targets Re:CoM, and a user reports visually identical cards rather than a replacement action system. It is not treated as a verified solution.

No third-party combat code or new sprite packs were adopted without verified applicability and reuse terms. The current overhaul reuses the game's existing authored attacks, animations and art, adds original command/control logic, and adds procedural impact feedback that does not alter Sora's appearance.

## GBA presentation and effect references — 0.5

The [Spriters Resource game catalogue](https://www.spriters-resource.com/game_boy_advance/khcom/) includes original Sora, Magic, Battle Extras, menus and text sheets. The [Sora sheet](https://www.spriters-resource.com/game_boy_advance/khcom/asset/1029/page-1/) and [Battle Extras sheet](https://www.spriters-resource.com/game_boy_advance/khcom/asset/1047/page-1/) are useful visual references. These are catalogued game rips; their presence is not permission to redistribute them as a new asset pack. No downloaded sprite sheet is bundled with this mod.

For an exact match, the in-game interface instead reads the original cartridge glyphs, load-menu background, confirm-window tiles and palettes through the existing decompilation. The new interface therefore uses the game's actual pixel artwork rather than a similar replacement font. Original Sora and spell animations remain the source of the attack poses and elemental effects. Supplemental slash, impact and defense cues use original code and a native-resolution pixel layer; imported third-party effects are not required.

The public [ReKHCoM source](https://github.com/MatyMeatBoy/ReKHCom) remains the technical foundation. Generic sprite libraries do not supply the combat's targeting, collision, recovery, effect ownership or control integration; those require changes and tests in this engine.
