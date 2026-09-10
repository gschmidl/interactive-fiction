#!/bin/bash
# ======================================================================
#  Regenerate tests/expected/ from a native Linux build of the UNTOUCHED
#  1985 sources.  Run inside WSL (Debian works):
#
#      wsl bash tests/linux-reference.sh
#
#  Three things are needed to make the comparison meaningful:
#
#  1. The originals do not build on a 64-bit host as they stand.  Every
#     undeclared function is assumed to return int, so malloc()'s pointer
#     in get.c is truncated and the game segfaults before its first
#     prompt.  posixfix.h supplies the missing declarations and nothing
#     else -- it is the same minimal repair the port gets from
#     port/portcompat.h.  gcc >= 10 also rejects getans.c's static
#     definitions after their implicit declarations; that is patched in
#     with sed below, exactly as in the ported copy.
#
#  2. rand() must agree.  glibc's and msvcrt's generators differ, so both
#     sides link port/testrand.c and are built with -DTESTRAND.
#
#  3. Input must arrive over a tty.  The game reads stdin with a bare
#     read(0,buf,79): on a terminal that is one line, from a pipe it is
#     79 bytes of whatever is buffered, and every command after the first
#     is thrown away.  ptyrun.py drives it through a real pty.
# ======================================================================
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
PORT="$(dirname "$HERE")"
PROJ="$(dirname "$PORT")"
D="$HOME/advport-reference"

rm -rf "$D"; mkdir -p "$D"
cp -r "$PROJ/src_original/." "$D/"
cp "$PORT/src/port/testrand.c" "$HERE/ptyrun.py" "$HERE/posixfix.h" "$D/"
cd "$D"

sed -i "/^    unsigned rand();$/d" adv/pct.c adv/events.c
sed -i 's/^getans(word1,word2,type1,type2)/static int inp_ini();\nstatic int inp_word();\n\ngetans(word1,word2,type1,type2)/' adv/getans.c

CF="-std=gnu89 -fcommon -fno-builtin -O2 -w -include $D/posixfix.h"
gcc $CF -o ini init/*.c common/savecm.c common/vocab.c
gcc $CF -Drand=port_rand -Dsrand=port_srand -o ad_t \
    adv/*.c common/savecm.c common/vocab.c testrand.c
( cd cave && ../ini > ../ini.log 2>&1 )

for s in basic stress; do
    tr -d '\r' < "$HERE/script-$s.txt" | iconv -f UTF-8 -t KOI8-R > "$D/$s.koi"
    rm -f cave/adv:frozen
    python3 ptyrun.py "$D/ad_t" "$D/$s.koi" "$D/$s.koi.out" "$D/cave" > /dev/null
    iconv -f KOI8-R -t UTF-8 "$D/$s.koi.out" > "$HERE/expected/$s.txt"
    echo "expected/$s.txt  $(stat -c%s "$HERE/expected/$s.txt") bytes"
done

tr -d '\r' < "$HERE/script-save-a.txt" | iconv -f UTF-8 -t KOI8-R > "$D/sa.koi"
tr -d '\r' < "$HERE/script-save-b.txt" | iconv -f UTF-8 -t KOI8-R > "$D/sb.koi"
rm -f cave/adv:frozen
python3 ptyrun.py "$D/ad_t" "$D/sa.koi" "$D/sa.out" "$D/cave" > /dev/null
python3 ptyrun.py "$D/ad_t" "$D/sb.koi" "$D/sb.out" "$D/cave" > /dev/null
cat "$D/sa.out" "$D/sb.out" | iconv -f KOI8-R -t UTF-8 > "$HERE/expected/save-restore.txt"
echo "expected/save-restore.txt  $(stat -c%s "$HERE/expected/save-restore.txt") bytes"

echo
echo "database built by the Linux ini (compare with the Windows one):"
md5sum cave/adv.text cave/adv.data
