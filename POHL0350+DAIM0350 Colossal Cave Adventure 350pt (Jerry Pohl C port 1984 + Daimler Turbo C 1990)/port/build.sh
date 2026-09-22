#!/bin/sh
# ======================================================================
#  Build the Windows ports of the Jaeger/Pohl C Adventure (1984) and of
#  Daimler's Turbo C 2.0 version (1990).  For each of the two:
#
#    1. stage the four text files with DOS line endings (CR LF), as they
#       were on the PC-SIG disk: the programs seek to byte offsets that
#       ftell() reported in text mode, which the Windows C library only
#       gets right for CR LF files
#    2. build advent0.exe, the authors' own utility, and let it index the
#       text files into advtext.h
#    3. build advent.exe with that header
#    4. stage   pohl\advent.exe  +  advent1..4.txt
#               daimler\advent.exe + advent1..4.txt
#
#  Needs a MinGW-w64 gcc on PATH (Strawberry Perl's works).
# ======================================================================
set -e
cd "$(dirname "$0")"

# -std=gnu89        K&R C: implicit int, old-style definitions
# -funsigned-char   typed bytes above 127 stay inside the ctype tables
# -fcommon          as the 1980s linkers merged tentative definitions
# -fwrapv           the game's random number generator relies on long overflow
# -fno-builtin      no library routine is replaced by a gcc built-in (the
#                   sources redeclare several with 1984 types)
CF="-std=gnu89 -funsigned-char -fcommon -fno-builtin -fwrapv -O2 -Isrc/port"

build() {                       # $1 variant dir  $2 original dir  $3 define
                                # [$4 text folder  $5 folder to stage in]
    V=$1
    TXT=${4:-../src_original/$2}
    STAGE=${5:-$V}
    OUT=.build/$V${4:+-alt}
    rm -rf "$OUT"
    mkdir -p "$OUT"
    cp src/$V/*.c src/$V/*.h "$OUT/"
    # a DOS text file ends at its Ctrl-Z; left in, it upsets the Windows C
    # library's text-mode ftell() by two bytes in the buffer that holds it,
    # and advent0 would index the last 1.5 KB of the file wrongly
    for t in 1 2 3 4; do
        awk '/^\032/ { exit } { sub(/\r$/, ""); printf "%s\r\n", $0 }' \
            "$TXT/advent$t.txt" > "$OUT/advent$t.txt"
    done
    gcc $CF -D$3 -c -o "$OUT/winport.o" src/port/winport.c
    gcc $CF -D$3 -include src/port/portcompat.h -o "$OUT/advent0.exe" \
        "$OUT/advent0.c" "$OUT/winport.o"
    # advent0's main() falls off its end (K&R), so its exit status is noise
    ( cd "$OUT" && ./advent0.exe > advent0.log ) || true
    grep -q "idx4" "$OUT/advtext.h" || { echo "advent0 failed"; exit 1; }
    gcc $CF -D$3 -include src/port/portcompat.h -o "$OUT/advent.exe" \
        "$OUT/advent.c" "$OUT/database.c" "$OUT/english.c" "$OUT/itverb.c" \
        "$OUT/turn.c" "$OUT/verb.c" "$OUT/winport.o"
    mkdir -p "$STAGE"
    cp "$OUT/advent.exe" "$OUT"/advent?.txt "$STAGE/"
    echo "built $STAGE/advent.exe"
}

# tests\crossdos.py builds a copy with the text files Daimler's DOS program
# was built for:  ADVENT_VARIANT=daimler ADVENT_TEXTS=DIR ADVENT_STAGE=DIR
if [ -n "$ADVENT_TEXTS" ]; then
    case "$ADVENT_VARIANT" in
    pohl)    build pohl    POHL0350 PORT_POHL    "$ADVENT_TEXTS" "$ADVENT_STAGE" ;;
    daimler) build daimler DAIM0350 PORT_DAIMLER "$ADVENT_TEXTS" "$ADVENT_STAGE" ;;
    *)       echo "ADVENT_VARIANT is pohl or daimler"; exit 2 ;;
    esac
    exit 0
fi

build pohl    POHL0350 PORT_POHL
build daimler DAIM0350 PORT_DAIMLER
