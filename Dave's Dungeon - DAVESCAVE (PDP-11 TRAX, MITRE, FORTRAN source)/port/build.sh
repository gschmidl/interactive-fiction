#!/bin/sh
# ======================================================================
#  Build the Windows port of Dave's Dungeon (DAVESCAVE), Dave Parker's
#  "Dungeons and Dragons", distributed by the MITRE Corporation, version
#  1.0 of September 1980, DEC FORTRAN IV-PLUS.
#
#    1. src/convert.py edits davescave.for into source gfortran will take
#       (every edit asserts how often it matches) and turns TEXTFILE.DAD
#       from PDP-11 segmented records into a gfortran record
#    2. compile that with the port's own run-time, src/port/pdave.f
#
#  The flags are the VAX's: -fdec for the DEC dialect, -fno-align-commons
#  because the VAX never padded a common block (and one routine declares
#  /DANDD2/ shorter than the rest), -fno-pad-source because a tab-format
#  line was taken as typed, and long lines because one of them reaches
#  column 73 and the compiler read it.
#
#  Needs MinGW-w64 gcc and gfortran on PATH (Strawberry Perl has both)
#  and python for the converter.
# ======================================================================
set -e
cd "$(dirname "$0")"

OUT=.build
FC="gfortran -O2 -std=legacy -fdec -fno-automatic -fno-align-commons"
FC="$FC -ffixed-line-length-132 -fno-pad-source"
CC="gcc -O2 -Wall"
PY=python
command -v $PY >/dev/null 2>&1 || PY=python3

rm -rf "$OUT"
$PY src/convert.py
cp src/port/*.f src/port/*.c "$OUT/src/"
cd "$OUT/src"

$CC -c pdavec.c
for f in *.f; do $FC -c "$f"; done

# RAN, SECNDS and ININT must be the port's, not gfortran's intrinsics.
for s in ran_ secnds_ inint_; do
    nm pdave.o | grep -q "T $s" || { echo "missing routine $s"; exit 1; }
    nm davescave.o | grep -q "U $s" || { echo "$s went to an intrinsic"; exit 1; }
done

$FC -static -o ../davescave.exe *.o

# The generator test: the port's RAN for a few seeds, which tests/pran.py
# checks against a model of the VAX instructions of FOR$IRAN.
$FC -static -o ../pran.exe ../../tests/pran.f pdave.o pdavec.o
cd ../..

cp "$OUT/davescave.exe" "$OUT/textfile.dad" .
mkdir -p saves
echo "built davescave.exe; run play.bat"
