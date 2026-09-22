#!/bin/sh
# Replay every recorded PRIMOS session against the port and diff byte for byte.
# The transcripts were captured from the original DUNGEON.SEG running under
# PRIMOS 23.4 on p50em (see ../README.md); cmp.py normalises only the PRIMOS
# command line and its trailing "OK,".
cd "$(dirname "$0")"
rc=0
for t in ref1 ref3 gold; do
    printf '%-6s ' "$t"
    python cmp.py "cmds_$t.txt" "primos_$t.txt" || rc=1
done
exit $rc
