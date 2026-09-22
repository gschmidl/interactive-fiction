#!/bin/sh
# ======================================================================
#  Build the Windows port of the PRIMOS FORTRAN Adventure 350 from the
#  SBD003 tape (Gary Palter's portable Adventure as installed on a Prime
#  50-Series, with the Prime's own mixed-case output and its wizard):
#
#    1. src/convert.py splits ADVENTURE.FTN and ADVSUB.FTN into program
#       units and applies the edits gfortran needs (each one asserts how
#       often it matches)
#    2. compile those, plus the port's own two support modules
#    3. convert the Prime's own ADVCOM - the initialised COMMON blocks the
#       game starts from - by swapping the bytes of every word
#
#  Needs MinGW-w64 gcc and gfortran on PATH (Strawberry Perl has both)
#  and python for the converter.
# ======================================================================
set -e
cd "$(dirname "$0")"

ORIG=../src_original/ADVENTURE.UFD
OUT=.build
FC="gfortran -O2 -std=legacy -fdollar-ok -Wall -Wno-unused-dummy-argument"
CC="gcc -O2 -Wall"
PY=python
command -v $PY >/dev/null 2>&1 || PY=python3

rm -rf "$OUT"
$PY src/convert.py
cp src/port/*.f src/port/*.c "$OUT/src/"
cd "$OUT/src"

# ---- 2. compile ------------------------------------------------------
$CC -c pprimec.c
for f in *.f; do $FC -c "$f"; done

# AND, OR and XOR were the Prime's own bit intrinsics and are gfortran's too,
# with the same meaning, so those are left to the compiler.  RAN, SHIFT and
# SIZE are the program's own and must not go to an intrinsic.
for s in ran_ shift_ size_ addr_ code1_; do
    nm *.o | grep -q "T $s" || { echo "missing routine $s"; exit 1; }
done
nm main.o | grep -q "U ran_" || { echo "RAN went to an intrinsic"; exit 1; }
nm main.o | grep -q "U size_" || { echo "SIZE went to the intrinsic"; exit 1; }

$FC -static -o ../advent.exe *.o
cd ../..

cp "$OUT/advent.exe" "$OUT/common" .
mkdir -p saves

# ---- 3. the game as the Prime had it set up --------------------------
$PY tests/advcom.py "$ORIG/ADVCOM/ADVCOM" advcom.dat
echo "built advent.exe; run play.bat"
