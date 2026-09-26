#!/bin/sh
# Mystery Mansion revision 9 (see build.bat): disc image, SIMH command file,
# SIMH's hp2100.exe (the one $HP2100 names, else PATH's).
set -e
cd "$(dirname "$0")"
mkdir -p .build
python src/mkdisc.py .build/disc0.img
cp src/rte.sim .build/rte.sim
if [ ! -f .build/hp2100.exe ]; then
  : "${HP2100:=$(command -v hp2100.exe || command -v hp2100 || true)}"
  if [ -z "$HP2100" ]; then
    echo "build: no hp2100.exe - set HP2100 to SIMH's hp2100.exe (V3.12), or put it on PATH" >&2
    exit 1
  fi
  cp "$HP2100" .build/hp2100.exe
fi
echo "Built .build/disc0.img; run.bat starts the game."
