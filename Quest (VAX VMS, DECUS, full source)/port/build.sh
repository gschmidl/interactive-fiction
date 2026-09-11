#!/bin/sh
#  Build QUEST.  gfortran from Strawberry Perl is fine; any gfortran >= 9 works.
#
#  -fno-pad-source is NOT optional: QUEST continues its message strings
#  across source lines, and VMS text files are variable length records, so
#  the literal resumes at the start of the continuation.  gfortran's
#  default pads every fixed-form line to the full line length first, which
#  buries each continuation under sixty spaces.
set -e
cd "$(dirname "$0")"
FC=${FC:-gfortran}
CC=${CC:-gcc}
FFLAGS="-std=legacy -fno-pad-source -ffixed-line-length-132 -fdollar-ok
        -fno-automatic -fno-align-commons -O2 -w"
CFLAGS="-O2 -Wall -Wno-unused-result"

mkdir -p build
for f in lib quest quest1 quest2 quest3 dndop vmsf; do
    $FC -c $FFLAGS -Isrc -Jbuild -o build/$f.o src/$f.f
done
for f in vmsrt keyed main; do
    $CC -c $CFLAGS -Isrc -o build/$f.o src/$f.c
done
$FC -o quest.exe build/*.o
echo "built $(pwd)/quest.exe"
