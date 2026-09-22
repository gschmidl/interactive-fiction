#!/bin/sh
# Mystery Mansion revision 9 (see build.bat): disc image, SIMH command file,
# SIMH's hp2100.exe from $HP2100 or the eXo copy.
set -e
cd "$(dirname "$0")"
mkdir -p .build
python src/mkdisc.py .build/disc0.img
cp src/rte.sim .build/rte.sim
: "${HP2100:=/e/EXO/Colossal Cave Adventure (1976)/0385-Point Adventure/HP-2100/hp2100.exe}"
[ -f .build/hp2100.exe ] || cp "$HP2100" .build/hp2100.exe
echo "Built .build/disc0.img; play.bat starts the game."
