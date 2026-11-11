# Kingdom Hearts Chain of Memories: Zwebti Mix â€” 0.11.0 validation

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
| HUD pipelines | 3/3 | Shipping pixels, documented HD development pipeline and Original-view HD-text fallback;363 captures, none missing |

All suites above use these exact guest/host hashes, isolated profiles and no gameplay cheats. Existing native game code executes through the unchanged host; no new local helper EXE was needed after the Norton reports. Three authored normal positions still precede the contextual fourth finisher. Finishers remain accessible after whiffs.

The dense comparison shortens first-normal acceptance to Blitz admission from91 ticks in0.10.1 to61 ticks (181â†’272 versus181â†’242). Every normal still contacts. Blitz itself remains201 ticks with all three segments and six native contacts. This measures one scripted simulation route, not physical controller latency or a universal speed gain. Both deliberate wide/rising opening routes connect through three normals and full Blitz; the rising fixture lands before later normals, so it does not establish a new sustained air route.

Fire now retains its original collision geometry while moving only its quick-cast origin when a valid close forward target would otherwise stand behind the first native box. The three-normalâ†’Fireâ†’third-normalâ†’Fire Raid weave connects in the native test. The production geometry fixture covers mirrored facing, prior36px spacing, close26px spacing, depth/height and unusable/stale targets, and inactive/classic/other-actor exclusions.

The portrait's private source now includes the full0x680 native normal/hurt/low-HP sheet; its displayed-face VRAM allocation remains0x280. Actual Tinker Bell/mixed captures show complete hurt faces. Ordinary enemy damage produces complete low-HP faces at68,42 and16HP, captured at least48 ticks after the preceding hit. Native face/body palettes and damage rules remain intact.

HUD captures show one green player HP bar, one blue MP bar, a narrow gold assist gauge and the original face at bottom right. No old player HUD or control legend appears at top left. Native multi-hit flashes and finisher darkening remain. The HD development probe uses its documented opt-in; the host visibly renders/caches its HD enemy-HP pilot while the generic guest Settings line still reports HUD0. That line is not used as proof of the rendered pipeline.

Local evidence is under the ignored Development folder's `validation/` subdirectories above. Logs, profiles, captures, ROMs and toolchains are not published.

## Hosted source checks and packaging

[The Windows source checks passed on GitHub](https://github.com/ramizwebti/kingdom-hearts-chain-of-memories-zwebti-mix/actions/runs/42961050506):116 meaningful controller routes,100,000 invariant steps, pressure/feedback/bounded FX,85 production targeting/Fire checks,75 collision checks,33 companion phase boundaries, full-source portrait/palette/lifecycle/layout checks, icon preservation, packaging archive boundaries, source catalog/client behavior, settings migration and updater/rollback checks. These are automated checks with simulated input, not physical-device testing.

After Norton flagged the locally generated feedback/pressure/routes/collision helpers, those helpers were stopped and remain blocked. Routine source checks and new launcher packaging run on GitHub's hosted Windows machines through Composure. Norton is kept enabled; no security setting is changed automatically. Native game validation reuses only the existing unchanged host. Existing cache safeguards remain active.

The new hosted packaging workflow accepts only the exact COM draft runtime asset and a verified SHA256. It must retain every runtime byte, build the final launcher, verify a synthetic0.10.1 upgrade and idempotent relaunch, preserve synthetic Story/Practice data and the previous version, then export only five qualified release deliverables. Final packaging execution and publication are still pending at this source checkpoint; do not label a package verified until its hosted manifest and server hashes have been checked.

## Limits

Physical controller feel, full campaign progression, encounter-wide balance and every possible enemy/technique interaction remain unverified. Shadow crowd reservations do not establish equivalent behavior for every enemy family. Automated checks do not prove that every player will find the combat fun. The original0.10.1 release and its evidence remain unchanged in [VALIDATION-0.10.1.md](VALIDATION-0.10.1.md). Other Kingdom Hearts projects remain outside this mod's scope.
