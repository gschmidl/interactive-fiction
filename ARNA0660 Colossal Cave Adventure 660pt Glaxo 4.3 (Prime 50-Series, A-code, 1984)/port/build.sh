#!/bin/sh
# ======================================================================
#  Build ADVENTURE4 - Mike Arnautov's 660-point Adventure, Glaxo version
#  4.3 (26 Jul 1984) - run by his A-code executive, as on the Prime.
#
#    1. src/convert.py turns EXECUTIVE.F77 and EXECUTIVE.INS.F77 (Prime
#       F77, read as PRIMOS wrote them in ../src_original/ADVENTURE4) into
#       gfortran source; every edit is counted
#    2. that, and the port's PRIMOS run-time (src/port/prime.f, primec.c),
#       are compiled into adventure4.exe
#    3. the four database files ADVINIT1-4.DAT are copied beside it, as
#       the tape has them
#
#  Needs MinGW-w64 gcc and gfortran on PATH, and python.
# ======================================================================
set -e
cd "$(dirname "$0")"

OUT=.build
SRC=../src_original/ADVENTURE4
FC="gfortran -O0 -std=legacy -fdollar-ok -fno-automatic -fwrapv -fno-align-commons -fallow-argument-mismatch"
CC="gcc -O2 -Wall"
PY=python
command -v $PY >/dev/null 2>&1 || PY=python3

rm -rf "$OUT"
mkdir -p "$OUT/src" "$OUT/obj"
$PY src/convert.py "$SRC" "$OUT/src"
cp src/port/*.f src/port/*.c "$OUT/src/"
cd "$OUT/obj"

$CC -c ../src/primec.c
$FC -I../src -c ../src/prime.f
$FC -I../src -c ../src/executive.f 2> executive.log
$FC -static -o ../adventure4.exe executive.o prime.o primec.o

# The executive's own RAND was renamed KRAND so that gfortran's intrinsic
# of that name does not stand in for it; no other intrinsic may have
# taken a routine of the program's.
nm executive.o | grep -q " T krand_$" || { echo "build.sh: krand_ not defined"; exit 1; }
if nm executive.o | grep " U _gfortran_" | grep -v -E \
    "_gfortran_(st_|transfer_|stop_|string_|compare_string|concat_string|os_error|runtime_error|set_args|set_options|generate_error|adjustl|adjustr|exit_i4)" ; then
    echo "build.sh: executive.o calls a gfortran intrinsic routine (above)"
    exit 1
fi

cd ../..
for f in ADVINIT1.DAT ADVINIT2.DAT ADVINIT3.DAT ADVINIT4.DAT; do
    cp "$SRC/$f" "$f"
    cmp -s "$SRC/$f" "$f"
done
cp "$OUT/adventure4.exe" .
echo "built adventure4.exe; run play.bat"
