#!/bin/sh
# ======================================================================
#  Build the port of Dungeon V2.0 for the Burroughs B7700 (CSL/1000
#  release 2213, contribution A072: "not edited to work on the HP-1000").
#
#    src/tftape.py   reads the release tape inside
#                    ../archive_original/CSL-1000_Rev-2213.zip
#    src/convert.py  takes &DUNGN (the B7700 FORTRAN source) and DUNTXT
#                    (its text) off the tape and writes .build/dungeon.f
#                    and .build/ZORK_DBTXT; every edit is counted
#    src/brt.f90     the port's run-time: the terminal, the files of
#                    CHANGE, INQUIRE PRESENT and CLOSE DISP, the clock,
#                    a stand-in for RANDOM
#    src/brtc.c      the program's folder, a console or not, mkdir
#
#  Every INTEGER, REAL and LOGICAL is 8 bytes (a B7700 word is 48 bits,
#  six characters); -fno-automatic keeps local variables between calls.
#  Then the game is run once with no input, as its installer did: that
#  builds its data base, ZORK_PTXT and ZORK_PINDX, beside dungeon.exe.
#
#  Needs gfortran, gcc and python on PATH.
# ======================================================================
set -e
cd "$(dirname "$0")"

python src/convert.py > .build-counts.txt || { cat .build-counts.txt; exit 1; }
mkdir -p .build
mv .build-counts.txt .build/counts.txt
FC="gfortran -O0 -std=legacy -fdefault-integer-8 -fdefault-real-8 -fno-automatic"
$FC -ffixed-line-length-none -fallow-argument-mismatch -w -c -o .build/dungeon.o .build/dungeon.f
$FC -Wall -c -J .build -o .build/brt.o src/brt.f90
gcc -O2 -Wall -c -o .build/brtc.o src/brtc.c
gfortran -o dungeon.exe .build/dungeon.o .build/brt.o .build/brtc.o
cp .build/ZORK_DBTXT ZORK_DBTXT
rm -f ZORK_PTXT ZORK_PINDX
./dungeon.exe --saves=.build/saves < /dev/null > .build/first-run.txt
test -s ZORK_PTXT && test -s ZORK_PINDX
echo "built dungeon.exe and its data base (ZORK_DBTXT, ZORK_PTXT, ZORK_PINDX); run play.bat"
