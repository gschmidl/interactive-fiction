#!/bin/sh
# ======================================================================
#  Build the Windows port of "ADVENTURE - FORTRAN FROM MSU" (Doug Moore,
#  CBT overflow tape COV466 file 119), the way its own JCL did:
#
#    1. src/convert.py turns the PDS members into sources gfortran takes
#       (every edit asserts how often it matches - see the file)
#    2. compile all of them, plus the port's own four support modules
#    3. link advwiz.exe ($WIZLINK), advent.exe ($ADVLINK) and
#       adventst.exe (the same list with ADVENT2, "CHANGE TO ADVENTST
#       FOR TEST VERSION")
#    4. run advwiz to compile ADVTDATA into advent.ini, as installation
#       steps 4-6 of $ADVDOC say to
#
#  Needs MinGW-w64 gcc and gfortran on PATH (Strawberry Perl has both)
#  and python for the converter.
# ======================================================================
set -e
cd "$(dirname "$0")"

ORIG=../src_original/CBT.COV466.FILE119.PDS
OUT=.build
FC="gfortran -O2 -std=legacy -Wall -Wno-unused-dummy-argument"
CC="gcc -O2 -Wall"
PY=python
command -v $PY >/dev/null 2>&1 || PY=python3

rm -rf "$OUT"
$PY src/convert.py
cp src/port/*.f src/port/*.c "$OUT/src/"
cd "$OUT/src"

# ---- 2. compile ------------------------------------------------------
$CC -c pgmdir.c
for f in *.f; do $FC -c "$f"; done

# GETDTM was the one assembler module; the port supplies its own, and
# AND/OR/XOR/SHIFT/RAN must be this program's own routines and not
# gfortran's g77-compatibility intrinsics of the same names.
for s in and_ or_ xor_ shift_ ran_ getdtm_; do
    nm *.o | grep -q "T $s" || { echo "missing routine $s"; exit 1; }
done
nm advent.o | grep -q "U ran_" || { echo "RAN resolved to an intrinsic"; exit 1; }

# ---- 3. link ---------------------------------------------------------
LIB=$(ls *.o | grep -v -E '^(advent|advent2|advwiz)\.o$' | tr '\n' ' ')
$FC -static -o ../advwiz.exe   advwiz.o  $LIB
$FC -static -o ../advent.exe   advent.o  $LIB
$FC -static -o ../adventst.exe advent2.o $LIB
cd ../..

# ---- 4. the database and the initialization file ---------------------
#  ADVTDATA is the database exactly as it is on the tape (fixed 80 column
#  records: number in columns 1-8, text in 9-78).
cp "$ORIG/ADVTDATA.txt" "$OUT/advtdata.txt"
cp "$OUT/advwiz.exe" "$OUT/advent.exe" "$OUT/adventst.exe" .
cp "$OUT/advtdata.txt" .
mkdir -p saves

#  $ADVDOC: "EXECUTE ADVWIZ.  NOW DETERMINE THE WIZARDS ALGORITHM. (GOOD
#  LUCK.)  EXECUTE ADVWIZ AGAIN AND SPECIFY ANY CHANGES YOU WANT."  The
#  answers below are that second run: no prime time (99 is out of range,
#  which is how NEWHRX is told "never"), no holiday, the lengths left
#  alone, no message of the day, and BLKLIN=F as IOINIT sets it.
#  --auto walks past the wizard's guessing game; tests/wizard.py plays it
#  for real and checks that the file comes out identical.
./advwiz.exe --auto > "$OUT/advwiz.log" <<'EOF'
n
y
99
99
99
n



n
F
EOF
grep -q 'INITIALIZATION COMPLETED' "$OUT/advwiz.log" || {
    echo "advwiz did not finish - see $OUT/advwiz.log"; exit 1; }
ls -l advent.ini
echo "built advent.exe; run play.bat"

# ---- 5. the test drivers, if asked for --------------------------------
#  PROBE and PROBEM are not part of the game: they call the routines that
#  only ADVENT calls, so that those can be compared against the same
#  routines running on MVS 3.8j (tests/cmpmvs.py).  They stay in .build.
if [ "$1" = "--tests" ]; then
    $PY tests/mkprobe.py
    cp tests/probe.f tests/probem.f "$OUT/src/"
    cd "$OUT/src"
    $FC -c probe.f probem.f
    LIB=$(ls *.o | grep -v -E '^(advent|advent2|advwiz|probe|probem)\.o$' \
          | tr '\n' ' ')
    $FC -static -o ../probe.exe  probe.o  $LIB
    $FC -static -o ../probem.exe probem.o $LIB
    cd ../..
    echo "built .build/probe.exe and .build/probem.exe"
fi
