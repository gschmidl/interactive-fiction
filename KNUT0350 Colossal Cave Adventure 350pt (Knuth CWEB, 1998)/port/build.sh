#!/bin/sh
# ======================================================================
#  Build the Windows console port of ADVENT by Don Woods and Don Knuth
#  (CWEB, 1998; KNUT0350).
#
#    1. ctangle  ../src_original/KNUT0350/advent.w  +  src/advent-win.ch
#                -> .build/advent.c      (advent.w itself is never edited)
#    2. gcc      .build/advent.c  ->  advent.exe
#
#  Needs a MinGW-w64 gcc on PATH (Strawberry Perl's works) and a working
#  ctangle: the native one if it runs, otherwise the one in WSL Debian
#  (package texlive-binaries).  MiKTeX's ctangle refuses to start while
#  its user and administrator updates are out of step.
# ======================================================================
set -e
cd "$(dirname "$0")"

OUT=.build
mkdir -p "$OUT"
cp ../src_original/KNUT0350/advent.w "$OUT/advent.w"
cp src/advent-win.ch "$OUT/advent-win.ch"
rm -f "$OUT/advent.c"

( cd "$OUT" && ctangle advent.w advent-win.ch advent.c > ctangle.log 2>&1 ) || true
if [ ! -s "$OUT/advent.c" ]; then
    ( cd "$OUT" && MSYS_NO_PATHCONV=1 wsl -d Debian -- ctangle advent.w advent-win.ch advent.c > ctangle.log 2>&1 )
fi
if [ ! -s "$OUT/advent.c" ]; then
    cat "$OUT/ctangle.log"; echo "ctangle failed"; exit 1
fi

# -std=gnu89        the program is 1998 C: K&R definitions, implicit int main,
#                   and an enum that names false and true (keywords in C23)
# -funsigned-char   8-bit input stays inside the C library's ctype tables,
#                   as it does with glibc; ASCII behaves exactly as before
gcc -std=gnu89 -funsigned-char -O2 -Wall -Wno-implicit-int -Wno-char-subscripts \
    -Wno-return-type -Wno-switch -Wno-unused-variable -Wno-parentheses -Wno-dangling-else \
    -o advent.exe "$OUT/advent.c"

echo "built advent.exe; run play.bat"
