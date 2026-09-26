#!/bin/sh
TMPDIR=${TMPDIR:-/tmp}
# Replay every scripted session against each of the three builds and check
# that they agree with each other and with the stored transcript.
#
# The game seeds its random numbers from the time of day and the date, so
# -t pins the clock and -D the day; without them two runs a minute (or a
# day) apart take different dwarf rolls.  With both pinned the run is exact,
# which is what makes this a test.  The transcripts in docs/ were recorded
# on 2026-09-03, and only that date reproduces all three.
cd "$(dirname "$0")/.." || exit 1
T=21:15
D=2026-09-03
fail=0
for t in tests/*.in; do
    name=$(basename "$t" .in)
    for b in 1979 1981 1978; do
        ./bin/advent-$b.exe -q -t $T -D $D < "$t" | sed 's/[ \t]*$//' > "$TMPDIR/adv-$name-$b.out"
    done
    if cmp -s "$TMPDIR/adv-$name-1979.out" "$TMPDIR/adv-$name-1981.out" &&
       cmp -s "$TMPDIR/adv-$name-1979.out" "$TMPDIR/adv-$name-1978.out"; then
        echo "ok    $name -- 1979, 1981 and 1978 agree byte for byte"
    else
        echo "FAIL  $name -- the three builds disagree"; fail=1
    fi
    if cmp -s "$TMPDIR/adv-$name-1979.out" "docs/transcript-$name.txt"; then
        echo "ok    $name -- matches docs/transcript-$name.txt"
    else
        echo "FAIL  $name -- differs from docs/transcript-$name.txt"; fail=1
    fi
done
exit $fail
