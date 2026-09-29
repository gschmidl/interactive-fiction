#!/bin/sh
# Record the reference games on RetroCore (see reffuzz.py and rebuild.py).
# usage: sh tools/refbatch.sh [FIRST LAST]   (from port\, with RetroCore running)
set -e
first=${1:-1}
last=${2:-12}
mkdir -p tests/ref tests/ref-original
until python -c "import socket; socket.create_connection(('127.0.0.1', 9000), 2)" 2>/dev/null; do sleep 5; done
s=$first
while [ "$s" -le "$last" ]; do
    n=$(printf %02d "$s")
    python tools/reffuzz.py "$s" 150 "tests/ref/game$n"
    if [ "$s" -le 6 ]; then
        python tools/reffuzz.py $((s + 100)) 100 "tests/ref-original/game$n" --original
    fi
    s=$((s + 1))
done
