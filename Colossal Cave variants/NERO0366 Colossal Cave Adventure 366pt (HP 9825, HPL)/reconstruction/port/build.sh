#!/bin/sh
# ======================================================================
#  Build the Windows console port of NERO0366, Adventure for the HP 9825
#  (Eric Nelson and Craig Robinson, HP Desktop Computer Division, 1980):
#
#    1. src/hpl2c.py translates the HPL programs ADVENT, A1, A2 and A3,
#       as the 9825 listed them (../listings), to C
#    2. src/mkdata.py turns the data files on the disk (../adventure.dsk)
#       into C tables, so advent.exe needs no files of its own
#    3. compile them with the runtime (src/rt.c) into one 32-bit exe that
#       uses only msvcrt.dll and kernel32.dll, so it runs on Windows 7
#       (no Universal CRT needed) as well as 10 and 11
#
#  Needs MinGW-w64 gcc with 32-bit libraries (Strawberry Perl's has
#  them) and python.
# ======================================================================
set -e
cd "$(dirname "$0")"

OUT=.build
PY=python
command -v $PY >/dev/null 2>&1 || PY=python3
CC="gcc -m32 -O2 -Wall -Wno-unused-label -Wno-unused-variable -Wno-unused-but-set-variable -fno-strict-aliasing"

rm -rf "$OUT"
mkdir -p "$OUT"
$PY src/hpl2c.py ../listings "$OUT/game.c"
$PY src/mkdata.py ../adventure.dsk "$OUT/data.c"

# Link against msvcrt.dll (libmsvcrt-os), not the Universal CRT that this
# toolchain's libmsvcrt.a stands for, and tell the headers so.
CRT="-D__MSVCRT_VERSION__=0x0700"
LIBS="-nodefaultlibs -lmingw32 -lgcc -lmingwex -lmsvcrt-os -lkernel32 -lmingw32 -lgcc -lmingwex -lmsvcrt-os -lkernel32"
$CC $CRT -Isrc -c src/rt.c -o "$OUT/rt.o"
$CC $CRT -Isrc -c "$OUT/game.c" -o "$OUT/game.o"
$CC $CRT -Isrc -c "$OUT/data.c" -o "$OUT/data.o"
$CC -static -o advent.exe "$OUT/rt.o" "$OUT/game.o" "$OUT/data.o" $LIBS
strip advent.exe
mkdir -p saves
echo "built advent.exe"
