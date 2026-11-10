"""Run this mod's development tools in a stable, project-specific build folder.

This does not create a sandbox or change antivirus/security settings. Commands
use existing local tools; player profiles and installed versions are untouched.
"""
import argparse
import os
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[2]
DEVELOPMENT=ROOT/'build/Kingdom-Hearts-Chain-of-Memories-Zwebti-Mix-Development'


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command',choices=('folder','host','host-tests','guest','launcher'))
    parser.add_argument('arguments',nargs=argparse.REMAINDER,
                        help='Extra arguments for the selected project build tool')
    args=parser.parse_args()
    DEVELOPMENT.mkdir(parents=True,exist_ok=True)
    if args.command=='folder':
        print(DEVELOPMENT)
        return 0
    temporary=DEVELOPMENT/'temp'
    temporary.mkdir(parents=True,exist_ok=True)
    environment=dict(os.environ,TEMP=str(temporary),TMP=str(temporary))
    forwarded=args.arguments[1:] if args.arguments[:1]==['--'] else args.arguments
    if args.command in ('host','host-tests'):
        command=[sys.executable,str(ROOT/'port/windows64/build_windows64.py'),'--host-only']
        if args.command=='host-tests': command.append('--test')
    elif args.command=='guest':
        toolchains=sorted(p for p in (ROOT.parent/'toolchain').glob('llvm-mingw-*') if p.is_dir())
        if not toolchains: parser.error('An extracted local LLVM-MinGW toolchain is required')
        command=[sys.executable,str(ROOT/'port/windows64/build_guest_windows.py'),
                 '--toolchain',str(toolchains[-1])]
        dependencies=ROOT.parent/'toolchain/python-deps'
        if dependencies.is_dir(): command+=['--python-deps',str(dependencies)]
    else:
        command=[sys.executable,str(ROOT/'launcher/build.py')]
    # No shell and no retry under a new name if Windows/Norton blocks a tool.
    return subprocess.run(command+forwarded,cwd=ROOT,env=environment,check=False).returncode


if __name__=='__main__':
    raise SystemExit(main())
