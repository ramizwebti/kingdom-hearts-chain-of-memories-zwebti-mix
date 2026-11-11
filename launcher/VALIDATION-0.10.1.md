# Kingdom Hearts Chain of Memories: Zwebti Mix — validation

Version **0.10.1**, checked on Windows on **November 9, 2026**. This update changes project artwork and title presentation, Windows identity, release naming and native Windows startup. Combat behavior remains the 0.10 implementation.

Tested, packaged and installed guest SHA256: `c0a02c81be4ff86301a8248574e3377a38dc8f37e5a7d5c03fddcaae2db08774`. Stamped Windows host SHA256: `c76f054f9df4f926519d313bb4ab64f7fc1595fbc23e513df05535e758f25106`. The original shipping SDL3 library is unchanged.

## Current checks

- **Seven focused native scenarios passed on these exact binaries**: Story title/reveal, Practice menu/title return, complete native Blitz, Options open/close, missing image fallback, invalid image fallback, and the preserved 0.10 guest as an original-title comparison. The fallback title BMPs are byte-identical to the original guest under the same host. Options stays above the new artwork; closing it restores the title. Full Blitz retains its three segments and native multi-hit sequence. Physical input was excluded only in the isolated fixtures; shipping controller behavior was not changed.
- The actual Windows launcher was visually inspected: full silver Kingdom Hearts/gold Chain of Memories/pink Zwebti Mix wordmark, blossom ornaments, heart-only title-bar icon, full project caption, readable controls and no clipping. The final compiled EXE's embedded logo, icon and release archive match their checked inputs. PNG corners are transparent, and all ten Windows ICO sizes are present.
- Native host ABI, mixed argument, low mapping, renderer/texture, input, title/fullscreen-control and BIOS checks passed. Real boot-marker tests returned the expected pre-guest errors and guest-requested exit; a Unicode synthetic guest path worked. Native startup uses a wide-character CRT entry, explicit SDL readiness and UTF-8 arguments. Earlier intermittent Windows process-start failures do not establish an SDL-shim defect; the final host is validated by actual execution.
- **71 updater/store checks passed**, including complete fully qualified archive/checksum pairs and legacy-pair compatibility. C# compilation and PowerShell syntax checks passed. Icon stamping preserves executable code/data and non-icon resources; restamping the final host is byte-identical.
- **Upgrade 0.10.0 → 0.10.1 passed twice in an isolated installation**, followed by the existing local installation. All **16 profile files stayed byte-identical**; 0.10.0 is retained. Archive CRC, exact tested runtime hashes, current logo/icon paths and ROM/save exclusions passed.

Focused native results: `build/branding-v0101-shipping/report.json`. Icon/resource results: `build/branding-v0101-runtime-icon-verification.json`. Native host results: `build/branding-v0101-native-entry/validation.json`. These are ignored local build evidence, not player data for publication.

## Native catalog and combat reference

The metadata catalog was refreshed against the current presentation sources: **33,924 records**, 355 source inputs, all330 native attack definitions, all124 concrete Sora dispatcher entries and zero unresolved records. Independent denominator and actual-client filtering/pagination/details checks pass. It includes no cartridge image/palette bytes, ROMs or saves. Combat sprites/effects remain native; the supplied subtitle and heart are project branding assets.

The broader 0.10 reference passed102 combat scenarios,124 catalog execution cases,11 assist probes, four visual scenarios and three HUD modes on guest `3ffa8e5a04013b35da0a4f39c5137821b0b522c6f5bd412ed14f0dcd9dd756ff`. Those broad suites were **not rerun on the changed 0.10.1 guest**. Their exact prior evidence and limits are preserved in [VALIDATION-0.10.0.md](VALIDATION-0.10.0.md); the current release uses the focused checks above.

Physical controller feel, full campaign progression and encounter balance remain unverified. Other Kingdom Hearts projects remain outside this mod's scope.

## GitHub restoration and research — November 10, 2026

The full source and branding are restored at `ramizwebti/kingdom-hearts-chain-of-memories-zwebti-mix` (repository ID1453493588). Restored main commit `bb915b2affaefa4f8eabb0dc83d130979259c823` has tree `198354235e97184b8e9ae8dc3603149ddb33b202`, identical to local checkpoint `5b7351e1d9e24ea9fd9c5ecd5e6690c5d35b4276`.

Published release `v0.10.1` uses source commit `1c45f8dfc8309a8adaf1191e3322405edf5460d0`, tree `c8341886869e0c22bb35d464a750dad9e6c91662`, identical to the original local release source `5d5086d6fcf02ec6a51cf3463b67ddb9b1bdee61`. All five uploaded assets (EXE, runtime ZIP, ZIP checksum, branding ZIP and native-catalog ZIP) have GitHub SHA256 digests matching their verified saved files. GitHub normalizes spaces in the EXE asset name to periods; its bytes are unchanged.

No new EXEs were built or executed, gameplay files changed, installed settings altered, or native combat suites rerun during restoration/research. Existing test evidence above keeps its original scope. Live publishing is verified; the launcher's complete live download/install path remains untested. The current source audit and proposed acceptance criteria are in [COMBAT_RESEARCH.md](../mod/action-edition/development/COMBAT_RESEARCH.md).
