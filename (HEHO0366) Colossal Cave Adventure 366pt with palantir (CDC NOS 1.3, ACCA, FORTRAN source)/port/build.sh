#!/bin/sh
# ======================================================================
#  Build the Windows port of the CDC NOS 1.3 Adventure with the gazebo
#  and the palantir - Bill Hein and Shelley Hobson's ACCA version of
#  Blackett and Supnik's FORTRAN IV, 366 points.
#
#    1. src/convert.py edits adventure.src into one file gfortran will
#       take (every edit asserts how often it matches) and copies
#       adventure.txt - TAPE1, the database - out beside the game
#    2. compile that with the port's own CDC layer, src/port/pcdc.f
#
#  Everything is built -fdefault-integer-8, because the program keeps
#  ten display code characters in a word and does arithmetic on them.
#
#  Needs MinGW-w64 gcc and gfortran on PATH (Strawberry Perl has both)
#  and python for the converter.
# ======================================================================
set -e
cd "$(dirname "$0")"

OUT=.build
FC="gfortran -O2 -std=legacy -fdefault-integer-8 -fno-automatic"
CC="gcc -O2 -Wall"
PY=python
command -v $PY >/dev/null 2>&1 || PY=python3

rm -rf "$OUT"
$PY src/convert.py
cp src/port/*.f src/port/*.c "$OUT/src/"
cd "$OUT/src"

$CC -c pcdcc.c
for f in *.f; do $FC -c "$f"; done

# SHIFT, MASK, RANF, RANSET, SECOND, EOF and the program's own ISHFT must
# all come from the port, not from a gfortran intrinsic of the same name.
for s in shift_ mask_ ranf_ ranset_ second_ eof_ ishft_ pdcw_ pdcl_; do
    nm *.o | grep -q "T $s" || { echo "missing routine $s"; exit 1; }
done
for s in shift_ mask_ ranf_ second_ eof_ pdcw_; do
    nm advent.o | grep -q "U $s" || { echo "$s went to an intrinsic"; exit 1; }
done

$FC -static -o ../advent.exe *.o

# The generator test driver: it calls the game's own RND with the seeds the
# machine was measured with (tests/cmpcyber.py checks what it prints).
$FC -static -o ../prnd.exe ../../tests/prnd.f advent.o pcdc.o pcdcc.o
cd ../..

cp "$OUT/advent.exe" "$OUT/adventure.txt" .
mkdir -p saves
echo "built advent.exe; run play.bat"
