#!/bin/sh
# ======================================================================
#  Build the port of the unnamed volcano adventure - the North Star BASIC
#  file DBACK on 101DISK.NSI - run by the BASIC it was written for.
#
#    1. src/mkdata.py cuts from the disk image (../archive_original):
#       HYBASIC (North Star BASIC, loaded at 2D00H), the DOS file (its data
#       at 2000H), DBACK, and DBACK listed as HYBASIC's LIST shows it
#    2. src/volcano.c + src/z80.c: a Z80 running HYBASIC; North Star DOS's
#       terminal routines and the disk's sector counter are done natively
#    3. check: HYBASIC, given the listing, must tokenise it back into DBACK
#       byte for byte
#
#  Needs MinGW-w64 gcc and python.
# ======================================================================
set -e
cd "$(dirname "$0")"

OUT=.build
PY=python
command -v $PY >/dev/null 2>&1 || PY=python3

rm -rf "$OUT"
mkdir -p "$OUT"
$PY src/mkdata.py ../archive_original/101DISK.NSI "$OUT/data.h"
gcc -O2 -Wall -I"$OUT" -o "$OUT/volcano.exe" src/volcano.c src/z80.c
"$OUT/volcano.exe" --check
cp "$OUT/volcano.exe" .
echo "built volcano.exe; start run.bat"
