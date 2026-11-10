# Kingdom Hearts Chain of Memories: Zwebti Mix — native catalog

The inventory is generated from this checkout's authored manifests and C source. It keeps the playable Sora subset separate from the larger game's resources. It never opens a ROM, a save, an extracted sprite, or an external art pack.

Run `python port/tools/build_native_catalog.py --out build/native-catalog-v010` to produce `native_catalog.json`, a searchable standalone `native_catalog.html`, and `coverage.json`. Python's standard library is sufficient. Every record includes its source, line, symbol, family, dependencies and source usages. The JSON also fingerprints every input so a later regeneration can be compared with the reviewed source.

## Defined coverage

All 64 `config/assets/*.yaml` manifests are counted. The visual records include 2,183 sprite sheets, 14,765 authored sheet frames, 130 standalone OAM sprites, 4,138 sheet animation sequences, 28 standalone animation headers and 1,081 typed palettes plus two raw named palette resources. Animation timelines retain their exact authored frame indices and durations; actor loops, speed changes and release rules remain separate runtime behavior. Palette names and region-specific entry counts are available, including duplicate/transparent entries and multi-bank tables. Cartridge RGB values are not embedded or guessed. Shared texture references, piece geometry, tile indices and flips remain explicit; custom sprite tables and map pattern records keep their own resource categories. Movie containers are listed separately and are not described as battle effects.

All `src/**/*.c` files are scanned for native animation, background effect, enemy parameter and task definitions. The catalog contains all 330 shared `BattleAttackDef` IDs, with raw power, knockback, hit stop, hit effects, numeric flags, elements and status behavior. Authored attack-box callsites retain their attack-ID expressions and geometry expressions. Dynamic expressions stay dynamic; an enemy's attack is not relabeled as an interchangeable Sora move.

State and Phase enums cover actors and other game flow. Their case regions expose explicit assignment transitions, referenced states, animation arguments and effect calls. These are lexical source relationships rather than a simulated control-flow graph. Conditional C branches that cannot be balanced as one physical function are listed under `parse_notes`; their callback usage is bounded by the next function definition. No authored manifest or timeline is silently dropped.

Original task declarations and the new mod-only assist wrapper are counted separately. PC callback overrides on an original task are explicitly listed. Other counts are source-definition counts, not a claim that every record is a distinct player attack or safe for direct reuse.

## Connected Sora coverage

The existing action catalog is a subset: 124 concrete Sora dispatcher entries plus two explicit idle placeholders in the source switch. The generated inventory also includes every native `MoveId` registry member, Sora's 77 normal and 30 directional animation bindings, eight attack patterns and its actor states.

The production controller routes expose 116 distinct meaningful entries: 36 magic variants, 18 offensive finishers including Trinity Limit, 45 assist variants and 17 keyblades. Seven native card-reload items and the unused legacy keyblade variant are explicitly outside these combat routes. The original 17 weapon-technique catalog flags remain separate from the controller's 18-finisher family, which also contains Trinity Limit.

`port/tools/test_action_routes_catalog.c` executes the production route functions across every direction combination, facing, charge tier and modifier. It checks that every result belongs to the catalog and that every meaningful entry is reachable. It does not introduce a finisher-selection menu.

## Native visual reuse

The first independent charged-combo accent can reuse `gTaskDescWlogoNvlObj` from `src/map/wlogo.c`. Its `WlogoNvlObjArg` contains only screen X, screen Y and animation ID. Coordinates use 8.8 screen pixels; convert the battle point with `WorldToScreen` first. It owns a 1,536-byte tile resource and one 16-color palette, shares repeated loads by the engine's reference counting, and releases both on native task destruction. Animations 0, 1 and 4 last 31, 29 and 20 authored ticks. A small Sora-owned cosmetic task pool avoids borrowing Sora, companion or BGFX storage. Admission must check tile and palette capacity.

`gTaskDescSmnTinkeff` from `src/btl/smn_tink.c` is another cosmetic source. It copies XYZ from its `BtlObj` argument during initialization and retains no actor pointer. Each of its three native star sequences lasts 24 ticks, with owned tiles and palette released by its destructor. The engine sets `ANIM_FLAG_FINISHED` after a loop cycle, so its native update remains bounded even though initialization requests `ANIM_FLAG_LOOP`.

`BgFxStartEnemyHit(x,y,z)` has no attack parameter and plays a 16-tick impact. `BgFxStartGlow(x,y,z,scale)` fades out under its updater in roughly 17 updates. Both reset the single shared background-effect channel: they are suitable only when that channel is idle and should be skipped during committed native attacks. The Obj tasks above support an independent cosmetic accent without replacing attack damage or native finisher effects.

Raw Marluxia petal tasks remain alive in their DONE state. Map spark tasks retain field objects and field-camera state; event expression tasks retain event-character objects. Their assets can inform a bounded visual wrapper, but their original actor tasks are not interchangeable battle effects.

Independent summons also need owned tiles, lifetime-safe targets, and camera policy: Simba, Cloud, Genie, Dumbo and King originally borrow Sora's tile slot; Genie controls enemy movement; several take over camera and zoom; Tink's original healing trajectory writes actor HP directly. The assist bridge handles these dependencies while preserving authored task behavior.

## Verification

`python port/tools/test_native_catalog.py` compares complete manifest denominators, all 330 native damage IDs, task declarations and callback resolution, dependency IDs, exact Sora animation bindings, and JSON/HTML round-trip integrity. `node port/tools/test_native_catalog_ui.cjs` exercises the generated catalog's real client filtering, pagination and details behavior. Its DOM contract test does not claim browser pixel review.

Native execution is checked separately by `test_action_catalog_native.py`. It recognizes independent assist start/state/contact/heal/end telemetry, verifies actual variants and authored playback, and checks dedicated tile release while Sora stays visible. Source inventory and runtime validation are separate evidence.
