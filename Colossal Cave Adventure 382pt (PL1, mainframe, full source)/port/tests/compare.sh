#!/bin/bash
# Transcript-diff the C transliteration against the Iron Spring PL/I build.
# The game is deterministic (IX is a static seed and ITIME's value is
# discarded), so identical input must give identical output apart from
# the wall-clock date banner.
#
# usage: cmp.sh <input-file> [label]
set -u
IN="$1"
LABEL="${2:-$(basename "$1")}"

here=$(cd "$(dirname "$0")" && pwd)
PLI=${PLI:-$here/../../port}
CPT=${CPT:-$here/../build}

run_pli() {
  local d=$(mktemp -d)
  cp "$PLI/adventure" "$PLI/OBJECT" "$d/" 2>/dev/null
  python3 -c "open('$d/STORAGE','wb').write(b' '*4800*20)"
  ( cd "$d" && timeout 60 ./adventure < "$IN" 2>&1 )
  rm -rf "$d"
}

run_c() {
  local d=$(mktemp -d)
  cp "$CPT/adventure.exe" "$CPT/OBJECT" "$d/" 2>/dev/null
  python3 -c "open('$d/STORAGE','wb').write(b' '*4800*20)"
  ( cd "$d" && timeout 60 ./adventure.exe < "$IN" 2>&1 )
  rm -rf "$d"
}

# Normalise: strip the ANSI clear-screen, CRs, and the date banner line.
norm() {
  sed -e 's/\x1b\[[0-9;]*[A-Za-z]//g' -e 's/\r$//' \
      -e '/^\(SUNDAY\|MONDAY\|TUESDAY\|WEDNESDAY\|THURSDAY\|FRIDAY\|SATURDAY\)/d' \
      -e 's/[[:space:]]*$//'
}

run_pli | norm > /tmp/out.pli
run_c   | norm > /tmp/out.c

if diff -q /tmp/out.pli /tmp/out.c >/dev/null; then
  echo "MATCH   $LABEL  ($(wc -l < /tmp/out.pli) lines)"
else
  echo "DIFFER  $LABEL"
  diff /tmp/out.pli /tmp/out.c | head -40
fi
