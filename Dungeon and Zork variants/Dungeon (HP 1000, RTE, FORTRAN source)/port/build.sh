#!/bin/sh
# ======================================================================
#  Build the Windows port of Dungeon V3.0a - Tom Hutchinson's HP 1000
#  RTE version (FTN4X, 18 Nov 82), contribution F042 of the INTEREX
#  CSL/1000 release 2240.
#
#    1. src/convert.py reads the sources off the tape image in
#       archive_original/ and writes them as gfortran source
#       (.build/src/dunga.f ... dungl.f), and the messages file @DUNGN
#    2. compile them with the port's run-time (src/port/pdng.f, pdngc.c)
#    3. run the game once: its INIT builds the text file @DUNGT and the
#       index @DUNGI from @DUNGN, as on the first run on the HP
#
#  Everything is compiled -fno-automatic: variables keep their values
#  between calls, as in FTN4X.  -fallow-argument-mismatch: HP INTEGER is
#  16 bits, and the program hands 16-bit variables and literals to the
#  same routines.
#
#  Needs MinGW-w64 gcc and gfortran on PATH and python.
# ======================================================================
set -e
cd "$(dirname "$0")"

OUT=.build
FC="gfortran -O0 -std=legacy -fno-automatic -fwrapv -fno-align-commons -fallow-argument-mismatch"
CC="gcc -O2 -Wall"
PY=python
command -v $PY >/dev/null 2>&1 || PY=python3

rm -rf "$OUT"
mkdir -p "$OUT/src" "$OUT/obj"
$PY src/convert.py
cp src/port/*.f src/port/*.c "$OUT/src/"
cd "$OUT/obj"

$CC -c ../src/pdngc.c
$FC -c ../src/pdng.f
for f in dunga dungb dungc dungd dunge dungf dungl; do
    $FC -c ../src/$f.f 2> $f.log
done
$FC -static -o ../dungeon.exe pdng.o dunga.o dungb.o dungc.o dungd.o \
    dunge.o dungf.o dungl.o pdngc.o

# the library's AND, OR and ITIME were renamed KAND, KOR and ITIMX so that
# gfortran's intrinsics of those names do not stand in for them
for s in kand_ kor_ itimx_; do
    nm dungl.o | grep -q " T $s\$" || { echo "build.sh: $s not defined"; exit 1; }
done

cd ..
rm -f @DUNGT @DUNGI
printf '' | ./dungeon.exe --time 12:00 --date 1982-11-18 > init.log
grep -q "CREATING NEW '@DUNGI'" init.log
cd ..
cp "$OUT/dungeon.exe" "$OUT/@DUNGN" "$OUT/@DUNGT" "$OUT/@DUNGI" .
mkdir -p saves
echo "built dungeon.exe; start run.bat"
