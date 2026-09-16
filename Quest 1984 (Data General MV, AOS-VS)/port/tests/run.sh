#!/bin/sh
# Regression check: replay each scripted session with the clock frozen and
# compare the screens with the ones recorded when the session was verified.
#   tests/run.sh            compare
#   tests/run.sh --record   (re)record the expected screens
cd "$(dirname "$0")/.." || exit 1
mkdir -p tests/expected
fail=0
check() {
    if [ "$record" ]; then
        cp tests/work/$1.snap tests/expected/$1.snap; echo "recorded $1"
    elif cmp -s tests/work/$1.snap tests/expected/$1.snap; then
        echo "ok    $1"
    else
        echo "FAIL  $1"; diff tests/expected/$1.snap tests/work/$1.snap | head -20; fail=1
    fi
}
record=; [ "$1" = "--record" ] && record=1

# castle: the title QUEST.CLI typed before the game (no server, stop at once)
rm -rf tests/work && mkdir tests/work
./aosvs32.exe -T snap -c data/CASTLE -q -n 1 -d data -s tests/work data/QUEST.PR \
    < /dev/null > tests/work/castle.snap 2>/dev/null
check castle

# newplayer: create GERHARD, walk N N W W, read the command help, ESC out --
# which makes the server save the character into USER_DATA_FILE
rm -rf tests/work && mkdir tests/work
./aosvs32.exe -Z 1000000000 -T snap -d data -s tests/work data/QUEST.PR \
    < tests/newplayer.txt > tests/work/newplayer.snap 2>&1
check newplayer

# relogin: the same save directory, so GERHARD comes back where he stopped
./aosvs32.exe -Z 1000000100 -T snap -d data -s tests/work data/QUEST.PR \
    < tests/relogin.txt > tests/work/relogin.snap 2>&1
check relogin
rm -rf tests/work
exit $fail
