#!/bin/sh
TMPDIR=${TMPDIR:-/tmp}
# Replay every scripted session against each of the three builds and check
# that they agree with each other and with the stored transcript.
#
# The game seeds its random numbers from the time of day, so -t pins the
# clock; without it two runs a minute apart take different dwarf rolls.
# With the clock pinned the run is exact, which is what makes this a test.
cd "$(dirname "$0")/.." || exit 1
T=21:15
fail=0
for t in tests/*.in; do
    name=$(basename "$t" .in)
    for b in 1979 1981 1978; do
        ./bin/advent-$b.exe -q -t $T < "$t" | sed 's/[ \t]*$//' > "$TMPDIR/adv-$name-$b.out"
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
