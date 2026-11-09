# Validation

The 0.6.0 release changes packaging, desktop launch/update flow and branding. Combat uses the unchanged game image `147f4cd0703045c604cc8928e89be7ad494b6e77e29e76f6c11a3ddc3fa0d893`.

- Gameplay: 33 native scenarios, 4 visual scenes and 7 additional spell-impact cases passed on that image. Actual ender contacts, paused inputs, casts, damage/healing/status, crowds and practice cleanup were checked.
- Bootstrap: 33 cases / 336 assertions passed in actual PowerShell 7.4.6/Linux archive/cache/profile tests cover staged extraction, integrity, invalid paths, partial installs, profile preservation and legacy import.
- Updater: 53 checks passed in actual PowerShell 7.4.6/Linux tests cover pinned release/asset metadata, hashes/sizes, executable identity, replacement/rollback and failure paths.
- Settings migration: 27 real PowerShell IO checks passed.
- Packaging: five IO tests plus actual embedded-payload extraction and cache reuse passed. Allowlisted runtime only; no ROM, saves, SDKs or development archive. Resource and manifest checks compare the built EXE with its embedded payload.
- Windows helper checks run in the repository's Windows launcher workflow. See its actual run result; local Linux tests are not Windows execution.

Current machine-readable reports live under `build/` during development. Run the checked-in test scripts to reproduce them. Full campaign balance, Windows game/gamepad testing and native Perfect Dodge revalidation remain incomplete. Riku retains cartridge combat.
