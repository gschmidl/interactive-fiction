#!/bin/sh
# ======================================================================
#  Build the Windows port of the MCAUTO Cyber 74 Adventure - Tony
#  Jarrett and Paul Zemlin's December 1978 conversion of Blackett's
#  FORTRAN IV, which asks whether you want the 350 point cave (BEG) or
#  the 500 point cave with the castle and the Black Wizard (ADV).
#
#    1. src/convert.py edits ADVENT.txt into one file gfortran will take
#       (every edit asserts how often it matches), turns the two
#       databases into databs1.txt and databs2.txt, and writes the
#       wizard's parameter file amaint.dat
#    2. compile that with the port's own CDC layer, src/port/pjaze.f
#
#  Built -fdefault-integer-8 because the program keeps ten display code
#  characters in a word, and -ffixed-line-length-132 because packing a
#  Hollerith constant into a call makes lines longer than 72 columns.
#
#  Needs MinGW-w64 gcc and gfortran on PATH (Strawberry Perl has both)
#  and python for the converter.
# ======================================================================
set -e
cd "$(dirname "$0")"

OUT=.build
FC="gfortran -O2 -std=legacy -fdefault-integer-8 -fno-automatic"
FC="$FC -ffixed-line-length-132"
CC="gcc -O2 -Wall"
PY=python
command -v $PY >/dev/null 2>&1 || PY=python3

rm -rf "$OUT"
$PY src/convert.py
cp src/port/*.f src/port/*.c "$OUT/src/"
cd "$OUT/src"

$CC -c pjazec.c
for f in *.f; do $FC -c "$f"; done

# SHIFT, MASK, OR, XOR, COMPL, JDATE, CLOCK, OPENMS/READMS/WRITMS and the
# program's own RAN must all come from the port or from the program, never
# from a gfortran intrinsic of the same name.
for s in shift_ mask_ or_ xor_ compl_ jdate_ clock_ readms_ writms_ ran_; do
    nm *.o | grep -q "T $s" || { echo "missing routine $s"; exit 1; }
done
for s in shift_ mask_ clock_; do
    nm advent.o | grep -q "U $s" || { echo "$s went to an intrinsic"; exit 1; }
done
# RAN is the program's own and lives in the same object, so the check is that
# the units calling it declare it EXTERNAL - without that gfortran uses its
# own RAN intrinsic.
test "$(grep -c 'EXTERNAL RAN' advent.f)" = 2 ||
    { echo "EXTERNAL RAN missing"; exit 1; }

$FC -static -o ../advent.exe *.o
cd ../..

cp "$OUT/advent.exe" "$OUT/databs1.txt" "$OUT/databs2.txt" \
   "$OUT/amaint.dat" .
mkdir -p saves
echo "built advent.exe; start run.bat"
