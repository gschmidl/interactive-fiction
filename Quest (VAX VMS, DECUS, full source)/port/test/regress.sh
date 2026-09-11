#!/bin/sh
#
#  Smoke test for the QUEST port.  Each case drives the game from a script
#  of keystrokes and checks for text the FORTRAN sources say it must
#  print.  The generator seed and the clock are pinned so a run is
#  reproducible.
#
#  Cases that need a particular square of the map use DNDOP, the operator
#  program, exactly as the author would have: it is reachable only from
#  one of the three accounts the sources name, so QUEST_USERNAME and
#  QUEST_UIC are set to his for those.
#
#      ./test/regress.sh
#
#  Transcripts of failures are left in test/out/.
#
cd "$(dirname "$0")/.."
Q=./quest.exe
OUT=test/out
rm -rf "$OUT"; mkdir -p "$OUT"
pass=0; fail=0

fresh() { cp data/character.dta.orig data/character.dta; }

#  make one throwaway fighter called CONAN and park him in Exeter
conan() {
    fresh
    printf 'CKFCONAN\nSECRET\nQ' | QUEST_USERNAME=00CKKELLEY \
        QUEST_UIC=065244 $Q -f -s 4242 >/dev/null 2>&1
}

#  check <name> <file> all|any <pattern>...
check() {
    name=$1; f=$2; mode=$3; shift 3
    hits=0; n=0; miss=
    for pat in "$@"; do
        n=$((n+1))
        if grep -qF "$pat" "$f"; then hits=$((hits+1))
        elif [ -z "$miss" ]; then miss="$pat"; fi
    done
    if { [ "$mode" = all ] && [ $hits = $n ]; } ||
       { [ "$mode" = any ] && [ $hits -gt 0 ]; }; then
        pass=$((pass+1)); printf '  ok    %s\n' "$name"; rm -f "$f"
    else
        fail=$((fail+1)); printf '  FAIL  %s  (no "%s")  see %s\n' "$name" "$miss" "$f"
    fi
}

#  run <name> all|any <keystrokes> <pattern>...
run() {
    name=$1; mode=$2; keys=$3; shift 3
    printf '%b' "$keys" | $Q -f -s 4242 --freeze "1985-04-27 20:30:00" \
        > "$OUT/$name.txt" 2>&1
    check "$name" "$OUT/$name.txt" "$mode" "$@"
}

#  op <name> <dungeon> <level> <x> <y> <keys after arrival> all|any <pattern>...
#  DNDOP:  O -> C -> name -> D dungeon level x y -> 0 (save, run QUEST3)
op() {
    name=$1; d=$2; l=$3; x=$4; y=$5; keys=$6; mode=$7; shift 7
    conan
    #  A, V and T make CONAN strong enough that an incidental monster on
    #  the way in cannot end the case before the square under test is
    #  reached: every statistic 25, level 20, 999 hit points
    printf "OCCONAN\nD%s\n%s\n%s\n%s\nA25\n25\n25\n25\n25\n25\nV20\nT999\n999\n0%b" \
        "$d" "$l" "$x" "$y" "$keys" |
        QUEST_USERNAME=00CKKELLEY QUEST_UIC=065244 \
        $Q -f -s 4242 --freeze "1985-04-27 20:30:00" > "$OUT/$name.txt" 2>&1
    check "$name" "$OUT/$name.txt" "$mode" "$@"
}

echo "QUEST regression"

# ---- the front end --------------------------------------------------
fresh
run intro all 'Q' \
    'Jim, the wizard, appears before you.' \
    'Version 1.00 December 16, 1984.  Version 3.36 April 27, 1985.' \
    'QUEST - written by Chris Kelley' \
    'This game is dedicated to Robert C. Matney' \
    ' QUEST normal termination.'

run menu-help all 'HQ' \
    'The time of day is 20:30' \
    'C - create a character' \
    'F - find experience for the various levels' \
    'Z - logoff'

#  the five characters that were live on the Ball State VAX in 1985
run list all 'P Q' \
    'ISMEL' 'NACERIMA' '1027376' '00AFHOOGENBO' \
    'Character         Class   STR INT WIS CON DEX CHR   Lvl'

run oneuser all 'Y00AFHOOGENBO\n Q' 'PARDUE1' '00AFHOOGENBO'

run experience all 'FF2\nQ' \
    'It takes     2000 experience points to reach level   2'

run experience-all all 'FM0\nQ' \
    'experience points to reach level  20'

run sort all 'S Q' \
    '###  Character        Experience  Level' 'ISMEL'

# ---- character life cycle -------------------------------------------
fresh
run create all 'CKFCONAN\nSECRET\nQ' \
    'Q - to quit,    K - to keep,' \
    'STR   INT   WIS   CON   DEX   CHR' \
    'Character class ("H" for Help) ?' \
    'Enter a name for your character:' \
    'Enter a secret name for your character:' \
    'Welcome to the fair city of Exeter.'

run duplicate all 'CKFCONAN\nSECRET\nQ' \
    'That player already exists. Please try another name.'

