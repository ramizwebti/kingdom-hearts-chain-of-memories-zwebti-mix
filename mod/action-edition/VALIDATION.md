# Kingdom Hearts Chain of Memories: Zwebti Mix — historical 0.5 validation

This records an older build. See [current validation](../../launcher/VALIDATION.md) for the delivered version; these passes do not apply to changed game images.

## Packaged build

- Guest SHA-256: `147f4cd0703045c604cc8928e89be7ad494b6e77e29e76f6c11a3ddc3fa0d893`.
- Windows game EXE: `86b425d3903b92e3dd53478068895b8590dd884e1ca21078b67b89cb40309da0`.
- Windows launcher EXE: `a4ffceeb2d757f2435a7d339c38c9a57e23d0ad1af114e3e8f40eda11d5d957b`.
- Windows executables cross-compiled successfully with version 0.5 metadata, GUI subsystems and all 10 embedded icon sizes verified against the source ICO. This is not physical Windows execution.

## Actual game checks

**33/33 native gameplay scenarios passed**, with 139 captures, on the exact unchanged game image above. These run the owned USA ROM through the Linux SDL host in isolated data folders with ordinary timed inputs; damage/invulnerability cheats and memory writes are not used.

Coverage includes three-hit ground/air chains; real earned Blitz/Air Rush contacts; deliberate misses; a one-frame attack press during measured hitstop; immediate press-to-cast and hold-without-repeat; separate spell cycling; queued casts and Attack/cycle cancellation; all seven spell releases and recoveries; aerial casting; movement and attack-to-dodge cancellation; real enemy damage under crowd pressure; all five Practice presets; retry/switch/title; active/queued-spell cleanup; and Story title/classic/Riku fallbacks.

Every one of six earned ground Blitz attacks contacted, and the following two accepted basics also contacted. Three ordinary aerial chains produced three physically airborne Air Rush contacts. The paused-input fixture measured the first hit at 191, pressed Attack at 192 during its pause, then observed the second attack accepted at 195 and contacting at 205.

The first mixed movement fixture requested a jump during real damage reaction and correctly failed to jump. The final movement fixture isolates controls against the harmless training target and requires actual attack-to-dodge cancellation. The separate mixed crowd-pressure fixture preserves real enemy attacks, observed 26 HP damage, and reached a 25-hit chain. No production damage rule was weakened to pass a fixture.

The seven spell scenarios verify selected command, authored release at 14 and recovery at 20. **Seven additional native spell-impact cases passed** against ordinary mixed enemies using only spell inputs. A read-only observer sampled bounded game fields; it did not write memory or alter combat. Fire, Blizzard, Thunder and Aero each dealt real damage after Sora had recovered. Gravity applied its native pending state and reduced enemy HP afterward. Stop applied its actual status and timer. Cure restored naturally wounded Sora from 554 to 560 at release, twice. The report records the observed frames, source-layout offsets and sampling limits. These observations are separate from the 33 main logic cases.

## Measured combat continuity

Identical 700-frame training-target attack input was run against 0.4 and 0.5 without cheats. These controlled measurements describe that fixture, not campaign-wide performance or difficulty.

| Measurement | 0.4 | 0.5 |
|---|---:|---:|
| Confirmed contacts | 19 | 27 |
| Confirmed damage | 220 | 365 |
| 90th-percentile contact gap, simulation frames | 49 | 24 |
| Median accepted basic to actual contact, frames | 10 | 10 |
| First Fire acceptance to following melee acceptance, frames | 67 | 21 |

Basic startup remains the authored 10 ticks. The new fourth strike, shorter recovery and shorter pauses improve continuity. Comparing only third-hit-to-next-basic would omit the intervening earned fourth hit.

## Visual review

**4/4 native visual scenes passed**, with 100 captures at 1280x720 on the same game image: the corrected Practice menu, visible queued-cast feedback, a connected eight-contact training sequence, and a Fire-opening mixed crowd sequence with ten contacts. Actual full-size menu, HUD, third swing, Blitz and crowd frames were inspected. No green backdrop padding, clipped text or smooth supplemental effects remained. The preview GIF uses these actual captures at their original three-simulation-tick spacing; no artwork or combat frames were generated. The new launcher’s local notice marker was reproduced so the first-start informational popup does not cover gameplay.

Original cartridge glyphs, palettes, confirm-window tiles and valid 240x160 load-menu artwork are used. Supplemental effects use a bounded native-resolution additive raster with nearest scaling. Unit rendering verifies exact 4x4 pixel blocks at 1280x720, distinct ground/air/basic finishers, seven spell palettes, clipping/blend preservation and safe texture reset without altering simulation lifetimes.

## Production checks

All seven fresh production suites passed: 64 collision checks, 49 targeting/retention/approach checks, 29 dodge-defense checks, 53 crowd scheduler checks, 41 Shadow concurrency/windup checks, 8 practice entry rules, and seven spell timelines including airborne casts, interruption and nine Cure tier/boost combinations.

Controller and direct-gesture tests passed strict GNU89 with Wall/Wextra/Werror and AddressSanitizer/UndefinedBehaviorSanitizer, including 50,000 deterministic steps. Movement/cancel/finite-air-hang tests passed the same sanitizers. FX passed strict software-renderer checks and sanitized state/lifecycle tests. LeakSanitizer was disabled because this runtime's tracing prevents initialization.

The complete source and test scripts are included. Machine-readable current reports are named cohesion-* under development/test-reports. Earlier action-* and zwebti-practice-* reports are historical evidence for their own hashes and older control schemes; they are not claimed as repeated 0.5 runs.

## Launcher and remaining limits

PowerShell source, package paths, quoting, local guide links, four-key one-time presentation migration, exact original-config backup and Story/Practice save separation were inspected. Later graphics choices and existing mappings are preserved. The launcher acknowledges the existing local modified-rules notice in this package's own data folder; official achievement blocking and earned progress are unchanged.

The Windows game, native bootstrap and WinForms chooser have not been executed on physical Windows hardware here. Complete campaign progression, boss balance and hardware/gamepad verification remain outstanding. Riku/Reverse-Rebirth and link battles retain cartridge combat. The original shared background-effect channel still limits overlapping effects. No ROM, player save or diagnostic memory dump is packaged.
