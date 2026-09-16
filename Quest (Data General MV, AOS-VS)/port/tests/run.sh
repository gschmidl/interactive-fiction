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
exit $fail
