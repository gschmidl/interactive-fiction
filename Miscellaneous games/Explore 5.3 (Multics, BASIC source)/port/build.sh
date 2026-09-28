#!/bin/sh
# ======================================================================
#  Build the Windows port of Explore 5.3 (Jim Lippard, Multics BASIC,
#  1980).  The game is the author's BASIC in share/, unchanged, run by the
#  author's MBasic interpreter and runner (lib/, explore.pl) with the
#  Windows changes in windows.diff.  This makes what goes around them:
#
#    1. explore.exe from src/launcher.c: starts perl/bin/perl.exe on
#       explore.pl
#    2. perl/: the part of Strawberry Perl the game uses, measured by
#       src/mkruntime.pl (run by that Perl)
#    3. windows.diff: the changed files against ../src_original, and a
#       check that share/ is still the author's files byte for byte
#    4. check: a short game through explore.exe, in a scratch folder
#
#  Needs MinGW-w64 gcc and Strawberry Perl: set SPERL to its perl.exe
#  unless the first perl on PATH is a Windows Perl.
# ======================================================================
set -e
cd "$(dirname "$0")"

SPERL=${SPERL:-perl}
if ! "$SPERL" -e 'exit($^O eq "MSWin32" ? 0 : 1)' 2>/dev/null; then
    echo "build.sh: set SPERL to the perl.exe of a Strawberry Perl" >&2
    exit 1
fi

OUT=.build
rm -rf "$OUT"
mkdir -p "$OUT"

# 1-2
gcc -O2 -Wall -municode -s -o "$OUT/explore.exe" src/launcher.c
"$SPERL" src/mkruntime.pl "$OUT/perl"
cp "$OUT/explore.exe" .
rm -rf perl
cp -r "$OUT/perl" perl

# 3
O=../src_original
for f in explore.basic explore.data explore.help hours.data winners.data \
         exp_after_.basic exp_before_.basic exp_cmd_query_.basic \
         exp_day_.basic exp_decode_.basic exp_encode_.basic exp_help_.basic \
         exp_log_.basic exp_search_.basic exp_val_.basic; do
    cmp -s "$O/explore/perl/share/$f" "share/$f" || {
        echo "build.sh: share/$f is not the author's file" >&2
        exit 1
    }
done
{
    for pair in "explore/perl/explore explore.pl" \
                "explore/perl/lib/Explore/Builtins.pm lib/Explore/Builtins.pm" \
                "MBasic/lib/MBasic.pm lib/MBasic.pm"; do
        set -- $pair
        diff -u --label "src_original/$1" --label "port/$2" "$O/$1" "$2" || true
    done
    for f in Arg Env Executor Expr File Interp Lexer Linker Parser Program Registry; do
        diff -u --label "src_original/MBasic/lib/MBasic/$f.pm" \
                --label "port/lib/MBasic/$f.pm" \
                "$O/MBasic/lib/MBasic/$f.pm" "lib/MBasic/$f.pm" || true
    done
} > windows.diff

# 4
T=$(mktemp -d)
printf 'in\nlook\nwhat\nsave check\n' | ./explore.exe --var "$T" > "$T/out1.txt"
printf 'restore check\nscore\nquit\nyes\n' | ./explore.exe --var "$T" > "$T/out2.txt"
grep -q 'Version 5.3' "$T/out1.txt" && grep -q 'small wooden' "$T/out1.txt" &&
    grep -q 'Game saved' "$T/out1.txt" && grep -q 'You scored' "$T/out2.txt" || {
    echo "build.sh: the check game did not run through; see $T" >&2
    exit 1
}
rm -rf "$T"
echo "built explore.exe and perl/; start run.bat"
