# Zwebti Mix

<p align="center"><img src="mod/action-edition/branding/zwebti-mix-logo.png" width="480" alt="Zwebti Mix"></p>

Fast, card-free Sora combat for Kingdom Hearts: Chain of Memories on ReKHCoM.

**[Download for Windows](https://github.com/ramizwebti/Zwebti-Mix/releases/latest/download/Zwebti-Mix.exe)**

Open the EXE, choose Story or Practice Arena, and select your own unmodified USA GBA game file when asked. Use **Check for updates** in the launcher for future releases. Progress and controls stay separate from application updates. No ROM is included.

| Action | Key |
|---|---|
| Move | Arrow keys |
| Attack / earned combo finisher | X |
| Jump / dodge | Z / C |
| Cast / change spell | Q / E |
| Practice menu | Backspace |

Connect three basic hits, then press Attack again for Blitz or Air Rush. Practice includes a training target, crowds, Axel and Jafar.

## Build

Supply your owned USA ROM locally and the LLVM-MinGW/SDL3 toolchain described in the [Windows build notes](port/windows64/README.md). Run `python port/windows64/build_windows64.py --regions us --jobs 3`, then `python mod/action-edition/package.py`. Release files are one EXE and its update manifest. The helper-test workflow requires no ROM; it is not a full game build.

## Credits

Built on [MatyMeatBoy/ReKHCoM](https://github.com/MatyMeatBoy/ReKHCoM) and [Pheenoh/khcom](https://github.com/Pheenoh/khcom). Source and third-party notices are preserved. Kingdom Hearts belongs to its respective owners; this is an independent fan modification. Full campaign balance and hardware testing remain in progress.
