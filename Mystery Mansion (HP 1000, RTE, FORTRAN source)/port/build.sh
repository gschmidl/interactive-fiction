#!/bin/sh
# ======================================================================
#  Build the Windows port of Mystery Mansion - Bill Wolpert's HP 1000
#  RTE version (FTN4, 23 July 81), from the INTEREX CSL/1000 startup tape.
#
#    1. src/convert.py reads the source off the tape image in
#       archive_original/ and writes it as gfortran source:
#       .build/src/mmmroot.f (the main program, BLOCK DATA, MMRI, MMRL)
#       and .build/src/mmmseg.f (the twelve segments MMSA-MMSL)
#    2. compile both with the port's run-time (src/port/pmmm.f, pmmmc.c)
#
#  The segments are compiled with fresh, zeroed local variables on every
#  call: RTE loaded a segment from the disc each time EXEC 8 asked for
#  it, so nothing but COMMON outlived a segment.  The main program and
#  its subroutines stay in memory and keep theirs (-fno-automatic).
#  -fallow-argument-mismatch: HP INTEGER is 16 bits, and the program
#  hands 16-bit variables and literals to the same routines.
#
#  Needs MinGW-w64 gcc and gfortran on PATH and python.
# ======================================================================
set -e
cd "$(dirname "$0")"

OUT=.build
FC="gfortran -O0 -std=legacy -fno-align-commons -fallow-argument-mismatch"
CC="gcc -O2 -Wall"
PY=python
command -v $PY >/dev/null 2>&1 || PY=python3

rm -rf "$OUT"
mkdir -p "$OUT/src"
$PY src/convert.py
cp src/port/*.f src/port/*.c "$OUT/src/"
cd "$OUT/src"

$CC -c pmmmc.c
$FC -fno-automatic -c pmmm.f
$FC -fno-automatic -c mmmroot.f
$FC -finit-local-zero -fmax-stack-var-size=100000 -c mmmseg.f
$FC -static -o ../mmm.exe pmmm.o mmmroot.o mmmseg.o pmmmc.o
cd ../..
cp "$OUT/mmm.exe" .
mkdir -p saves
echo "built mmm.exe; run play.bat"
