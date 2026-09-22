#!/bin/sh
# ======================================================================
#  Build the Windows port of Jonathan Reed's DUNGEON (Prime PL/I subset G,
#  June 1983).  src/dungeon.c is a statement-for-statement transliteration
#  of ../src_original/PL1G_GAMES/DUNGEON.PL1G; src/pl1io.c is the PL/I
#  list-directed input and output it needs, as measured against the
#  original running under PRIMOS 23.4 on p50em.
#
#  Needs a MinGW-w64 gcc on PATH (Strawberry Perl's works).
# ======================================================================
set -e
cd "$(dirname "$0")"

gcc -std=c99 -O2 -Wall -Wextra -Wno-unused-parameter \
    -o dungeon.exe src/dungeon.c src/pl1io.c -lm
echo "built dungeon.exe; run play.bat"
