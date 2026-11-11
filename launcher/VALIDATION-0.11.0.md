# Kingdom Hearts Chain of Memories: Zwebti Mix  -  0.11.0 validation

Native checks completed on Windows in November 2026. Tested guest SHA256: `83b99a117f14fb17c6e7c72df7c9eb19e9d1fee7c49cf87830e0691df1b782e9`. Tested unchanged host SHA256: `96401afa24137e87317f13326448c8d5f7a64d6f85c29c9b6b76c4f77790de19`. The shipping SDL3 DLL is unchanged. These hashes also identify the staged runtime input; packaging must preserve them exactly.

## Native checks

| Suite | Passed | Scope |
|---|---:|---|
| Combat scenarios | 102/102 | Real normal/finisher contacts, elemental and advanced branches, spell weaving, defense, crowd/boss probes, Practice/Story and classic/Riku fallback |
| Native dispatcher inventory | 124/124 | Actual native dispatch, tiers, lifetime, neutral recovery and task/shared-channel cleanup in isolated training |
| Strict combat flow | 14/14 | Connected wide/rising openers, bounded offensive requests, startup/post-strike guard/roll, continuation expiry and final-impact defensive cleanup |
| Established assists | 11/11 | Independent attacks, real earned gauge, healing after damage, session carry and safe Trinity/shared-effect scheduling |
| Physical-assist overlap | 3/3 | Sora's three normal contacts and six Blitz contacts preserved beside actual Ariel damage |
| Visual scenarios | 4/4 | Combo, mixed crowd with approach/guard/roll/Fire, menu and effect-channel wait |
| HUD pipelines | 3/3 | Shipping pixels, documented HD development pipeline and Original-view HD-text fallback; 363 captures, none missing |

All suites above use these exact guest/host hashes, isolated profiles and no gameplay cheats. Existing native game code executes through the unchanged host; no new local helper EXE was needed after the Norton reports. Three authored normal positions still precede the contextual fourth finisher. Finishers remain accessible after whiffs.

The dense comparison shortens first-normal acceptance to Blitz admission from 91 ticks in 0.10.1 to 61 ticks (181â†'272 versus 181â†'242). Every normal still contacts. Blitz itself remains 201 ticks with all three segments and six native contacts. This measures one scripted simulation route, not physical controller latency or a universal speed gain. Both deliberate wide/rising opening routes connect through three normals and full Blitz; the rising fixture lands before later normals, so it does not establish a new sustained air route.

Fire now retains its original collision geometry while moving only its quick-cast origin when a valid close forward target would otherwise stand behind the first native box. The three-normalâ†'Fireâ†'third-normalâ†'Fire Raid weave connects in the native test. The production geometry fixture covers mirrored facing, prior 36px spacing, close 26px spacing, depth/height and unusable/stale targets, and inactive/classic/other-actor exclusions.

The portrait's private source now includes the full 0x680 native normal/hurt/low-HP sheet; its displayed-face VRAM allocation remains 0x280. Actual Tinker Bell/mixed captures show complete hurt faces. Ordinary enemy damage produces complete low-HP faces at 68, 42 and 16 HP, captured at least 48 ticks after the preceding hit. Native face/body palettes and damage rules remain intact.

HUD captures show one green player HP bar, one blue MP bar, a narrow gold assist gauge and the original face at bottom right. No old player HUD or control legend appears at top left. Native multi-hit flashes and finisher darkening remain. The HD development probe uses its documented opt-in; the host visibly renders/caches its HD enemy-HP pilot while the generic guest Settings line still reports HUD0. That line is not used as proof of the rendered pipeline.

Local evidence is under the ignored Development folder's `validation/` subdirectories above. Logs, profiles, captures, ROMs and toolchains are not published.

## Hosted source checks and packaging

