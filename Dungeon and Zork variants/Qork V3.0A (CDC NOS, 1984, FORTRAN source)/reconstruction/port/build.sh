#!/bin/sh
# ======================================================================
#  Build Qork V3.0A - S. O. Lidie's CDC NOS implementation of the DECUS
#  Dungeon, "created 84/05/30", from the FTN5 source.
#
#    1. src/convert.py edits qork.src into gfortran source (every edit
#       asserts how often it matches); the port's CDC layer is
#       src/port/pqork.f (display code, 6/12 terminal I/O, FTN5's
#       library) and src/port/pqorkc.c (the COMPASS module RIO)
#    2. qork.exe --build does what the site did with sense switch 1 off:
#       reads qork.txt (DATBAS) into the random text file qork.dat and
#       saves the initial state as qork.ini
#
#  Needs MinGW-w64 gcc and gfortran on PATH, and python.
# ======================================================================
set -e
cd "$(dirname "$0")"

OUT=.build
FC="gfortran -O1 -std=legacy -fdefault-integer-8 -fdefault-real-8"
FC="$FC -ffixed-line-length-none -fno-automatic -fwrapv -fno-align-commons"
CC="gcc -O2 -Wall"
PY=python
command -v $PY >/dev/null 2>&1 || PY=python3

rm -rf "$OUT"
mkdir -p "$OUT/src" "$OUT/obj"
$PY src/convert.py
cp src/port/*.f src/port/*.c "$OUT/src/"
cd "$OUT/obj"
$CC -c ../src/pqorkc.c
for f in ../src/*.f; do
    $FC -c "$f" 2>> fortran.log || { cat fortran.log; exit 1; }
done
$FC -static -o ../qork.exe *.o

# FORLIB's names must be the port's: nothing of the program may have
# been taken by a gfortran intrinsic of the same name
if nm qork.o | grep " U _gfortran_" | grep -v -E \
    "_gfortran_(st_|transfer_|stop_|string_|compare_string|concat_string|os_error|runtime_error|set_args|set_options|generate_error|exit_i8|exit_i4|pow_)"; then
    echo "build.sh: qork.o calls a gfortran intrinsic routine (above)"
    exit 1
fi

cd ../..
cp "$OUT/qork.exe" .
cp ../src_original/qork.txt qork.txt
./qork.exe --build
echo "built qork.exe, qork.dat and qork.ini; start run.bat"
