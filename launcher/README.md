# Kingdom Hearts Chain of Memories: Zwebti Mix  -  Windows build

Version **0.11.1** improves native combo recovery, opening variations, defense buffering, target continuity, contact feedback and safe assist overlap while retaining the established project artwork and SDL ABI. Do not source files from another Kingdom Hearts project.

## Rebuild the game

For routine local work, use the stable development folder and build reuse described in [DEVELOPMENT.md](DEVELOPMENT.md). Norton stays enabled; any project-folder exception is a manual user setting. The commands below remain available for explicit release staging.

Use portable LLVM-MinGW (`clang.exe`, `ld.lld.exe`, `llvm-objcopy.exe`) and Python with PyYAML. Dependencies may remain in a local folder.

```powershell
python port/windows64/build_guest_windows.py --toolchain "path/to/llvm-mingw" --python-deps "path/to/python-deps" --rom "path/to/owned-USA-dump.gba" --jobs 3
```

The helper validates the cartridge dump and writes `build/windows64/out/guest-us.bin`; it never packages the dump. Rebuild the matching native Windows host with the extracted local SDL3 SDK and the previously tested SDL runtime:

```powershell
python port/windows64/build_windows64.py --host-only --output "build/branding-host/out" --sdl-runtime "path/to/previous-tested-runtime/SDL3.dll" --test
```

Stage the tested guest with the rebuilt `rekhcom64.exe`, the pinned `SDL3.dll` and `assets/`, retaining original notices in a sibling `licenses/` folder. Put the qualified master wordmark in `assets/branding/`; `launcher/build.py` also maps the authoritative source master there. The unchanged 0.10.1 reference is `build/runtime-v0101-shipping`. Stage the new tested runtime in the dedicated development folder and keep the reference intact. Native Windows startup explicitly declares SDL readiness and converts its wide-character arguments to UTF-8; the host/game bridge and SDL library stay compatible.

## Build the single-file EXE

```powershell
python launcher/build.py --runtime-source "path/to/candidate-runtime" --output "path/to/output"
```

Windows' .NET Framework C# compiler creates **Kingdom Hearts Chain of Memories - Zwebti Mix.exe**. The delivered launcher is named **Kingdom Hearts Chain of Memories - Zwebti Mix 0.11.1.exe**, preserving the earlier launcher. Icon stamping preserves the supplied host's code/data sections.

The embedded release installs under `%LOCALAPPDATA%/Kingdom Hearts Chain of Memories - Zwebti Mix/versions/0.11.1`. Story and Practice stay under `profile/data` and `profile/practice-data`. Opening a newer EXE upgrades an older installation while retaining profiles and prior versions. Failed updates leave the active release unchanged.

The full launcher wordmark is `mod/action-edition/branding/kingdom-hearts-chain-of-memories-zwebti-mix-logo.png`: the Chain of Memories title with the pink **Zwebti Mix** subtitle. App/taskbar icons use the separate heart-only ICO. The build embeds these as `logo.png` and `heart.ico`, and maps the corresponding BMP to `runtime/assets/icon.bmp`. See [brand notes](../mod/action-edition/branding/BRAND_NOTES.md).

## Updates and source

The [0.11.1 release](https://github.com/ramizwebti/kingdom-hearts-chain-of-memories-zwebti-mix/releases/tag/v0.11.1) is published through **Composure**. Its source tag matches the hosted packaging commit; all server asset digests and sizes match the verified outputs. The previous 0.10.1 release remains intact. See [PACKAGING.md](PACKAGING.md) for the hosted build and exact-runtime/synthetic-upgrade checks; the local installation also retains all 16 profile files and prior versions.

The launcher defaults to this exact project repository. A nonempty `updates.json` naming another repository is rejected. Public updates use the published release's `Kingdom-Hearts-Chain-of-Memories-Zwebti-Mix-0.11.1.zip` plus its `.sha256` file. The updater prefers this full project filename and accepts the old `Zwebti-Mix-VERSION.zip` pair for existing releases. It never mixes the two pairs. Updates preserve profiles and immutable previous versions. A complete live network update through the launcher must be distinguished from local installation and updater/store checks. Private-repository authentication is not implemented.

No ROMs, saves, credentials, personal settings, logs, generated binaries or toolchains belong in source or releases. Upstream source and notices retain their terms. See [VALIDATION.md](VALIDATION.md) for current checks and limits; old reports cannot validate a changed guest.
