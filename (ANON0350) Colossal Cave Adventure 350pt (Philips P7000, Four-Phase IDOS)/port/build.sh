#!/bin/sh
# ======================================================================
#  Build the port of "ADVENT UNDER IDOS", the 350-point Colossal Cave
#  Adventure (ADVENTURE 07 JUNE 1978) of a Philips P7000 / Four-Phase
#  System IV site.
#
#  The game is not rewritten: the site's IDOS disc pack is booted on an
#  emulated Four-Phase IV/70 and the game is started as the operator did.
#    src/cpu.c     the IV/70 processor
#    src/io.c      interrupts, the 8231 disc, the 60 Hz clock, keyboards
#    src/advent.c  the front end: console screen / transcript, the operator
#    src/trace.c   an instruction trace (debugging)
#  p7000.pack      the disc pack, copied from ../src_original/P7000.PACK
#
#  Needs a C compiler on PATH (MinGW-w64 gcc on Windows).  Writes
#  advent.exe (advent elsewhere) and p7000.pack into this folder.
# ======================================================================
set -e
cd "$(dirname "$0")"

case "$(uname -s 2>/dev/null)" in
MINGW*|MSYS*|CYGWIN*) EXE=advent.exe ;;
*) EXE=advent ;;
esac
[ "$OS" = "Windows_NT" ] && EXE=advent.exe

CC=${CC:-gcc}
$CC -std=gnu99 -O2 -Wall -Wextra -o "$EXE" src/cpu.c src/io.c src/trace.c src/advent.c
cp ../src_original/P7000.PACK p7000.pack
echo "built $EXE and p7000.pack; run play.bat"
