#!/bin/sh
# Replays scripted sessions with the clock frozen (-Z) -- the game seeds its
# random numbers from it -- and compares each transcript with the recorded
# one.  Every session saves into a throwaway directory; data/ is only read.
#
#     sh tests/run.sh        check
#     sh tests/run.sh -r     re-record (only after reading the new output!)
#
# long:  a walk from the mailbox to the troll with SAVE / RESTORE, an overlong
#        command line and the informational commands.
# hang:  600 random commands.  Before the DIVX fix (2026-09-16) the game hung
#        for ever after about 460 of them -- see NOTES.md.
cd "$(dirname "$0")/.." || exit 1
CLOCK=1789560000                        # 2026-09-16 12:00 UTC
record=
[ "$1" = "-r" ] && record=1
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
rc=0
for t in long hang; do
    mkdir -p "$tmp/$t"
    timeout 300 ./aosvs32.exe -Z $CLOCK -d data -s "$tmp/$t" data/ZORK.PR \
        < "tests/$t.txt" > "$tmp/$t.out" 2>&1
    status=$?
    if [ $status -eq 124 ]; then echo "FAIL $t (did not finish)"; rc=1; continue; fi
    if [ "$record" ]; then
        cp "$tmp/$t.out" "tests/expected/$t.out"; echo "recorded $t"
    elif cmp -s "$tmp/$t.out" "tests/expected/$t.out"; then
        echo "ok   $t"
    else
        echo "FAIL $t"; diff "tests/expected/$t.out" "$tmp/$t.out" | head -20; rc=1
    fi
done
exit $rc
