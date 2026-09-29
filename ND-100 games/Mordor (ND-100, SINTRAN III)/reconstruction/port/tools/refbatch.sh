#!/bin/sh
# Record the reference games of the fixed program on RetroCore (see reffuzz.py).
# usage: sh tools/refbatch.sh [OUTDIR]   (from port\, with RetroCore running;
# OUTDIR defaults to tests/ref)
set -e
out=${1:-tests/ref}
mkdir -p "$out"
until python -c "import socket; socket.create_connection(('127.0.0.1', 9000), 2)" 2>/dev/null; do sleep 5; done
sleep 75
for s in 1 2 3 4 5 6 7 8; do
    python tools/reffuzz.py $s 400 "$out/game0$s" --prog MORDORF-REF
    python tools/reffuzz.py $((s + 50)) 300 "$out/game0${s}b-keep" --prog MORDORF-REF --keep-map
done
for s in 9 10 11 12; do
    python tools/reffuzz.py $((s * 7)) 400 "$out/game$(printf %02d $s)" --prog MORDORF-REF
done
