#!/bin/sh
# Replays scripted sessions with the clock frozen (-Z) and compares each
# transcript with the one recorded in tests/expected.  Run from anywhere:
#
#     sh tests/run.sh        check
#     sh tests/run.sh -r     re-record (only after reading the new output!)
#
# Every session runs in a throwaway save directory; data/ is only read.
cd "$(dirname "$0")/.." || exit 1
EXE=./aosvs16.exe
record=
[ "$1" = "-r" ] && record=1
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
rc=0

check() {
    if [ "$record" ]; then
        cp "$tmp/$1.out" "tests/expected/$1.out"; echo "recorded $1"
    elif cmp -s "$tmp/$1.out" "tests/expected/$1.out"; then
        echo "ok   $1"
    else
        echo "FAIL $1"; diff "tests/expected/$1.out" "$tmp/$1.out" | head -20; rc=1
    fi
}

# tour:  xyzzy, the bird and the snake, five treasures home by plugh, score.
# death: falling in the dark, reincarnation, brief, hours.
for t in tour death; do
    mkdir -p "$tmp/$t"
    $EXE -Z -d data -s "$tmp/$t" data/ADVENTURE.PR < "tests/$t.txt" > "$tmp/$t.out" 2>&1
    check $t
done

# SAVE writes MYGAME (2414 bytes) and stops, and nothing else lands in the
# save directory.  "adventure MYGAME" resumes it -- and deletes it, as the
# original did, so a suspended game can be resumed once.
mkdir -p "$tmp/sr"
$EXE -Z -d data -s "$tmp/sr" data/ADVENTURE.PR < tests/save.txt > "$tmp/save.out" 2>&1
check save
if [ "$(ls "$tmp/sr")" != "MYGAME" ] || [ "$(wc -c < "$tmp/sr/MYGAME")" -ne 2414 ]; then
    echo "FAIL after SAVE the save directory holds: $(ls "$tmp/sr")"; rc=1
fi
$EXE -Z -d data -s "$tmp/sr" data/ADVENTURE.PR MYGAME < tests/restore.txt > "$tmp/restore.out" 2>&1
check restore
if [ -n "$(ls "$tmp/sr")" ]; then
    echo "FAIL after resuming the save directory holds: $(ls "$tmp/sr")"; rc=1
fi
exit $rc
