# Kingdom Hearts Chain of Memories: Zwebti Mix

This recovered checkout belongs only to **Kingdom Hearts Chain of Memories: Zwebti Mix**.

- The user explicitly forbids touching any other Kingdom Hearts project. Do not inspect, edit, rename, publish, or coordinate those projects. Do not use the upstream collaboration/art queue from this private fork.
- Use **Composure**, with the user's existing GitHub connection, for all GitHub work. If its tools are unavailable, preserve local work and report that exact blocker; do not switch accounts or providers.
- Intended repository slug: `kingdom-hearts-chain-of-memories-zwebti-mix`. Verify owner and existing repositories through Composure before creating anything. Never push to ReKHCoM upstream or another Kingdom Hearts repository.
- Preserve original Downloads packages and player data. ROMs, saves, personal settings, logs, credentials, memory dumps and toolchains must never be committed or uploaded.
- Game files live in immutable version folders; player profiles remain outside those folders. Failed updates must retain the previous active version.
- Read launcher/README.md and launcher/VALIDATION.md for launcher work. Source under mod/action-edition/development describes the recovered combat build and its limits.
- English is the user's current conversation language. Keep the project clean and concise.

## Local development and Norton

- Keep Norton enabled. Never change antivirus settings, add exclusions, restore quarantined files or suppress security alerts automatically. The user performs any narrowly scoped exception in Norton.
- Use `python port/windows64/develop.py` for future Windows development in this project. Native binaries, launcher staging and child-process temporary files use `build/Kingdom-Hearts-Chain-of-Memories-Zwebti-Mix-Development`. This is a build folder, not a security sandbox.
- Reuse verified unchanged builds through the content cache. Rebuild the guest alone for guest-only gameplay changes; do not create fresh hosts, GUI probes or installer EXEs for every check. Reuse stable target names.
- If Norton blocks/quarantines a file, stop that execution and identify the exact file/alert for review. Do not rename, repack or repeatedly regenerate it to get around the block.
- Saves, installed release versions, Downloads, other projects and the rest of this workspace remain outside the development-folder exception. Release files copied outside it still receive normal protection.
