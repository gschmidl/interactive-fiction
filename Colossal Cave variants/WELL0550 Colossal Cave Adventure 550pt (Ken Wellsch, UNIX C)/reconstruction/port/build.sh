#!/bin/sh
# ======================================================================
#  Build munge.exe and adv.exe from Ken Wellsch's 1985 C sources, then
#  compile the A-code database (ADV_DB) into adv.key and adv.rec.
#
#  Needs a MinGW-w64 gcc on PATH (Strawberry Perl's works).  The sources
#  are K&R C with 8.3 upper-case names, so gcc is told they are C (-x c).
# ======================================================================
set -e
cd "$(dirname "$0")"
CFLAGS="-x c -std=gnu89 -w -fcommon -O2 -include ../portcompat.h"
for d in KIO MUNGE ADV; do
    (cd src/$d && rm -f *.o && gcc $CFLAGS -c *.C)
done
KIO=$(ls src/KIO/*.o | grep -v EFFIC)
gcc -static -s -o bin/munge.exe src/MUNGE/*.o $KIO
gcc -static -s -o bin/adv.exe src/ADV/*.o $KIO
rm -f src/*/*.o bin/adv.key bin/adv.rec
./bin/munge.exe < COMCAVE > /dev/null
mv adv.key adv.rec bin/
echo "built bin/adv.exe, adv.key and adv.rec; start bin/run.bat"
