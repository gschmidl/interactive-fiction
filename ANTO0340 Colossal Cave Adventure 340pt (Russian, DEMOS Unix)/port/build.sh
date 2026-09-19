#!/bin/sh
# ======================================================================
#  Build the Windows port of the Russian 340-point "ПРИКЛЮЧЕНИЕ"
#  (DEMOS Unix, 1984-85) and stage the playable folder.
#
#    ini.exe        reads cave/adv_*  ->  adv.text  adv.data  adv.common
#    adventure.exe  plays the game against those three files
#
#  Needs a MinGW-w64 gcc on PATH (Strawberry Perl's works).
#  Pass  --test  to additionally build the deterministic-RNG binaries
#  used by tests/verify.sh.
# ======================================================================
set -e
cd "$(dirname "$0")"

OUT=build
SRC=src

# -std=gnu89   the sources are K&R C from 1984-85
# -fcommon     they rely on tentative definitions being merged across files
# -fno-builtin no library routine may be replaced by a compiler intrinsic
# -f*-charset  bytes in string literals must survive verbatim: they are KOI8-R
CF="-std=gnu89 -fcommon -fno-builtin -O2 -I$SRC/port \
    -finput-charset=ISO-8859-1 -fexec-charset=ISO-8859-1 \
    -Wno-incompatible-pointer-types"
GAME="-include $SRC/port/portcompat.h"

mkdir -p "$OUT"

build_pair() {                      # $1 suffix, $2 extra flags, $3 rng source
    # The port layer must NOT get the redirection macros: it is what they call.
    gcc $CF $2 -DPORT_IMPLEMENTATION -c -o "$OUT/winport$1.o" "$SRC/port/winport.c"
    gcc $CF $2 $GAME -o "$OUT/ad$1.exe" \
        "$SRC"/adv/*.c "$SRC"/common/savecm.c "$SRC"/common/vocab.c $3 \
        "$OUT/winport$1.o"
    gcc $CF $2 $GAME -o "$OUT/ini$1.exe" \
        "$SRC"/init/*.c "$SRC"/common/savecm.c "$SRC"/common/vocab.c $3 \
        "$OUT/winport$1.o"
}

build_pair "" "" ""
gcc -O2 -o "$OUT/koi8test.exe" tests/koi8test.c "$OUT/winport.o"

if [ "$1" = "--test" ]; then build_pair "_t" "-DTESTRAND" "$SRC/port/testrand.c"; fi

# --- generate the two databases ---------------------------------------
#     ini writes into the current directory, exactly as it did under Unix
gendb() {                           # $1 cave source dir, $2 output dir
    rm -rf "$OUT/$2"; mkdir -p "$OUT/$2"
    cp "$1"/* "$OUT/$2/"
    ( cd "$OUT/$2" && ../ini.exe > ini.log 2>&1 )
    rm -f "$OUT/$2"/adv_*
}
gendb "$SRC/cave"         db-1985
gendb "$SRC/cave_revised" db-revised

# --- stage the playable folder ----------------------------------------
cp "$OUT/ad.exe"  adventure.exe
cp "$OUT/ini.exe" ini.exe
cp "$OUT"/db-1985/adv.common "$OUT"/db-1985/adv.data "$OUT"/db-1985/adv.text .
mkdir -p text-revised
cp "$OUT"/db-revised/adv.common "$OUT"/db-revised/adv.data \
   "$OUT"/db-revised/adv.text text-revised/

echo "built adventure.exe + database; run play.bat"
