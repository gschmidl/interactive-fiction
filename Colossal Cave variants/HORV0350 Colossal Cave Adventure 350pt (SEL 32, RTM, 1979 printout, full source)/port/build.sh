#!/bin/sh
# ======================================================================
#  Build the Windows port of the SEL 32 / RTM Adventure 350 (Horvath and
#  Norwood, 1978, from the printout of 1979-03-21 transcribed by Arthur
#  O'Dwyer):
#
#    1. src/convert.py splits the printout into program units, drops the
#       job control and the curatorial markings, and applies the edits
#       gfortran needs (each one asserts how often it matches)
#    2. compile those, plus the port's own four support modules
#    3. run the game once with the wizard's answers, which is how the
#       program itself builds newgame2.sav and adv.line out of adv.data
#
#  Needs MinGW-w64 gcc and gfortran on PATH (Strawberry Perl has both)
#  and python for the converter.
# ======================================================================
set -e
cd "$(dirname "$0")"

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
$CC -c pselc.c
for f in *.f; do $FC -c "$f"; done

# The program's own OR, XOR, RAN and LOWSIX must be called, not gfortran's
# intrinsics of the same names.
for s in or_ xor_ ran_ lowsix_ code1_ lines_ size_ addr_; do
    nm *.o | grep -q "T $s" || { echo "missing routine $s"; exit 1; }
done
nm adventur.o | grep -q "U ran_" || { echo "RAN went to an intrinsic"; exit 1; }
nm adventur.o | grep -q "U size_" || { echo "SIZE went to the intrinsic"; exit 1; }

$FC -static -o ../advent.exe *.o
cd ../..

cp "$OUT/advent.exe" "$OUT/adv.data" .
mkdir -p saves
rm -f newgame2.sav adv.line

# ---- 3. let the game build itself ------------------------------------
#  With no newgame2.sav the program reads adv.data, builds the text and the
#  tables, and then calls MAINT - which only a wizard may enter, and which
#  ends by saving the fresh game.  The answers: the magic word DWARF, the
#  challenge (--auto walks past it), no change to the hours, no holiday, the
#  lengths left alone, no message of the day.
./advent.exe --auto --day 0 --time 1000 > "$OUT/setup.log" <<'EOF'
y
DWARF
n
n
n
n



n
EOF
grep -q 'HAS BEEN SAVED' "$OUT/setup.log" || {
    echo "the game did not save itself - see $OUT/setup.log"; exit 1; }
ls -l newgame2.sav adv.line
echo "built advent.exe; start run.bat"
