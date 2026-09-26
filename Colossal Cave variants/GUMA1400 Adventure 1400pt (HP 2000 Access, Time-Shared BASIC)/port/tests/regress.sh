#!/bin/sh
# Regression harness for the Adventure ]I[ port.
#
#   sh tests/regress.sh            compare against the recorded transcripts
#   sh tests/regress.sh --record   re-record them
#
# Every script is run against both variants.  The data files are rebuilt
# from scratch first so a run never depends on leftover state, and SAVE
# files written by a previous run are removed.

set -e
here=$(dirname "$0")
root=$(cd "$here/.." && pwd)
exe="$root/advent.exe"
record=0
[ "$1" = "--record" ] && record=1

[ -x "$exe" ] || { echo "build advent.exe first"; exit 1; }

rm -rf "$root/data"
fail=0
total=0

for variant in recon orig; do
    for s in "$here"/scripts/*.txt; do
        name=$(basename "$s" .txt)
        out="$here/expected/$variant-$name.txt"
        rm -f "$root/data/$variant/regress.dat" "$root/data/$variant/mygame.dat" \
              "$root/data/$variant/origsave.dat" "$root/data/$variant/Score.dat" \
              "$root/data/$variant/Bug.dat" 2>/dev/null || true
        got=$("$exe" -v "$variant" -e -q < "$s" 2>&1) || true
        total=$((total + 1))
        if [ "$record" = 1 ]; then
            printf '%s\n' "$got" > "$out"
            echo "recorded $variant/$name"
        elif [ ! -f "$out" ]; then
            echo "MISSING  $variant/$name (run with --record)"
            fail=$((fail + 1))
        elif printf '%s\n' "$got" | diff -q - "$out" > /dev/null; then
            echo "ok       $variant/$name"
        else
            echo "FAIL     $variant/$name"
            printf '%s\n' "$got" | diff - "$out" | head -20
            fail=$((fail + 1))
        fi
    done
done

echo
if [ "$record" = 1 ]; then
    echo "recorded $total transcripts"
else
    echo "$((total - fail))/$total passed"
    [ "$fail" = 0 ] || exit 1
fi
