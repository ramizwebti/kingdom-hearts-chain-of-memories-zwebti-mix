#!/usr/bin/env python3
"""Rebuild only the USA x32 game image on Windows, retaining the existing host.

Use a portable LLVM-MinGW toolchain containing clang.exe, ld.lld.exe and
llvm-objcopy.exe. Python requires PyYAML. No installation or system settings
are changed; generated game data and binaries stay in ignored build folders.
"""
import argparse
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
USA_SHA1 = "10729bd884f8fdca7a310b6d606c52e46657aa48"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--toolchain", type=Path, required=True)
    parser.add_argument("--python-deps", type=Path,
                        help="Optional isolated directory containing PyYAML")
    parser.add_argument("--rom", type=Path,
                        help="Your own unmodified USA cartridge dump, copied into ignored roms/")
    parser.add_argument("--jobs", type=int, default=3)
    parser.add_argument("--rebuild", action="store_true")
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    tools = args.toolchain.resolve() / "bin"
    env = dict(os.environ, REKHCOM_GUEST_DUALSENSE="0", REKHCOM_GUEST_LINUX64="1")
    for variable, executable in (("CLANG", "clang.exe"), ("LLD", "ld.lld.exe"),
                                 ("OBJCOPY", "llvm-objcopy.exe")):
        path = tools / executable
        if not path.is_file():
            parser.error("Missing portable compiler tool: " + str(path))
        env[variable] = str(path)
    env["PYTHONUTF8"] = "1"
    if args.python_deps:
        env["PYTHONPATH"] = str(args.python_deps.resolve()) + os.pathsep + env.get("PYTHONPATH", "")
    if args.rebuild:
        env["REBUILD"] = "1"
    if args.rom:
        source = args.rom.resolve()
        digest = hashlib.sha1(source.read_bytes()).hexdigest()
        if digest != USA_SHA1:
            parser.error("The supplied file is not the supported unmodified USA dump.")
        destination = ROOT / "roms/B8CE.gba"
        if destination.exists():
            if hashlib.sha1(destination.read_bytes()).hexdigest() != USA_SHA1:
                parser.error("The existing roms/B8CE.gba differs; it was preserved.")
        else:
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, destination)
    subprocess.run([sys.executable, "-X", "utf8", str(ROOT / "port/ps5/build_ps5.py"),
                    "guest", "--region", "us", "--jobs", str(args.jobs)],
                   cwd=ROOT, env=env, check=True)
    source = ROOT / "port/ps5/build/guest-us/guest.bin"
    output = ROOT / "build/windows64/out/guest-us.bin"
    output.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, output)
    print("Windows game image: " + str(output))
    print("SHA256: " + hashlib.sha256(output.read_bytes()).hexdigest())


if __name__ == "__main__":
    main()
