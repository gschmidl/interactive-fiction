#!/bin/sh
# ======================================================================
#  Build ABENTEUER - the German Colossal Cave of a Harris VULCAN site:
#  Gary Palter's portable Adventure as HCSD adapted it to the Harris
#  (Dick Reynolds, 1977), its messages translated.
#
#    1. src/convert.py turns the FORTRAN of the job stream J.ADV (cut out
#       of the FAST save fast.tap, ../src_original) into gfortran source;
#       every edit is counted
#    2. that, and the port's Harris run-time (src/port/pharris.f,
#       pharrisc.c), are compiled into abenteuer.exe
#    3. ADV.DATA, the database the program reads when it sets itself up,
#       is the section files TAPE1 ... TAPE1012 from the same save, in
#       section order; src/edition1980.py writes ADV1980.DATA, the same
#       with five later edits undone - the edition NEUSPIEL was set up
#       from in 1980 (abenteuer --fresh=1980)
#    4. src/neuspiel.py turns the site's own NEUSPIEL - the game as the
#       site had it set up, cut out of fast.tap - into NEUSPIEL.DAT, in
#       this port's layout
#
#  Needs MinGW-w64 gcc and gfortran on PATH, and python.
# ======================================================================
set -e
cd "$(dirname "$0")"

OUT=.build
DB=../src_original/database
FC="gfortran -O0 -std=legacy -fno-automatic -fwrapv -fno-align-commons -fallow-argument-mismatch -Werror=line-truncation"
CC="gcc -O2 -Wall"
PY=python
command -v $PY >/dev/null 2>&1 || PY=python3

rm -rf "$OUT"
mkdir -p "$OUT/src" "$OUT/obj"
$PY src/convert.py
cp src/port/*.f src/port/*.c "$OUT/src/"
cd "$OUT/obj"
$CC -c ../src/pharrisc.c
for f in ../src/*.f; do
    $FC -c "$f" 2>> fortran.log
done
$FC -static -o ../abenteuer.exe *.o

# The program's own RAN, AND, OR and XOR were renamed so that gfortran's
# intrinsics of those names do not stand in for them; nothing else of the
# program's may have been taken by one.
for s in kran_ kand_ kor_ kxor_ shift_ lowsix_; do
    nm *.o | grep -q " T $s\$" || { echo "build.sh: $s not defined"; exit 1; }
done
if nm main.o | grep " U _gfortran_" | grep -v -E \
    "_gfortran_(st_|transfer_|stop_|string_|compare_string|concat_string|os_error|runtime_error|set_args|set_options|generate_error|exit_i4|iand|ishft)"; then
    echo "build.sh: main.o calls a gfortran intrinsic routine (above)"
    exit 1
fi

# the same with subscripts checked, for tests/fuzz.py
mkdir -p ../checked
cd ../checked
$CC -c ../src/pharrisc.c
for f in ../src/*.f; do
    $FC -fcheck=bounds,do,mem,pointer,recursion -c "$f" 2>> fortran.log
done
$FC -static -o abenteuer.exe *.o

cd ../..
cat "$DB/TAPE1.txt" "$DB/TAPE2.txt" "$DB/TAPE3.txt" "$DB/TAPE4.txt" \
    "$DB/TAPE5.txt" "$DB/TAPE6.txt" "$DB/TAPE7-9.txt" "$DB/TAPE1012.txt" \
    > ADV.DATA
$PY src/edition1980.py "$DB" ADV1980.DATA
if [ -f src/neuspiel.py ]; then
    $PY src/neuspiel.py ../archive_original/fast.tap "$OUT/src" NEUSPIEL.DAT
fi
cp "$OUT/abenteuer.exe" .
echo "built abenteuer.exe; run play.bat"
