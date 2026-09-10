#!/bin/bash
# Run every comparison case.  From WSL:
#     bash tests/runall.sh
# Needs the Iron Spring PL/I build in ../port (see ../port/README.md) and
# the C build in ../build (build.bat).
here=$(cd "$(dirname "$0")" && pwd)
fail=0
for t in "$here"/t*.txt "$here"/w*.txt; do
    out=$(bash "$here/compare.sh" "$t" "$(basename "$t")")
    echo "$out"
    case "$out" in DIFFER*) fail=1 ;; esac
done
if [ $fail -eq 0 ]; then
    echo "all cases match"
else
    echo "SOME CASES DIFFER"
fi
exit $fail
