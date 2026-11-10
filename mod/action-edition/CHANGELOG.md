# Kingdom Hearts Chain of Memories: Zwebti Mix — changelog

## 0.10.1 — pink logo and clear project identity

- Combined the original game-title reference and gold Chain of Memories label with the supplied pink Zwebti Mix subtitle and blossoms. The separate pink heart is reserved for Windows/app icons.
- Updated the launcher, startup dialogs, game title presentation, Practice header, help artwork and current release filenames. Window captions identify Kingdom Hearts Chain of Memories: Zwebti Mix.
- The native title retains its original reveal, character art, fade and menus, and falls back to its original logo if the new image cannot load. No combat behavior changed in this presentation update.
- Existing installation/profile identifiers remain compatible. Releases use a full project name; updater compatibility with older archive names is retained.

## 0.10 — controller combo tree and independent assists

- Three normal attacks lead into contextual fourth-hit finishers; direction, hold timing and selected magic determine meaningful branches without a finisher menu.
- All116 meaningful native routes are connected:36 magic,18 finisher/team moves,45 assist variants and17 obtained Keyblades. Native effects, multi-hit sequences and screen darkening remain.
- Fourteen assist families use an earned gauge while Sora continues fighting. The original portrait, one HP bar, one MP bar and compact assist/magic information sit at bottom right.
- Detailed coverage and limits are recorded in [current validation](../../launcher/VALIDATION.md).

## Historical 0.8 — native finishers, guard and clean gauges

- The third combo step uses distinct native Blitz or Air Rush, with pose-owned contact and full recovery. Landing retains the combo step.
- Basics use 1.5× authored timing and finishers 2×, preserving poses and preventing repeated Attack from restarting them.
- Dodge starts immediately. A held guard stops frontal physical attacks; rear hits and magic bypass it.
- Replaced stacked cartridge HP tiers with one green bar for total HP/max HP and one blue MP bar. Kept the native portrait and tiny spell symbol; no command frame or combat controls are displayed.
- Preserved short keyboard event edges and the first fresh key after UI use. Real keyboard finisher/defense checks pass. See `launcher/VALIDATION.md` for the final checks and limits.

# Zwebti Mix 0.7 — full swings and compact HUD

- Attacks finish their authored animation before a queued next swing begins; the normal Kingdom Key animation timing replaces the accelerated variant.
- The third attack is the ground or aerial finisher. Removed the hidden fourth-press requirement.
- Reduced startup movement and kept basic hit reactions close enough to connect a normal chain.
- Replaced the command panel with a narrow blue MP gauge beneath HP and one tiny spell mark. Removed combat control labels and procedural slash/glow overlays.
- New single-file launchers upgrade the installed game while keeping Story and Practice profiles separate from release files.

# Zwebti Mix 0.5 — connected combat and native pixel menus

- Rebuilt the arena menu and command panel with the original GBA bitmap font, menu artwork and window tiles. Native integer scaling and nearest filtering replace the previous smooth presentation defaults.
- Simplified magic: Q casts on press, E selects the next spell. Removed the hold-to-browse gesture and direction-based standalone techniques from action controls.
- Connecting three basic attacks earns a fourth attack on the same button: Blitz on the ground or Air Rush in the air. No MP cost or separate technique cooldown. Whiffs do not earn it.
- Removed individual spell cooldown timers. MP, the cast animation and the active effect still determine availability. A visible pending cast can be canceled with Attack or spell cycling.
- Shortened attack and spell recovery, increased movement response, and retained pressed inputs during hit pauses.
- Reworked supplemental effects at native pixel resolution: alternating slash arcs, gold descending Blitz cuts, blue Air Rush streaks, confirmed-contact bursts and seven elemental cast colors, with crisp nearest scaling.
- Existing settings receive a one-time graphics migration with an original-file backup. Input mappings and saves remain intact; subsequent user graphics changes are preserved.

# Zwebti Mix 0.4 — branding and Practice Arena

- Renamed the project Zwebti Mix with a brush-ink Z emblem, matching wordmark, and Windows app icon.
- Added a visible Zwebti Mix.exe launcher, Story/Practice selection, ROM-search progress and a plain-language START_HERE guide.
- Practice Arena launches directly into a training target, Shadow crowd, mixed encounter, Axel, or Jafar, with a native retry/arena menu afterward.
- Practice has its own data folder and does not import story saves.

# Action Edition 0.3 — combat flow and readability

- Direction-aware nearby target selection, stable targeting during a swing, and a bounded approach through normal collision handling. Targets disappearing or moving beyond reach cannot pull Sora backward. Directional misses face the intended direction.
- Finishers can link into spells or techniques after the active strike, giving combos a useful follow-up while retaining recovery for repeated basic attacks.
- Successful basic swings recover 1 MP; finishers recover 5 MP. One reward per swing prevents multi-target MP farming and connects melee play to magic use.
- Tap-release magic and a hold-to-browse seven-spell palette with costs and availability. Browsing no longer accidentally casts, moves Sora, or consumes the dedicated Dodge input.
- Short, specific feedback for insufficient MP, recharge and cooldown, plus visible melee MP gains.
- Amber warning rings, direction markers and exclamations during real Shadow attack windups. Warnings clear on attack, interruption and death, with a reserved three-entry pool.

# Action Edition 0.2 — combat polish

- Blue-white trails follow the active Keyblade swing; finishers add gold shock rings and stars.
- Each spell has a colored casting halo. Dodge rolls leave short speed trails; a successful Perfect Dodge has a distinct blue/gold burst.
- Perfect Dodge: narrowly evade an overlapping attack to recover 10 MP and earn an immediate finisher on the next Attack. One reward per roll; normal vulnerability remains.
- Attack buffering survives dodge recovery, and grounded landing inputs are accepted sooner.
- Player hit pauses are shorter, with simultaneous contacts combined only within their own frame.
- The command HUD shows confirmed-hit chains, chain expiry, counter readiness, and magic/technique cooldown progress.
- Reinforcements arrive throughout the fight and reuse suitable lightweight enemies from that encounter. Sparse fights refill faster, and reinforcement positions account for arena walls.
- Effects use a fixed pool and batched drawing to limit overhead in crowds. Original character art, animations, sounds and menu font remain the foundation.

Extract the new package into its own folder and use PLAY.cmd. To carry over an Action Edition save, close the game and copy the previous build's data folder into the new folder. Keep the old copy until the new build runs correctly.

This is a tested development update, not a full campaign certification. See VALIDATION.md for exact results and limitations.
