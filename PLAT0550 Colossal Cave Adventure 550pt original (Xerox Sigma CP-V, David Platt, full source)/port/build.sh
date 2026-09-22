#!/bin/sh
# ======================================================================
#  Build the Windows port of CP-V Adventure - David Platt's, Honeywell
#  Los Angeles Development Center, the 7/20/79 release with the 12/1/79
#  patches, as it came on the LADC SST tapes.
#
#    1. src/convert.py reads the tape image (archive_original/LADC_0012)
#       and writes the munger (MUNGESI) and the interpreter (ADVSI) as
#       gfortran source, and the cave's D: files as keyed files
#    2. compile the munger with the port's run-time and run it over the
#       cave, as COMPILE_CAVE did: that writes ADVT (text) and ADVI (code)
#    3. compile the interpreter
#
#  -O0: the interpreter reads ARGWORDS(1..4) of a 2-element array (the
#  words after it in common), as it did on the Sigma; optimised code
#  may not.  -fwrapv: RND's seed arithmetic overflows and wraps.
#
#  Needs MinGW-w64 gcc and gfortran on PATH and python.
# ======================================================================
set -e
cd "$(dirname "$0")"

OUT=.build
FC="gfortran -O0 -std=legacy -fno-automatic -fwrapv -ffixed-line-length-132"
CC="gcc -O2 -Wall"
PY=python
command -v $PY >/dev/null 2>&1 || PY=python3

rm -rf "$OUT"
$PY src/convert.py
cp src/port/*.f src/port/*.c "$OUT/src/"
cd "$OUT/src"

$CC -c pcpvc.c
$FC -c pcpv.f
$FC -c munge.f
$FC -c adv.f
$FC -c pmungm.f
$FC -c padvm.f

# the Sigma routines must be the port's, not gfortran intrinsics
for s in isl_ isa_ inot_ mash_; do
    nm pcpv.o | grep -q "T $s" || { echo "missing routine $s"; exit 1; }
done
nm adv.o | grep -q "U inot_" || { echo "INOT went to an intrinsic"; exit 1; }
nm adv.o | grep -q "U isa_" || { echo "ISA went to an intrinsic"; exit 1; }
nm munge.o | grep -q "U isl_" || { echo "ISL went to an intrinsic"; exit 1; }

$FC -static -o ../munge.exe pmungm.o munge.o pcpv.o pcpvc.o
$FC -static -o ../adv.exe padvm.o adv.o pcpv.o pcpvc.o
cd ..

# COMPILE_CAVE: the munger over the D: files -> cave/advt.dat, advi.dat
./munge.exe cave > munge.log 2>&1 || true
tail -3 munge.log
grep -q 'STOP Fini' munge.log || { echo "the munger did not finish"; exit 1; }
cd ..

cp "$OUT/adv.exe" "$OUT/cave/advt.dat" "$OUT/cave/advi.dat" .
mkdir -p saves
echo "built adv.exe; run play.bat"
