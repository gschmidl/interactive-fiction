#!/bin/sh
# ======================================================================
#  Build the Windows port of adv462: Adventure 1.2 (462 points), the
#  Colossal Cave of Honeywell's Phoenix Multics, 1980 (LIPP0462).
#
#    1. src/mkwin.py makes .build/adv462w.f from Jim Lippard's Unix source
#       (../src_original/adv462/unix/adv462.f) with the port's three edits
#    2. compile it and src/adv462_win.c, the site routines, into a static
#       adv462.exe that needs no DLLs beyond Windows' own
#    3. run adv462.exe once with the wizard's answers: that is how the game
#       builds adventure.newgame, the database read in and saved
#
#  Meant for WSL (Debian with gfortran-mingw-w64, gcc-mingw-w64 and python3),
#  which cross-compiles a native Windows program; build.bat starts it there,
#  and step 3 runs the exe through WSL's Windows interop.  In an MSYS2 or Git
#  Bash shell with a native MinGW-w64 gfortran on PATH it works as it is.
#  FC, CC and NM may be set to choose the compilers.
# ======================================================================
set -e
cd "$(dirname "$0")"

if [ -z "$FC" ]; then
    if command -v x86_64-w64-mingw32-gfortran >/dev/null 2>&1; then
        FC=x86_64-w64-mingw32-gfortran
        CC=x86_64-w64-mingw32-gcc
        NM=x86_64-w64-mingw32-nm
    else
        FC=gfortran
    fi
fi
: "${CC:=gcc}" "${NM:=nm}"
PY=python3
command -v $PY >/dev/null 2>&1 || PY=python

# As in the Unix Makefile: -fdefault-integer-8 gives the 36-bit Multics
# integers room; the rest let gfortran accept 1970s Fortran.
FFLAGS="-O -std=legacy -fdec-char-conversions -fdefault-integer-8"
FFLAGS="$FFLAGS -ffixed-line-length-none -fno-range-check -w"
OUT=.build

# ---- 1. generate ---------------------------------------------------------
rm -rf "$OUT"
$PY src/mkwin.py

# ---- 2. compile ----------------------------------------------------------
$FC $FFLAGS -c "$OUT/adv462w.f" -o "$OUT/adv462w.o"
$CC -O2 -Wall -c src/adv462_win.c -o "$OUT/adv462_win.o"

# The program's own RAN and the site's SIZE must be called, not gfortran's
# intrinsics of the same names (mkunix.py declares both EXTERNAL).
$NM "$OUT/adv462w.o" | grep -q " T ran_" || { echo "RAN is missing"; exit 1; }
$NM "$OUT/adv462w.o" | grep -q " U size_" || {
    echo "SIZE went to the intrinsic"; exit 1; }

$FC -static -o "$OUT/adv462.exe" "$OUT/adv462w.o" "$OUT/adv462_win.o"

# ---- 3. let the game build its new-game image ----------------------------
#  With no adventure.newgame the program reads adventure.data, reports its
#  table space and calls maint, which only a wizard may enter and which ends
#  by saving the fresh game.  The answers, as in the Unix Makefile: a wizard?
#  yes; the magic word, dwarf; do you know what I thought it was? no; the
#  challenge, dwarf again; see the hours? change them? a holiday? no; the
#  short game, the magic word and the latency left as they are; a new
#  message of the day? no.  The clock is set so that every build writes the
#  same image; WSLENV hands ADV462_CLOCK on to a Windows program started
#  from WSL, and does nothing elsewhere.
cp ../src_original/adv462/Multics/adventure.data "$OUT/"
printf 'yes\ndwarf\nno\ndwarf\nno\nno\nno\n\n\n\nno\n' |
    ADV462_CLOCK="18000 600" WSLENV="${WSLENV:+$WSLENV:}ADV462_CLOCK" \
    "$OUT/adv462.exe" > "$OUT/setup.log"
grep -q "Initialization completed" "$OUT/setup.log" &&
test -s "$OUT/adventure.newgame" || {
    echo "the game did not save itself - see $OUT/setup.log"; exit 1; }

cp "$OUT/adv462.exe" "$OUT/adventure.data" "$OUT/adventure.newgame" .
mkdir -p saves
ls -l adv462.exe adventure.data adventure.newgame
echo "built adv462.exe; start run.bat"
