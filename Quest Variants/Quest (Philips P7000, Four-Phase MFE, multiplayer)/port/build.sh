#!/bin/sh
# ======================================================================
#  Build the port of "QUEST UNDER MFE", QUEST version 1, the
#  multi-player game of a Philips P7000 / Four-Phase IV/90 site.
#
#  The game is not rewritten: the site's disc pack is booted on an
#  emulated Four-Phase IV/90 Model 2, and MFE/7000 and QUEST are started
#  as the operator did.
#    src/cpu.c     the IV/70 processor with the IV/90 mapper and instructions
#    src/io.c      interrupts, the 8231 disc, the 60 Hz clock, the keyboards
#    src/quest.c   the front end: the operator, the players' screens
#    src/net.c     the players' windows over TCP, the console (Windows)
#    src/trace.c   an instruction trace (debugging)
#  p7000.pack      the disc pack, copied from ../src_original/P7000.PACK
#  QHELP.txt       QUEST's manual, from the pack (../src_original)
#
#  Needs MinGW-w64 gcc on PATH (Windows only: the players' windows use
#  Winsock and the Windows console).  Writes quest.exe, p7000.pack and
#  QHELP.txt into this folder.
# ======================================================================
set -e
cd "$(dirname "$0")"

CC=${CC:-gcc}
$CC -std=gnu99 -O2 -Wall -Wextra -o quest.exe src/cpu.c src/io.c src/trace.c src/quest.c \
    src/net.c -lws2_32
cp ../src_original/P7000.PACK p7000.pack
cp ../src_original/QHELP.txt QHELP.txt
echo "built quest.exe and p7000.pack; start run.bat"