run rename all 'NCONAN\nBORIS\nHUSH\nRBORIS\nQQ' \
    'Rename a character' 'Jim: Welcome to Quest, BORIS'

run kill all 'KBORIS\nQ' 'Player has died a sorry death.....'

run notfound all 'RNOSUCHNAME\nQ' 'That character was not found.'

# ---- Exeter ----------------------------------------------------------
fresh
run exeter all 'CKCAHMED\nSECRET\nH' \
    'D - dungeon adventuring' 'M - visit the magic shop' 'Q - stop adventuring'

fresh
run cleric all 'CKCAHMED\nSECRET\nC10\nH' \
    'You have traveled across town to the temple of Ra.' \
    'Ahman, resident cleric, will help you' \
    'W - have wounds healed'

fresh
run shop all 'CKFAHMED\nSECRET\nMH' \
    'A magic shop is' 'greeted by Jim,' \
    'B - buy a magic item' 'R - have a magic item recharged'

fresh
run shop-poor all 'CKFAHMED\nSECRET\nMB' \
    "I'm sorry. You do not have the"

fresh
run badnumber all 'CKFNUMER\nSECRET\nCXX\n' \
    '%QSTOTS - Error on numeric input. Please reenter the number.'

# ---- the dungeon -----------------------------------------------------
fresh
run dungeons all 'CKFDELVE\nSECRET\nDOLLAM\n' \
    'There are numerous dungeons and passages beneath the city of' \
    'OLLAMh Castle          (Beginner)' \
    'TOMBS of Tarasar       (Advanced)' \
    'Name of the dungeon you wish to enter:'

fresh
run toodeep all 'CKFDELVE\nSECRET\nDTOMBS\n' \
    'I cannot let such an inexperienced player into so difficult'

# ---- one square of each kind, reached through DNDOP -------------------
#  the key streams alternate F (fight, if a monster turned up) with the
#  answer the square itself wants, so a case survives either way
op move      1 2  2  3 'FEFSFWFNFEFSFWFN'  all \
    'Direction ("H" for Help) ?' 'East' 'South' 'West' 'North'
op dirhelp   1 2  2  3 'FHFHFH'  all \
    'N - to move north' 'Q - quit and save your character' \
    'A - expert map mode <re>set'

op dndop     1 2  9 12 ''        all \
    'Command:' 'Characters' 'Dungeon: 1' 'Y Coordinate: 12' \
    'Thus endeth time stop.'
op pool      1 2  9 12 'FYFYFYFN' all 'You have found a pool filled with a'
op fountain  1 1  2 18 'FYFYFYFN' all 'You have discovered a fountain shimmering with a'
op throne    1 3 14  4 'FYFYFYFN' all 'You have discovered a throne of'
op pit       1 3 23  4 'FNFNFNFN' any 'You avoided the pit.' 'fallen into a pit'
op telepad   1 1  3 15 'FNFNFNFN' any 'teleporter' 'PIT'
op stairsdn  1 1  3 11 'FNFYFN'   all 'You have found some steps down. Would you like'
op stairsup  1 1  1 14 'FNFNFN'   all 'You have found some steps up. Would you like to'

# ---- monsters, treasure, statistics ----------------------------------
op monster   1 2  9 12 'FFFFFFFFFF' any \
    'You have encountered a' 'What would you like to do ("H" for Help) ?'
op stats     1 2  9 12 'FNXZI'    all \
    'Gold in credit ring' 'Your armor class is:' \
    'You are currently carrying the following'

# ---- the Ball State opening hours ------------------------------------
#  ACCESS.FIL brackets the hours QUEST is closed, every day but Saturday
#  and Sunday.  QUEST.FOR gets the day from MOD(LIB$DAY(),7), and
#  17-NOV-1858 was a Wednesday, so 3 is Saturday and 4 is Sunday.
cp data/access.fil "$OUT/access.saved"
printf '008:0020:00\n' > data/access.fil
for case in '1985-04-24 12:00:00|wed-noon|shut' \
            '1985-04-24 21:00:00|wed-evening|open' \
            '1985-04-27 12:00:00|sat-noon|open' \
            '1985-04-28 12:00:00|sun-noon|open'; do
    when=${case%%|*}; rest=${case#*|}; label=clock-${rest%%|*}; want=${rest#*|}
    printf 'Q' | $Q -f --freeze "$when" > "$OUT/$label.txt" 2>&1
    if [ "$want" = shut ]; then
        check "$label" "$OUT/$label.txt" all \
            'They are securely locked.' \
            'Quest is available before 8:00 a.m. and after'
    else
        if grep -qF 'securely locked' "$OUT/$label.txt"; then
            fail=$((fail+1)); printf '  FAIL  %s  (locked, should be open)\n' "$label"
        else
            check "$label" "$OUT/$label.txt" all ' QUEST normal termination.'
        fi
    fi
done
cp "$OUT/access.saved" data/access.fil
rm -f "$OUT/access.saved"

echo
echo "  $pass passed, $fail failed"
[ $fail = 0 ]
