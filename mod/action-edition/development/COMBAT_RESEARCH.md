# Kingdom Hearts Chain of Memories: Zwebti Mix — combat research and improvement plan

Research checkpoint: November 10, 2026. Baseline: local source `5b7351e1d9e24ea9fd9c5ecd5e6690c5d35b4276`, restored on GitHub as `bb915b2affaefa4f8eabb0dc83d130979259c823`, with identical tree `198354235e97184b8e9ae8dc3603149ddb33b202`. The playable checkpoint remains 0.10.1. This document proposes the next combat work; no gameplay code or timings were changed for this research phase.

## Main conclusion

The first priority is to make an exchange connect: player intent, attack motion, real contact, enemy reaction, reachable follow-up, and a fair defensive decision. The present kit is large enough to begin. Its principal weakness is the relationship between moves, rather than a shortage of named techniques.

Preserve the three normal strikes, native multi-hit finishers, original screen darkening, controller controls, independent summons and compact bottom-right HUD. Improve the rules around them. A usable move is not necessarily a usable combo link: it must leave the player and enemy in compatible states, at a compatible distance and height, with a readable opportunity to continue or defend.

The proposed identity is expressive Kingdom Hearts crowd combat: quick basic decisions, deliberate powerful techniques, meaningful positioning and defense, and native spectacle with clear impacts. Different combat games make different choices about commitment. Their solutions are references to evaluate, not universal rules to transplant.

## Developer research

These are developer-authored explanations, official documentation, presentations or direct developer interviews. The observations in this section summarize those sources; the later COM design is our application of them. Published timing values from another game are not target values for this engine.

