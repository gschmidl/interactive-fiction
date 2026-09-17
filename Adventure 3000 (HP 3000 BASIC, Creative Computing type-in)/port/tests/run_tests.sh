#!/bin/sh
# Replay the walkthroughs that were recorded on the real HP 3000 and compare.
# The programs under test are the frozen-RNG copies (RND(0) replaced by .5),
# so both sides take the same branches and the transcripts can be compared
# line for line.
cd "$(dirname "$0")" || exit 1
EXE=../adventure3000.exe
[ -x "$EXE" ] || EXE=../adventure3000
fail=0
for t in b c; do
    rm -f data/*.dat
    "$EXE" --build-data -d data >/dev/null || exit 1
    "$EXE" ADV3000T -p . -d data -e < moves_$t.txt > out_$t.txt 2>&1
    if python compare.py reference_$t.txt out_$t.txt moves_$t.txt > diff_$t.txt; then
        echo "walkthrough $t: matches the HP 3000 ($(wc -l < out_$t.txt) lines)"
    else
        echo "walkthrough $t: DIFFERS"
        head -20 diff_$t.txt
        fail=1
    fi
done
exit $fail
