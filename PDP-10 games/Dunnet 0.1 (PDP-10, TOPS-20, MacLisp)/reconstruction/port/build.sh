#!/bin/sh
# Builds dunnet.exe with mingw-w64 (x86_64-w64-mingw32-gcc), e.g. in WSL.
set -e
cd "$(dirname "$0")"
python3 src/mksources.py
x86_64-w64-mingw32-gcc -std=c99 -O2 -Wall -Wextra -Wno-clobbered -s -static \
    -Wl,--stack,67108864 -o dunnet.exe src/dunnet.c
echo "built dunnet.exe - start run.bat"