| Source | Supported design lesson |
|---|---|
| [1. Takahisa Taura and the NieR:Automata team, PlatinumGames](https://www.platinumgames.com/official-blog/article/8997) | Response is tuned through individual animation cancel windows, restrained speed adjustments and repeated playtests. Enemy placement is part of that same work. Animation appearance and controllability have to be evaluated together. |
| [2. Ben Ruiz, Aztez: Frame By Frame — The Sword’s Basics](https://aztez.com/blog/2014/09/08/frame-by-frame-the-swords-basics/) | Separate input acceptance from move execution. Authored timelines distinguish hitboxes, translation, different cancels and onward links. Defense can have different permission from the next attack; a strong ender can restrict further offense. |
| [3. Ben Ruiz, Aztez: The Amalur Problem](https://aztez.com/blog/2012/05/24/the-amalur-problem/) | Mandatory recovery can undermine fluid expressive combat. Deliberate commitment belongs to a chosen combat identity; removing every commitment is not the only legitimate design. |
| [4. Ben Ruiz, Anatomy of a Successful Attack](https://aztez.com/blog/2014/01/06/anatomy-of-a-successful-attack/) | Attack and struck animation, trails, contact effects, sound, restrained shake and contact freeze work together. An impact should feel synchronized without hiding the action. |
| [5. Mihir Sheth, Evolving God of War’s Combat for a New Perspective, GDC 2019](https://media.gdcvault.com/gdc2019/presentations/Sheth_Mihir_EvolvingCombat.pdf) | Target selection, movement assistance and enemy reactions can keep a follow-up reachable. Group aggression is scheduled; struck enemies can retain attack-allocation tokens briefly, rewarding offense with room to act. See targeting/movement pp.97–114, strike assistance pp.129–150 and offensive safety pp.154–158. |
| [6. Takaaki Yamaguchi, Bayonetta 2: Figuring Out Damage Motion](https://www.platinumgames.com/official-blog/article/6647) | Attack-specific victim responses are a substantial part of impact and satisfaction. The enemy’s motion is part of the player’s attack design. |
| [7. Nintendo, Bayonetta official manual, combat section](https://www.nintendo.com/eu/media/downloads/games_8/emanuals/wii_u_6/bayonetta/ElectronicManual_WiiU_Bayonetta_EN.pdf) | Dodge Offset preserves a combo position across a deliberate dodge through held attack input. Preserving sequence state is distinct from continuing the interrupted animation. |
| [8. Nintendo, Bayonetta 3 version 1.2.0 adjustments](https://www.nintendo.com/en-gb/Support/Nintendo-Switch/Game-Updates/How-to-Update-Bayonetta-3-2290805.html) | Official changes tune defensive timing, successful-block feedback, charge/sequence continuity and repeated-defense rewards, while fixing evade/summon exploits. Forgiveness and abuse prevention must be tested together. |
| [9. Yusuke Hashimoto, Enemy Design in Bayonetta 2, Part 1](https://www.platinumgames.com/official-blog/article/6701) | Enemy readability and behavior support the intended evade/counter loop. Support enemies can change encounter decisions without merely having more health or damage. |
| [10. PlatinumGames, On Creating Enemies](https://www.platinumgames.com/official-blog/article/1924) | Enemy behavior needs a fair, recognizable opportunity to respond. Player and enemy interactions must be considered together. |
| [11. Sébastien Bénard, Dead Cells: What the F*n!?, GDC 2019](https://media.gdcvault.com/gdc2019/presentations/Benard-Sebastian-DeepCells.pdf) | Reliable controls and intention-aware assistance keep incidental handling problems from becoming the main difficulty. Its movement/aiming examples are relevant analogies, not prescriptions for COM’s isometric geometry. |
| [12. Thomas Vasseur, Art Design Deep Dive: Dead Cells animation](https://www.gamedeveloper.com/production/art-design-deep-dive-using-a-3d-pipeline-for-2d-animation-in-i-dead-cells-i-) | Key poses, timing, interpolation and effects are developed around gameplay. Distinguish anticipation, contact and follow-through instead of uniformly scaling an entire clip. |
| [13. Motion Twin / Steve Filby, Dead Cells developer interview](https://80.lv/articles/interview-with-the-developers-of-dead-cells) | Impact combines brief freeze/slowdown, particles and dedicated sound. The interview’s specific implementation is historical evidence, not a universal frame recipe. |
| [14. Sébastien Bénard, Dead Cells developer introduction](https://blog.playstation.com/2018/01/25/rogue-lite-action-platformer-dead-cells-coming-to-ps4-this-year/) | Responsive handling and early mobility/defense support mastery of an existing kit. Acquiring more power is not the sole answer to difficulty. |
| [15. Denny Yeh, Designing God of War’s Valkyrie battles](https://blog.playstation.com/archive/2019/02/01/how-god-of-wars-nine-epic-valkyrie-boss-battles-were-created) | Enemy attacks test particular player responses. The intended encounter behavior should determine the boss design. |
| [16. Riot Games, Clarity in League](https://www.leagueoflegends.com/en-us/news/dev/clarity-in-league/) | Visual/audio hierarchy should express gameplay importance; hitboxes and effects should agree; accumulated noise can prevent an informed response. This principle transfers more directly than the game’s specific visual style. |
| [17. Riot Games ArtEdu, Visual Effects](https://www.riotgames.com/en/artedu/visual-effects) | Effects must represent the correct action/state, identify their source and remain coherent across a complete kit. Restraint is an intentional VFX discipline. |
| [18. Capcom producer Matt Walker, DMC5 official PlayStation interview](https://blog.playstation.com/2019/03/08/old-dante-new-tricks-capcom-talks-devil-may-cry-5-out-today-on-ps4/) | Launchers, aerial offense, pulling enemies and live style changes are examples of tools with connective purposes. The useful reference is their relationship to positioning and available actions. |
| [19. Square Enix, FFXVI official battle overview](https://na.finalfantasyxvi.com/en-gb/battle) | Torgal can act alongside the player, with timed commands producing coordinated offense. The page establishes the companion concept, not its internal cancel or resource implementation. |
| [20. Greg Kasavin / Supergiant, The Beefy Update](https://www.supergiantgames.com/blog/get-pumped-for-the-beefy-update/) | The Hades update identifies a dash overhaul as a responsiveness improvement. It supports evaluating mobility as a central part of combat, without establishing unpublished buffer internals. |

## What the existing source actually does

References below use the baseline source. Line numbers will move when implementation begins. Existing generated metadata contains 33,924 records and all 124 concrete Sora dispatcher entries; that inventory includes diagnostic reload/unused entries. The 116 meaningful routes are not proof of 116 satisfying combo connections. Enemy tasks and effects in the broader catalog are not automatically playable Sora attacks.

| Finding | Evidence in this project | Consequence to investigate |
|---|---|---|
| Basic links wait for complete animation and neutral state | `src/btl/btl_sora.c:3880–3890`, `7544–7550` | Prevents self-restarting, but makes every recovery tail part of the required wait. There is no authored attack-to-attack transition window. |
| Most variation is selected at the fourth action | `port/src/port_action_routes.c:15–32`; `src/btl/btl_sora.c:2631–2643` | Direction/hold selects finishers. Basic visual patterns rotate automatically; changing the animation does not necessarily create a different tactical decision. |
| Defensive interruption clears combo state | `src/btl/btl_sora.c:2791–2815`; `port/src/port_action.c:96–114` | Guard/roll may rescue the player while discarding their sequence progress. Bounded continuity is a candidate improvement, not an existing feature. |
| Dodge requests are consumed before defensive eligibility is checked | `port/src/port_input.c:769–777`; `src/btl/btl_sora.c:1617`, `1672` | A short roll tap during a lock can disappear before an eligible phase. Verify with a focused native trace before changing it. |
| Offensive queue age depends on ability to act | `port/src/port_action.c:180–184`; `src/btl/btl_sora.c:7548` | An old attack or spell can survive a long restriction. Contrast this with the consumed defensive tap; define one predictable input-age policy. |
| Most full native techniques prevent defensive exit throughout the action | `src/btl/btl_sora.c:708–726`, `148–149` | Late Blitz/Sonic Blade have exceptions; many spells and other techniques share a blanket lock. Commitment needs review per phase and move role. |
| Target selection is repeated between accepted attacks | `src/btl/btl_sora.c:7292–7309`; `port/include/port/port_action_target.h:21`, `27` | The retained target has a small score bonus rather than strong continuity. Selection reach can exceed the small approach correction at `btl_sora.c:7339–7342`. |
| Gesture choice and targeting can use different input samples | `port/src/port_action.c:270–284`; `src/btl/btl_sora.c:7532–7534` | A direction chosen for the finisher can differ from held input when it executes. Specify whether each intent is captured or live. |
| Assist overlap is broadly paused | `src/btl/btl_sora.c:919–931` | Player full-native techniques and quick spells pause the assist even before a specific effect conflict is considered. Safe independence currently has significant serialization. |
| Finishers follow accepted normals, not mandatory hit confirmation | `port/src/port_action.c:269–305`, `349–367` | Keeps finishers accessible after whiffs. Do not reintroduce the earlier failure where missing one contact makes the finisher vanish. Use confirmed contact to enrich continuation and rewards instead. |
| Some valuable feedback and crowd rules already exist | `src/btl/btl_collision.c:824–889`; `port/src/port_action_fx.c`; `src/btl/battle_runtime.c:2020–2060` | Contact effects, hitstop caps, limited early knockback and Shadow attack scheduling already exist. Audit actual timing/readability before adding duplicates. Shadow coverage does not establish equivalent behavior for every enemy family. |

### Timing evidence and its limits

The existing focused 0.10.1 Blitz trace records normal actions accepted/completed at 181/203, 221/249 and 261/299: spans of 22, 28 and 38 logged frames. Blitz is accepted at 313 and completes at 514, spanning 201 frames. Its recorded contacts occur at 345/357, 412/424 and 479/491, leaving 55-frame intervals between the pairs. The important observation is a sequence of distinct native segments with large intervening gaps, not a single instantaneous finisher.

The fixture deliberately supplies normal inputs 40 frames apart. Those scripted intervals are not measurements of the earliest legal link. These are existing logs, not new controller measurements. `port/src/port_main.c:3138` and `3543` describe the nominal 59.7275 Hz GBA/VBlank clock; real input-to-display time also depends on hitstop, task scheduling, rendering and the device. Do not convert a log frame difference into a claimed physical input latency.

Preserve the complete native Blitz sequence while investigating phase spacing. Count Sora’s normals separately from Blitz contacts; an aggregate battle hit count is not the finisher’s own contact count.

## Proposed combat design

### 1. Give each action an explicit transition contract

Record startup, real collision events, victim response, active sequence completion, essential follow-through and optional recovery. Independently record when the next input may be accepted and when its action may execute. The current native animation/task remains the authority for the actual attack.

Keep one deliberate pending offensive request, with a defined lifetime and replacement rule. A fresh attack press must never restart the current swing. Repeated attack presses must never skip required finisher segments. Hitstop can preserve intentional input without allowing a stale request to survive an unrelated long technique or resource wait. Distinguish simulation time, hitstop and elapsed input age explicitly.

Author attack links and defensive exits separately. Protect meaningful strikes and multi-hit choreography from ordinary attack mashing. Allow appropriate guard/roll recovery exits where the native task can be cleaned up safely. If a particular defensive interruption is allowed before the attack is complete, it must forfeit unfinished damage and clear owned attack/effect tasks. Long cinematic techniques can remain deliberate commitments when their opening and incoming threats make that decision understandable.

Do not assign one universal cancel tick or speed multiplier. First locate actual native events for representative actions, then evaluate their transitions in play.

### 2. Retain the three normals and give branches a purpose

The normal three-piece remains the reliable backbone. Work toward a small grammar of approach, pressure, control/launch, pursuit, defense and ender. A variation should change what the player can accomplish, not merely swap a sprite or spell name.

Candidate routes using existing assets:

| Situation | Candidate route to prove | What makes it meaningful |
|---|---|---|
| A close opening | Three normals → full Blitz | A readable escalating sequence with a deliberate ending; every expected native hit remains available. |
| A target drifts outside melee reach | Normal → quick spell → return to the normal chain | The spell bridges a spacing/timing problem while preserving a clear melee continuation. It must not simply add a long neutral wait. |
| A crowd threatens the chain | Normal → deliberate roll/guard → short re-engagement opportunity | Defense changes positioning and timing without automatically discarding all useful progress. Taking damage still has a real cost. |
| A grounded victim can be displaced | Verified native control/launch option → reachable aerial continuation → chosen ender | Victim height and recovery make the air route physically possible. Shock Impact’s existing launch/status behavior needs verification before assigning this job. |
| A distant target needs approach | Verified Sliding Dash/raid branch → appropriate follow-up | Movement must leave Sora facing the intended target within an honest next-hit envelope. A long knockback attack may instead be an intentional ender. |
| Assist gauge is ready | Assist entry/control phase + Sora’s compatible pressure | The ally creates an opportunity while Sora remains active. Only actual conflicting presentation phases should serialize. |

These are prototype candidates, not claims that the restored build already provides these links. Preserve access to the entire meaningful native move set as the contract expands. Do not collapse it back to two finishers or make a selection menu a prerequisite for combat.

### 3. Make the enemy remain a plausible partner in the sequence

Keep a valid intended target through compatible links. Permit deliberate direction changes to select another target; do not silently switch to a closer bystander. Reject dead, untargetable or vertically incompatible targets. Decide what happens when the victim dies between input acceptance and execution.

Test selection range against actual reach and pursuit, including the isometric depth axis. A move that selects an enemy it cannot reasonably reach should not create the impression of a successful auto-approach. Use bounded correction, preserve facing/pose continuity, and avoid cross-arena snaps.

Early pressure attacks should usually leave compatible victims near enough to continue. Strong displacement needs a chase/air option or an intentional end. Author victim stagger, knockback and height alongside the player transition. Keep resistant/armored/boss reactions explicit so a normal hitstun rule does not accidentally make every boss permanently helpless.

### 4. Make defense predictable without making it free

Test a short defensive input slightly before, during and after every permitted exit. Keep that request until its defined legal moment or expiry; clear it predictably when a damage reaction or invalid state supersedes it. Resolve attack/guard/roll priority deliberately and test held inputs as well as taps.

Prototype bounded combo continuation through a deliberate successful defense. Preserve only the next legitimate chain position/intent for a short opportunity, not invulnerability, unfinished hitboxes or unearned contact rewards. A player should not be able to roll indefinitely with a ready finisher or cancel a move while all of its damaging children continue for free.

Evaluate the existing right-stick-click assist charge alongside face-button attacks and guard on a standard controller. Being dispatchable through modifiers is not enough if a comfortable grip cannot produce the intended simultaneous actions. Keep the current bindings stable during the first timing prototype; assess ergonomic alternatives in the separate controller review.

### 5. Tune the enemies and crowd around the same rules

For each tested enemy, specify the cue, response opportunity, interruption/stagger behavior, recovery and pressure on nearby allies. Crowd scheduling must consider ranged/support enemies and threats outside the immediate melee, not only practice Shadows.

When a player earns a stagger, investigate whether another enemy immediately replaces that pressure before any follow-up is possible. When the player commits to a long native technique, test whether readable threats and available defense make that commitment fair. Avoid solving this with universal attack invulnerability or blanket enemy freezes.

Use encounters that create different choices: a vulnerable close target, an evasive target, a resistant target, and a mixed crowd. A boss must be evaluated through its own rules rather than assumed to inherit ordinary hitstun.

### 6. Reuse native effects with a clear hierarchy

Use the catalog to classify effects by role: anticipation/charge, weapon motion, actual contact, elemental status, enemy warning, assist arrival, and decisive ending. Native effects from another character are candidates only after their palette, ownership, placement and meaning are checked.

Keep light contacts readable and compact. Heavier contacts should have distinct victim motion, sound and impact; finishers retain their native darkening and multi-hit sequence. Whiffs can have a swing trail and sound, but must not show a contact flash or grant contact resources. Optional accents must agree with real hit timing/range and leave incoming danger, enemy silhouettes and Sora’s facing visible.

Audit native hitstop and existing supplemental effects together before layering more. Multiple independent freezes or large flashes can make a successful hit feel slower. Compare native-only and supplemental presentation to identify what each layer contributes. The HUD remains the original portrait with one HP bar, one MP bar and small contextual indicators at bottom right; combat does not gain a command deck or permanent control instructions.

## Implementation order after research

1. **Measure the current loop.** Produce a per-action timeline for the three normals, Blitz, one quick spell, one control/approach action and one assist. Include actor state, collision events, target position/height, reactions, input capture, eligibility and cleanup. Match traces to frame captures.
2. **Make input and transitions reliable.** Address the offense/defense buffer asymmetry and author a small set of native-event-based links. Verify every protected hit and resource debit before broadening the prototype.
3. **Make connections physically work.** Tune target retention, pursuit and victim reactions together. Prove melee→spell→melee and one real ground/air route, including misses, target deaths and landing.
4. **Integrate defense, assists and crowd pressure.** Test bounded continuation and real effect-resource conflicts. Preserve independent Sora activity, safe task cleanup and fair enemy opportunities.
5. **Polish and expand.** Tune synchronized native feedback, then extend the proven contract across the remaining move roles. Keep each technique’s identity rather than converting every move to the same fast cancelable attack.

## Acceptance and playtest plan

| Test | Required evidence |
|---|---|
| Repeated attack presses | Exactly one current action; one intentional next request; no restarted swings, skipped finisher segments or duplicate collision rewards. |
| Early/on-time/late defense | A trace explains each accepted, buffered, expired or denied request. No unexplained lost tap or surprise delayed action. |
| Protected finisher sequence | All expected native segments/contacts and presentation remain on the exact built guest. Cancellation, if permitted, removes unfinished damage and owned tasks. |
| Target continuity | Target ID, planar position and height explain every follow-up; no accidental switch to a bystander, dead actor or unreachable height. |
| Mixed transitions | Melee/spell, ground/air and defense/re-entry routes connect against real victim reactions rather than only passing dispatcher tests. |
| Assist overlap | Sora remains active; safe nonconflicting phases overlap; genuine shared-resource waits are bounded and spend no gauge before successful spawn. |
| Crowd fairness | Melee/ranged/support pressure and boss resistance are tested beyond the Shadow fixture. Threats remain readable during native darkness and effects. |
| Feedback truthfulness | Whiffs, blocks, light hits, heavy hits and finishing contacts differ appropriately. Contact cues follow collision events; effects do not misrepresent reach. |
| Resource and task cleanup | No duplicate MP/gauge reward, free unfinished damage, persistent invulnerability, stale target, orphan task or corrupted presentation after every exit. |
| Physical controller | Deliberate inputs, novice mashing, held gestures, simultaneous defense/assist inputs, branch mistakes and fatigue are evaluated on a real standard controller. Automated reports do not substitute for this. |

Collect first-action response, earliest successful follow-up, contact-to-next-contact gaps, lost/expired input counts, unintended target/branch switches, interruption causes and complete-finisher rate. Compare those measurements to controller playtest observations. Timing targets should be established from the measured prototype; this research does not invent universal millisecond thresholds.

## What remains unproven

The restored 0.10.1 artifacts retain their original focused validation and limitations. The research does not establish improved feel or new gameplay behavior. Physical-controller feel, full campaign progression/balance and every native enemy/technique interaction require further work. The next step is the measured small-loop prototype above, using existing native assets before commissioning new attack art.
