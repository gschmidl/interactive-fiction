#!/bin/sh
# Replay the walkthroughs recorded on the real HP 3000 and compare.
#
# The programs under test have RND(0) replaced by .5 so that both sides take
# the same branches:
#   ADV3000T  the program with the author's three bugs corrected   (b, c, d)
#   ADV3000V  the program with every correction in fixes.txt        (e)
# Walkthrough d exercises "take all" / "drop all"; e the place-word parser.
cd "$(dirname "$0")" || exit 1
EXE=../adventure3000.exe
[ -x "$EXE" ] || EXE=../adventure3000
fail=0
for t in b:ADV3000T c:ADV3000T d:ADV3000T e:ADV3000V; do
    w=${t%%:*}; prog=${t#*:}
    rm -f data/*.dat
    "$EXE" --build-data -d data >/dev/null || exit 1
    "$EXE" "$prog" -p . -d data -e < moves_$w.txt > out_$w.txt 2>&1
    if python compare.py reference_$w.txt out_$w.txt moves_$w.txt > diff_$w.txt; then
        echo "walkthrough $w ($prog): matches the HP 3000 ($(wc -l < out_$w.txt) lines)"
    else
        echo "walkthrough $w ($prog): DIFFERS"
        head -20 diff_$w.txt
        fail=1
    fi
done
exit $fail
