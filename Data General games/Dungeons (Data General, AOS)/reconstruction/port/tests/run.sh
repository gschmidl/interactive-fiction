#!/bin/sh
# Replays keystroke scripts with the clock frozen (-Z), which fixes the
# dungeon, the character and every random event, and compares each
# transcript with the one recorded in tests/expected.  Run from anywhere:
#
#     sh tests/run.sh        check
#     sh tests/run.sh -r     re-record (only after reading the new output!)
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

run() {     # name level keys
    $EXE -Z -d data -s "$tmp" data/DG.PR $2 < "tests/$3" > "$tmp/$1.out" 2>&1
    check $1
}

# intro:   level 1 -- enter, help, status, look, the map you cannot read,
#          moves through a steel door, fool's gold, killed by the alchemist.
# levels:  the Dungeon Keeper's welcome and the character at levels 2-4.
# long:    four games at level 3, keys recorded by tools/drive.py.
# noarg:   no level at all, which the CLI macro never allowed -- the program
#          stops with its own message.
run intro 1 intro.keys
for l in 2 3 4; do run level$l $l decline.keys; done
run long 3 long.keys
$EXE -Z -d data -s "$tmp" data/DG.PR < tests/decline.keys > "$tmp/noarg.out" 2>&1
check noarg
exit $rc
