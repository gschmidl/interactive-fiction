#!/bin/sh
# ======================================================================
#  Build the Windows port of Robert R. Hall's "Generic Adventure --
#  Version:7.0, July 1994" (MINIX advent, 551-point content).
#
#    1. setup.exe (Hall's own) reads advent1..4.txt and writes the
#       encoded advent1..4.dat plus the index header advtext.h
#    2. advent.exe is compiled with that header
#    3. advent.exe and advent1..4.dat are staged in this folder
#
#  Needs a MinGW-w64 gcc on PATH (Strawberry Perl's works).
#  --test also builds .build/advent_t.exe with a fixed random number
#  generator (seed from ADV_SEED) for comparison with a Linux build.
# ======================================================================
set -e
cd "$(dirname "$0")"

OUT=.build
# -std=gnu89      1994 C with K&R definitions (C23 no longer accepts them)
# -D_POSIX_SOURCE as Hall's Makefile
CF="-std=gnu89 -O2 -D_POSIX_SOURCE -Isrc/port"
GAME="advent database english initial itverb score travel turn utility verb vocab"

rm -rf "$OUT"
mkdir -p "$OUT"
cp src/*.c src/*.h "$OUT/"
cp ../src_original/HALL0501/advent?.txt "$OUT/"

gcc $CF -c -o "$OUT/winport.o" src/port/winport.c
gcc $CF -include src/port/portcompat.h -o "$OUT/setup.exe" "$OUT/setup.c" "$OUT/winport.o"
( cd "$OUT" && ./setup.exe )

build_game() {                    # $1 exe name, $2 extra flags
    objs=""
    for f in $GAME; do
        gcc $CF $2 -include src/port/portcompat.h -c -o "$OUT/$f$1.o" "$OUT/$f.c"
        objs="$objs $OUT/$f$1.o"
    done
    gcc $CF $2 -c -o "$OUT/winport$1.o" src/port/winport.c
    gcc -o "$OUT/advent$1.exe" $objs "$OUT/winport$1.o"
}

build_game "" ""
if [ "$1" = "--test" ]; then build_game "_t" "-DTESTRAND"; fi

cp "$OUT/advent.exe" "$OUT"/advent?.dat .
echo "built advent.exe + advent1..4.dat; start run.bat"
