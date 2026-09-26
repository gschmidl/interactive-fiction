#!/bin/sh
# Regenerate the six translated FORTRAN sources in port/src from the
# untouched DECUS sources in src_original.  src/vmsf.f, src/qstcom.inc and
# the three C files are written for the port and are NOT regenerated.
set -e
cd "$(dirname "$0")/../.."
for f in lib quest quest1 quest2 quest3 dndop; do
    python port/tools/detab.py "src_original/$f.for" "port/src/$f.f" >/dev/null 2>&1
done
python port/tools/portify.py
