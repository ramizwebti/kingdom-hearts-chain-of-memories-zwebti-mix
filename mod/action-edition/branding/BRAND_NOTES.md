# Kingdom Hearts Chain of Memories: Zwebti Mix - branding

The project name is **Kingdom Hearts Chain of Memories: Zwebti Mix**. Version 0.10.1 uses the silver Kingdom Hearts wordmark, the original gold **Chain of Memories** label, and the supplied pink **Zwebti Mix** lettering with blossoms centered directly beneath. The app/taskbar icon is the separate pink heart alone.

## Authoritative assets

| File | Purpose |
|---|---|
| `kingdom-hearts-chain-of-memories-zwebti-mix-logo.png` | Complete transparent wordmark, 1525 x 1031 RGBA |
| `kingdom-hearts-chain-of-memories-zwebti-mix-icon.png` | Transparent pink heart, 1254 x 1254 RGBA |
| `kingdom-hearts-chain-of-memories-zwebti-mix.ico` | Windows heart icon: 16, 20, 24, 32, 40, 48, 64, 96, 128 and 256px |
| `kingdom-hearts-chain-of-memories-zwebti-mix-icon.bmp` | Native-game icon fallback, 256 x 256, 32-bit |

All corners of the PNG masters have zero alpha. Preserve the complete wordmark and its Chain of Memories label; use the separate heart for small icons. Do not reduce the full wordmark into an app icon.

## Runtime mapping

- `launcher/build.py` embeds the wordmark as `logo.png` and the ICO as `heart.ico`, and sets the launcher's Windows icon. These short resource identifiers are internal aliases.
- `launcher/Launcher.cs` shows the full wordmark in its larger hero area; `mod/action-edition/launch.ps1` uses the same artwork in startup and play-mode dialogs. All window and message captions use the full project name.
- The release includes only the four qualified assets above. The BMP maps to `runtime/assets/icon.bmp`, and the wordmark also maps to `runtime/assets/branding/kingdom-hearts-chain-of-memories-zwebti-mix-logo.png`. `launcher/stamp_runtime_icon.py` copies the qualified BMP when staging a runtime and stamps the supplied ICO into Windows icon group 1 without changing executable code/data sections.
- The source README and packaged `START_HERE.html` display the same full wordmark. File and archive names use hyphens where Windows cannot use the title's colon.
- The native title presentation uses the same wordmark through `PortTitleBrandUse`/`PortTitleBrandDraw` in `port/src/port_hdsprites.c`, for Story/Practice and Sora/Riku. It is drawn only after the image is ready, keeps the intro and palette fades, and stays beneath Options menus. If the image is missing, the complete original title remains. Original cartridge resources in `config/assets/sprites_title.yaml` and the optional six `gTitleLogo*` HD entries in `port/data/hd_sprites.tsv` remain unchanged.

The retired `zwebti-mix-*` assets and `zwebti-heart.svg` are not aliases or fallback assets for the 0.10.1 launcher and are excluded from its release.

## Sources and creation

The [Chain of Memories logo](https://www.khwiki.com/images/2/2a/Kingdom_Hearts_Chain_of_Memories_Logo_KHCOM.png) supplied the original game title reference. Built-in image editing composed it with the user's supplied `04_04_17` pink subtitle reference, preserving the silver main wordmark and gold Chain of Memories label. The standalone heart was cleaned from the user's supplied `04_10_59` pink-heart reference. Transparency cleanup was part of the image edit; subsequent icon exports were mechanical. This is an AI-assisted composition, not a claim of pixel-identical reproduction.

Source references were kept unchanged. Existing game names and marks remain their owners' property. This is an independent fan modification. No art from another Kingdom Hearts project is used.

## Validation

Inspect the full image at its actual launcher size and the heart at 16, 24, 32 and 48px. Re-extract the compiled launcher's `logo.png` and `heart.ico` and compare their bytes with these masters. Icon stamping verifies unchanged non-resource sections, preserved non-icon resources, exact re-extracted icon frames, and an unchanged source executable. Check the actual window caption, release ZIP paths/CRC, and retained profiles after the upgrade. Asset/identity changes do not substitute for the existing combat validation.
