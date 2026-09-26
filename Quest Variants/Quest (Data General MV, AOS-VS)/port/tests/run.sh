#!/bin/sh
# Regression check: replay each scripted session with the clock frozen and
# compare the screens with the ones recorded when the session was verified.
#   tests/run.sh            compare
#   tests/run.sh --record   (re)record the expected screens
cd "$(dirname "$0")/.." || exit 1
mkdir -p tests/expected
fail=0
for s in newplayer moves quit; do
    rm -rf tests/work && mkdir tests/work
    ./aosvs32.exe -Z 1000000000 -T snap -d data -s tests/work data/QUEST.PR \
        < tests/$s.txt > tests/work/$s.snap 2>&1
    if [ "$1" = "--record" ]; then
        cp tests/work/$s.snap tests/expected/$s.snap; echo "recorded $s"
    elif cmp -s tests/work/$s.snap tests/expected/$s.snap; then
        echo "ok    $s"
    else
        echo "FAIL  $s"; diff tests/expected/$s.snap tests/work/$s.snap | head -20; fail=1
    fi
done
# the round trip: a character saved by one session comes back in the next
rm -rf tests/work && mkdir tests/work
./aosvs32.exe -Z 1000000000 -T snap -d data -s tests/work data/QUEST.PR < tests/quit.txt > /dev/null 2>&1
./aosvs32.exe -Z 1000000100 -T snap -d data -s tests/work data/QUEST.PR < tests/relogin.txt > tests/work/relogin.snap 2>&1
if grep -q "Strength 17 \[fighter\]" tests/work/relogin.snap; then echo "ok    relogin"; else echo "FAIL  relogin"; fail=1; fi
rm -rf tests/work

# two players in one world at once (-p): both characters made and saved
rm -rf tests/work && mkdir tests/work
./aosvs32.exe -Z 1000000000 -T snap -d data -s tests/work \
    -p tests/player2.txt tests/work/player2.snap data/QUEST.PR \
    < tests/player1.txt > tests/work/player1.snap 2>&1
if grep -aq GERHARD tests/work/USER_DATA_FILE && grep -aq ALICE tests/work/USER_DATA_FILE &&
   grep -q INVENTORY tests/work/player1.snap && grep -q INVENTORY tests/work/player2.snap
then echo "ok    twoplayers"; else echo "FAIL  twoplayers"; fail=1; fi
rm -rf tests/work

# god: stepping north and south beside Xenobia's tower, GERHARD is shot down
# by the tower guards.  With --god he walks on at strength 1024 -- and still
# does when his strength is set to nothing just where TOWER_ATTACK decides
# (17D3D1), because then DIED itself is skipped.
godrun() {
    rm -rf tests/work && mkdir tests/work
    ./aosvs32.exe -Z 1000000000 -T snap -d data -s tests/work "$@" data/QUEST.PR \
        < tests/deadly.txt > tests/work/out.snap 2>&1
}
godrun;                          mortal=$(grep -c "Better luck next time" tests/work/out.snap)
godrun --god;                    god=$(grep -c "Better luck next time" tests/work/out.snap)
                                 strong=$(grep -c "Strength 1024" tests/work/out.snap)
godrun --god -P 17D3D1 1803E3 0; forced=$(grep -c "Better luck next time" tests/work/out.snap)
if [ "$mortal" -ge 1 ] && [ "$god" -eq 0 ] && [ "$strong" -ge 1 ] && [ "$forced" -eq 0 ]
then echo "ok    god"; else echo "FAIL  god (mortal $mortal, god $god, strong $strong, forced $forced)"; fail=1; fi
rm -rf tests/work

# the multiplayer game itself: players over the network, and at consoles
python tests/netplay.py || fail=1
python tests/consoleplay.py || fail=1
exit $fail
