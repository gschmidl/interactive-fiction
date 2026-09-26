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

# twoplayers: two players in one world at once (-p), both characters saved
rm -rf tests/work && mkdir tests/work
./aosvs32.exe -Z 1000000000 -T snap -d data -s tests/work \
    -p tests/player2.txt tests/work/player2.snap data/QUEST.PR \
    < tests/player1.txt > tests/work/player1.snap 2>&1
if [ "$record" ]; then :
elif grep -aq GERHARD tests/work/USER_DATA_FILE && grep -aq ALICE tests/work/USER_DATA_FILE &&
     grep -q INVENTORY tests/work/player1.snap && grep -q INVENTORY tests/work/player2.snap
then echo "ok    twoplayers"; else echo "FAIL  twoplayers"; fail=1; fi
rm -rf tests/work

# god: a new player placed in Southwest Dragonia (the frozen clock decides)
# who walks south is done away with.  With --god, GERHARD walks on at
# strength 1024 -- and still does when his strength is set to 1 every time
# DEFEND begins, because then DIED itself is skipped.
godrun() {
    rm -rf tests/work && mkdir tests/work
    ./aosvs32.exe -Z 1000003600 -T snap -d data -s tests/work "$@" data/QUEST.PR \
        < tests/deadly.txt > tests/work/out.snap 2>&1
}
godrun;                          mortal=$(grep -c "Better luck next time" tests/work/out.snap)
godrun --god;                    god=$(grep -c "Better luck next time" tests/work/out.snap)
                                 strong=$(grep -c "Strength 1024" tests/work/out.snap)
godrun --god -P 16D699 180284 1; forced=$(grep -c "Better luck next time" tests/work/out.snap)
if [ "$record" ]; then :
elif [ "$mortal" -ge 1 ] && [ "$god" -eq 0 ] && [ "$strong" -ge 1 ] && [ "$forced" -eq 0 ]
then echo "ok    god"; else echo "FAIL  god (mortal $mortal, god $god, strong $strong, forced $forced)"; fail=1; fi
rm -rf tests/work

# the multiplayer game itself: players over the network, and at consoles
python tests/netplay.py || fail=1
python tests/consoleplay.py || fail=1
exit $fail
