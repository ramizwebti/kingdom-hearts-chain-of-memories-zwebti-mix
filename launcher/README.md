# Kingdom Hearts Chain of Memories: Zwebti Mix — Windows build

Version **0.10.1** refreshes this mod's logo, app icon and full project labels while retaining 0.10's combat and SDL ABI. Do not source files from another Kingdom Hearts project.

## Rebuild the game

Use portable LLVM-MinGW (`clang.exe`, `ld.lld.exe`, `llvm-objcopy.exe`) and Python with PyYAML. Dependencies may remain in a local folder.

```powershell
python port/windows64/build_guest_windows.py --toolchain "path/to/llvm-mingw" --python-deps "path/to/python-deps" --rom "path/to/owned-USA-dump.gba" --jobs 3
```

The helper validates the cartridge dump and writes `build/windows64/out/guest-us.bin`; it never packages the dump. Rebuild the matching native Windows host with the extracted local SDL3 SDK and the previously tested SDL runtime:

```powershell
python port/windows64/build_windows64.py --host-only --output "build/branding-host/out" --sdl-runtime "path/to/previous-tested-runtime/SDL3.dll" --test
```

Stage the tested guest with the rebuilt `rekhcom64.exe`, the pinned `SDL3.dll` and `assets/`, retaining original notices in a sibling `licenses/` folder. Put the qualified master wordmark in `assets/branding/`; `launcher/build.py` also maps the authoritative source master there. The 0.10 combat reference is `build/runtime-v010-shipping`; the 0.10.1 branding candidate is `build/runtime-v0101-shipping`. Keep these folders separate. Native Windows startup explicitly declares SDL readiness and converts its wide-character arguments to UTF-8; the host/game bridge and SDL library stay compatible.

## Build the single-file EXE

```powershell
python launcher/build.py --runtime-source "path/to/candidate-runtime" --output "path/to/output"
```

Windows' .NET Framework C# compiler creates **Kingdom Hearts Chain of Memories - Zwebti Mix.exe**. The delivered launcher is named **Kingdom Hearts Chain of Memories - Zwebti Mix 0.10.1.exe**, preserving the earlier launcher. Icon stamping preserves the supplied host's code/data sections.

The embedded release installs under `%LOCALAPPDATA%/Kingdom Hearts Chain of Memories - Zwebti Mix/versions/0.10.1`. Story and Practice stay under `profile/data` and `profile/practice-data`. Opening a newer EXE upgrades an older installation while retaining profiles and prior versions. Failed updates leave the active release unchanged.

The full launcher wordmark is `mod/action-edition/branding/kingdom-hearts-chain-of-memories-zwebti-mix-logo.png`: the Chain of Memories title with the pink **Zwebti Mix** subtitle. App/taskbar icons use the separate heart-only ICO. The build embeds these as `logo.png` and `heart.ico`, and maps the corresponding BMP to `runtime/assets/icon.bmp`. See [brand notes](../mod/action-edition/branding/BRAND_NOTES.md).

## Updates and source

Live updates await this project's verified GitHub connection through **Composure**. Public updates use `updates.json` in the install root with `{"repository":"OWNER/REPOSITORY"}` and a release tagged `v0.10.1` containing `Kingdom-Hearts-Chain-of-Memories-Zwebti-Mix-0.10.1.zip` plus its `.sha256` file. The updater prefers this full project filename and accepts the old `Zwebti-Mix-VERSION.zip` pair for existing releases. It never mixes the two pairs. Configure this only after the intended repository and release are verified. Private-repository authentication is not implemented.

No ROMs, saves, credentials, personal settings, logs, generated binaries or toolchains belong in source or releases. Upstream source and notices retain their terms. See [VALIDATION.md](VALIDATION.md) for current checks and limits; old reports cannot validate a changed guest.
