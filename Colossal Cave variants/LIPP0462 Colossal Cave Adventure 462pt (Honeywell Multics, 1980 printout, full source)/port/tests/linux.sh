#!/bin/sh
# Build the port's own source (.build/adv462w.f and src/adv462_win.c) for
# Linux with Debian's gfortran and gcc, let it make its new-game image as
# build.sh does, and play the runs of tests/fuzz.py with it:
#
#     python tests\fuzz.py                  (Windows: makes the runs)
#     wsl sh tests/linux.sh [DIR]           (DIR as in fuzz.py)
#     python tests\fuzz.py --compare DIR
#
# The Linux program is only a yardstick for the Windows one - the same
# program built by another compiler for another system - not part of the port.
set -e
cd "$(dirname "$0")/.."
D=${1:-.build/fuzz}
L=.build/linux
FFLAGS="-O -std=legacy -fdec-char-conversions -fdefault-integer-8"
FFLAGS="$FFLAGS -ffixed-line-length-none -fno-range-check -w"

rm -rf "$L"
mkdir -p "$L"
gfortran $FFLAGS -c .build/adv462w.f -o "$L/adv462w.o"
gcc -O2 -Wall -c src/adv462_win.c -o "$L/adv462_win.o"
gfortran -o "$L/adv462" "$L/adv462w.o" "$L/adv462_win.o"
cp adventure.data "$L/"
( cd "$L" &&
  printf 'yes\ndwarf\nno\ndwarf\nno\nno\nno\n\n\n\nno\n' |
  ADV462_CLOCK="18000 600" ./adv462 > setup.log )
if cmp -s "$L/adventure.newgame" adventure.newgame; then
    echo "adventure.newgame: byte for byte the Windows program's"
else
    echo "adventure.newgame: differs from the Windows program's"
fi
cp adventure.newgame "$L/"
python3 tests/fuzz.py --replay "$D" --exe "$L/adv462" --tag linux
