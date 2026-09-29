#!/bin/sh
# ======================================================================
#  Build the port of "ADVENTURE  Wandering adventure thru a Cave", the
#  350-point Colossal Cave Adventure of the TI 990 DX10 GAMES library.
#
#  The game is the original task (a TI 990 FORTRAN program) run on an
#  emulated 990/10 with the DX10 supervisor calls it makes:
#    src/cpu990.c   the 990/10 in user mode
#    src/dx10.c     DX10 3.7's supervisor calls, SCI's synonyms, the terminal
#    src/adven.c    the ADVENTUR procedure, options, main
#    src/images.c   the task's two segments and its data file CAVE, from
#                   ../src_original (regenerate with python src/mkimages.py)
#
#  Needs a C compiler on PATH (MinGW-w64 gcc on Windows).  Writes
#  adventure.exe (adventure elsewhere) into this folder.
# ======================================================================
set -e
cd "$(dirname "$0")"

case "$(uname -s 2>/dev/null)" in
MINGW*|MSYS*|CYGWIN*|Windows_NT) EXE=adventure.exe ;;
*) EXE=adventure ;;
esac
[ -n "$OS" ] && [ "$OS" = "Windows_NT" ] && EXE=adventure.exe

CC=${CC:-gcc}
$CC -std=c99 -O2 -Wall -Wextra -o "$EXE" src/cpu990.c src/dx10.c src/adven.c src/images.c
echo "built $EXE; start run.bat"