[The Windows source checks passed on GitHub](https://github.com/ramizwebti/kingdom-hearts-chain-of-memories-zwebti-mix/actions/runs/42961050506): 116 meaningful controller routes, 100,000 invariant steps, pressure/feedback/bounded FX, 85 production targeting/Fire checks, 75 collision checks, 33 companion phase boundaries, full-source portrait/palette/lifecycle/layout checks, icon preservation, packaging archive boundaries, source catalog/client behavior, settings migration and updater/rollback checks. These are automated checks with simulated input, not physical-device testing.

After Norton flagged the locally generated feedback/pressure/routes/collision helpers, those helpers were stopped and remain blocked. Routine source checks and new launcher packaging run on GitHub's hosted Windows machines through Composure. Norton is kept enabled; no security setting is changed automatically. Native game validation reuses only the existing unchanged host. Existing cache safeguards remain active.

The new hosted packaging workflow accepts only the exact COM draft runtime asset and a verified SHA256. It must retain every runtime byte, build the final launcher, verify a synthetic 0.10.1 upgrade and idempotent relaunch, preserve synthetic Story/Practice data and the previous version, then export only five qualified release deliverables. [Hosted packaging passed](https://github.com/ramizwebti/kingdom-hearts-chain-of-memories-zwebti-mix/actions/runs/42962567553) on source commit 060e815e37d645bc51b5b50e26a0568f81bd6603. Its seven archive-boundary checks, exact-runtime checks, synthetic 0.10.1 upgrade and idempotent launcher relaunch passed. The synthetic profiles and previous version stayed byte-identical. Final source checks also [passed on that same commit](https://github.com/ramizwebti/kingdom-hearts-chain-of-memories-zwebti-mix/actions/runs/42962567810). The hosted synthetic installation uses a short runner temporary path for classic Framework path compatibility. Only its guarded packaging job grants the repository-scoped token the push access GitHub requires to read an unpublished draft; the script reads only the fixed asset, strips the token before building/running programs and does not publish.

The [published 0.11.0 release](https://github.com/ramizwebti/kingdom-hearts-chain-of-memories-zwebti-mix/releases/tag/v0.11.0) is confirmed latest. Its tag points to that exact packaging source commit. GitHub's SHA256 and size match every checked upload, including the separate runtime input. The final ZIP SHA256 is `1b9eebc75e4c316ddae6737c1c8a18ec773ae930b3cb340f03922ee1d0e6af33`; launcher SHA256 is `807b6e8bb7b4de3ef76c6b26f08c33996d838e27f7e86630906ca71d21114c7e`. The [qualified Manifest JSON](https://github.com/ramizwebti/kingdom-hearts-chain-of-memories-zwebti-mix/releases/download/v0.11.0/Kingdom-Hearts-Chain-of-Memories-Zwebti-Mix-0.11.0-Manifest.json) records every packaged member and hosted check.

The fresh catalog retains 33,924 source metadata records, 355 fingerprinted inputs, all 330 attack definitions, 124 concrete dispatcher entries and zero unresolved records. Its archive and provenance are generated from the packaging source commit, with no ROM/image/palette pixel bytes. Catalog ZIP SHA256 is `452331c762c84bafe02fa7095fd63054d9928812f62548d8998ba960703530e8`.

The local installation was then updated from 0.10.1 to 0.11.0 by staging the exact cloud-verified ZIP and atomically switching the active-version pointer. All 16 existing profile files stayed byte-identical, and all previous version folders remain. No new local installer/helper EXE was executed: the unchanged game host and existing launcher continue to use the active version. This local payload check is separate from the hosted C# installer check. A complete live network update through the launcher remains unverified.

## Limits

Physical controller feel, full campaign progression, encounter-wide balance and every possible enemy/technique interaction remain unverified. Shadow crowd reservations do not establish equivalent behavior for every enemy family. Automated checks do not prove that every player will find the combat fun. The original0.10.1 release and its evidence remain unchanged in [VALIDATION-0.10.1.md](VALIDATION-0.10.1.md). Other Kingdom Hearts projects remain outside this mod's scope.
