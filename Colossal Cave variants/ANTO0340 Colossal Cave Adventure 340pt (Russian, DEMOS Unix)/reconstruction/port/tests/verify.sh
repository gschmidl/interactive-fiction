#!/bin/sh
# ======================================================================
#  Windows-side verification of the port.
#
#  Every expected/ transcript was produced by a native Linux build of the
#  UNTOUCHED 1985 sources (see linux-reference.sh), driven through a real
#  tty, with both builds using the same fixed RNG (-DTESTRAND).  If the
#  port changed any game behaviour, these diffs show it.
#
#  Run from a Git-Bash / MSYS shell:   sh tests/verify.sh
# ======================================================================
set -e
cd "$(dirname "$0")/.."
fail=0
say() { printf '%-46s %s\n' "$1" "$2"; }

sh build.sh --test > /dev/null 2>&1

# --- 1. the database generator ----------------------------------------
#     adv.text/adv.data are byte streams with no host-dependent layout;
#     a correct ini produces exactly what the Unix one did.
cmp -s text-revised/adv.text ../linux_reference/adv.text \
  && say "adv.text (revised) vs Linux build" "IDENTICAL" \
  || { say "adv.text (revised) vs Linux build" "DIFFERS"; fail=1; }
cmp -s text-revised/adv.data ../linux_reference/adv.data \
  && say "adv.data vs Linux build" "IDENTICAL" \
  || { say "adv.data vs Linux build" "DIFFERS"; fail=1; }

# --- 2. the KOI8-R console encoder ------------------------------------
#     what WriteConsoleW would receive, checked against Python's codec
build/koi8test.exe adv.text build/adv.text.utf16
python - <<'PY' || fail=1
raw  = open("adv.text", "rb").read()
got  = open("build/adv.text.utf16", "rb").read()
want = raw.decode("koi8-r").replace("\n", "\r\n").encode("utf-16-le")
print("%-46s %s" % ("console encoder over the whole message file",
                    "IDENTICAL" if got == want else "DIFFERS"))
raise SystemExit(0 if got == want else 1)
PY

# --- 3. gameplay ------------------------------------------------------
run() {                                  # $1 script, $2 expected, $3 label
    ( cd build/db-1985 && ../ad_t.exe ) < "tests/$1" > "build/$3.out" 2>&1
    if cmp -s "build/$3.out" "tests/expected/$2"; then say "$3" "IDENTICAL"
    else say "$3" "DIFFERS"; diff -u "tests/expected/$2" "build/$3.out" | head -20; fail=1; fi
}
run script-basic.txt  basic.txt  "40-command transcript vs Linux"
run script-stress.txt stress.txt "406-command transcript vs Linux"

# save, restart, restore -- and the save file itself must match Linux's
rm -f build/db-1985/adv.frozen build/adv.frozen
( cd build/db-1985 && ../ad_t.exe ) < tests/script-save-a.txt >  build/save.out 2>&1
( cd build/db-1985 && ../ad_t.exe ) < tests/script-save-b.txt >> build/save.out 2>&1
if cmp -s build/save.out tests/expected/save-restore.txt
then say "save / restart / restore vs Linux" "IDENTICAL"
else say "save / restart / restore vs Linux" "DIFFERS"; fail=1; fi

[ "$fail" = 0 ] && echo "all checks passed" || echo "SOME CHECKS FAILED"
exit $fail
