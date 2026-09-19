#!/bin/bash
# Build script for the SHARE Adventure PL/I port (Iron Spring PL/I / WSL2)
set -e
cd "$(dirname "$0")"
PLIDIR=${PLIDIR:-~/tapecave/pli/pli-1.4.1}
PLIC="$PLIDIR/plic"
INC="$PLIDIR/lib/include"
LIB="$PLIDIR/lib/libprf.a"
FLAGS="-C -dELF -cn(^)"

$PLIC $FLAGS -i "$INC" adventure.pli -o adventure.o
for f in stgwr tread twrite clrscrn randu itime decdate r062a10 warnmsg whisper; do
  $PLIC $FLAGS -i "$INC" "$f.pli" -o "$f.o"
done

# Fixed copy of the runtime's _pli_Bool (see the header of plibool.pli): the
# stock one segfaults at random on "^" applied to bit strings wider than 32
# bits, which adventure.pli does on every vocabulary lookup.  Compiled with
# the same flags the runtime library itself is built with, and linked ahead
# of libprf.a so the archive's own copy is never pulled in.
$PLIC $FLAGS -dLIB -i "$INC" plibool.pli -o plibool.o

ld -z muldefs -Bstatic -o adventure --oformat=elf32-i386 -melf_i386 -e main \
  plibool.o \
  adventure.o stgwr.o tread.o twrite.o clrscrn.o randu.o itime.o decdate.o r062a10.o warnmsg.o whisper.o \
  --start-group "$LIB" --end-group

echo "Build OK: ./adventure"
