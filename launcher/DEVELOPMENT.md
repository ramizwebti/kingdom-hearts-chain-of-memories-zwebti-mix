# Kingdom Hearts Chain of Memories: Zwebti Mix — local development

Keep Norton enabled. Development uses one clearly named folder inside this checkout:

`build/Kingdom-Hearts-Chain-of-Memories-Zwebti-Mix-Development`

Native Windows executables use `native/out`; launcher staging and default output use `launcher` and `launcher/out`. Compiler/test temporary files use `temp` only in the development process and its children. This folder is not a virtual machine or security sandbox. The incremental guest builder retains its existing non-Windows-executable object/image paths outside this folder.

Use the GitHub-hosted Windows checks for routine ROM-free combat tests. The `verify-launcher.yml` workflow uses installed Visual C++ to check controller routes, 100,000 invariant steps, interrupted-attacker reservations, feedback and bounded FX; its x86 adapters reuse production targeting/collision code and companion phase gates with checked 32-bit layouts. Python and Node also verify the source inventory and standalone catalog client. No ROM, extracted graphics, local game data, compiler download or account token is required.

Local compiler, helper and native-test launches are currently paused after Norton flagged `feedback.exe`, `pressure.exe`, `routes.exe` and `adapter-collision.exe`. Review the named detections before resuming local execution. Keep Norton enabled; do not add broad exclusions, restore quarantined files, or rename/repackage a blocked executable to make it run. Hosted CI is the default for these source-only checks.

## Local commands after security review

```powershell
python port/windows64/develop.py folder
python port/windows64/develop.py guest --jobs 3
python port/windows64/develop.py host --sdl-runtime "path/to/previous-tested-runtime/SDL3.dll"
python port/windows64/develop.py host-tests --sdl-runtime "path/to/previous-tested-runtime/SDL3.dll"
python port/windows64/develop.py launcher --runtime-source "path/to/tested-candidate-runtime"
```

Rebuild only the guest for guest-only gameplay changes. Stage/test a new candidate before packaging; retain immutable release versions and profiles. Put explicitly chosen native diagnostic output folders below the same development folder. Do not create a new host, GUI probe or installer for each guest test.

## Reuse and verification

Native resource, host, fallback launcher and regression executables have separate content caches. Compiler/SDK contents, relevant sources/headers, generated includes, commands and relevant environment values determine validity. Every cached output is checked by full-file hash. Maintain the native host's explicit transitive port-header list when adding new includes. Changes during compilation prevent publishing a cache record. Missing, modified or quarantined outputs are not reused.

The single-file launcher cache includes the effective archive member map and contents, compiler/config/response/reference inputs, C# sources, icon/stamping helpers and flags. Fixed ZIP metadata prevents file-copy timestamps from changing an otherwise identical payload. An exact existing runtime icon skips resource editing. Unchanged output files remain untouched. `--force` is available for a deliberate rebuild.

The cache is an efficiency feature, not a security verdict. New or changed programs can still be scanned; files exported outside this folder receive normal protection. A blocking/quarantine alert requires review of the actual detection and file. Do not rename/repack or repeatedly regenerate a blocked executable to get around Norton. No security settings, installed game files or player profiles are changed by this setup.

## Validation of this workflow change

The original cache-workflow validation comprised fourteen generic cache tests, ten launcher-orchestration tests and nine resource-stamping tests using Python and non-executable `.bin` fixtures. These cover unchanged-file reuse, full-hash invalidation, missing/corrupt outputs, dependency/member changes, deterministic ZIPs, failed/mid-change builds and icon/resource preservation. Python syntax and command help were checked. Those counts describe that cache change; the expanded hosted combat workflow reports its own results separately.
