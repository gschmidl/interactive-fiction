# UNDERGROUND V1 (1979) — paste-ready for RSTS/E

Base: the **14-Mar-79** listing, `07192605` pages 1–6, with the tail recovered
from `07192600` p2, `07192601` p1, `07192604` p1 and `07192606` p3.

Two programs, as the original had:

| File | What it is |
|---|---|
| `UNDERG-V1.BAS` | Main program — parser, verbs, all text, and the world in `DATA` |
| `LOOK-V1.BAS` | Room describer, chained to and from |
| `check_data.py` | Validator for the `DATA` block |

## Verification

Three independent checks, all passing:

**1. The DATA block reads exactly the right number of values.** Line 70 does
`M%(X%,0%)=0% for X%=1% to 87% : O%(0%)=39% : MAT READ M%,O% : X%(0%)=8%`.
`MAT READ` on `DIM M%(87%,14), O%(45)` fills `M%(1..87,1..14)` and `O%(1..45)`
— MAT skips row/column 0, which is why column 0 is zeroed on the same line — so
the block must supply 87 × 14 + 45 = **1263** values.

```
python check_data.py UNDERG-V1.BAS
  87 of 87 room rows present
  object rows: lines [880] -> 45 values (expected 45)
  total DATA values = 1263 (expected 1263)
  RESULT: OK
```

It also range-checks every destination (0, 1..87, or a small negative message
code). This caught a real error: line 80 was first transcribed one comma short,
which would have silently shifted all 86 rooms after it.

**2. Every branch target resolves.** 84 distinct `GOTO`/`GOSUB`/`THEN`/`ELSE`
targets in `UNDERG-V1.BAS` and 10 in `LOOK-V1.BAS`, zero dangling.

**3. Every room has a description.** `UNDERG` chains to `LOOK` at line
`room*100`, plus 50 if the room has been seen. All 87 rooms have both lines
defined. Rooms whose long and short text are identical use a bare `!` line that
falls through to the next — that is the author's own trick, and it is what makes
the maze (rooms 61–71, all falling through to line 7150) work.

A nice consistency check fell out of this: the main program starts at
`X%(0%)=8%`, and `LOOK` line 850 is `&"Outside building"` — which is exactly
where the surviving RUN transcript opens.

## What is reconstructed rather than transcribed

Clearly marked in the files. Everything else is off the paper.

| Lines | Why | Basis |
|---|---|---|
| `LOOK` 4250–4350 | rooms 42–43 fell in the sheet break between `07192607` p3/p4 | only the tail of 4350 survives, reading `…chasm":goto10000`; rooms 44/45 are the bridge and far side, so 43 is the near side |
| `LOOK` 10370–10500 | `07192607` ends mid-line-10350 | object numbering is fixed by UNDERG's vocabulary (37 Gnome … 45 wall); wording follows the run transcript on `07192606` p11 |
| `UNDERG` 3150, 3195, 3200 | end-of-game housekeeping, on a sheet that was not scanned | 3150 mirrors line 1220's `CLOSE2%:KILLSYS(""):GOTO3200`; 3195 is the "no" branch of the lamp offer; 3200 is the exit |
| `UNDERG` 3140 ending | the endgame speech runs past the bottom of `07192600` p2 | transcribed as far as legible, then `:goto3150` |
| `UNDERG` 1390, 1400 | fell in the page break between `07192605` p3/p4 | taken from the 01-Mar-79 listing; they are plain vocabulary lines (`POUR` V%=9, `FILL` V%=10) filling the gap between RUB=8 and READ=11 |

`3200` is stubbed as `END` so the game returns to `Ready` rather than logging
you out. The original almost certainly chained to `$LOGOUT`, as line 3130 does.

## The sheets mix revisions — watch out

`07192604` p2/p3 are clean listings of an **earlier 01-Mar-79** revision that
differs in ways that would misbehave silently:

| Line | 01-Mar-79 | 14-Mar-79 (used here) |
|---|---|---|
| 70 | `MATREADM%,O%:X%(0%)=8%:X%(11%)=30%` | also zeroes `M%(x,0)` and sets `O%(0%)=39%` |
| 992 | `IFR<.3333333THEN3040` | `IFR>.6666666THEN3040` |
| 1000 | `A$(2%)=" "` | `A$(1%)=" "` |
| 2230 | no `O%(9%)=X%(0%)` | sets `O%(9%)=X%(0%)` |

Other sheets (`07192604` p1, `07192600`–`602`) are terminal **editing sessions**
— lines appear several times with `\old\new\` substitutions and `^U`/`^R`
echoes, and only the last form of each is real.

## Pasting into RSTS/E under SIMH

1. Log in to any account. Both `CHAIN` statements now name no account
   (`CHAIN"LOOK.BAC"` and `CHAIN"UNDERG.BAC"`), so they resolve in whatever
   account you are in. Note RSTS/E accounts are `[project,programmer]`, so
   `HELLO 1/2` puts you in `[1,2]` — not `[2,1]`.
2. `NEW` → name it `LOOK` → paste `LOOK-V1.BAS` → `SAVE` → `COMPILE`.
   (Use `REPLACE` rather than `SAVE` if the file already exists.)
   This produces `LOOK.BAC`, which is what `UNDERG` chains to.
3. `NEW` → name it `UNDERG` → paste `UNDERG-V1.BAS` → `SAVE` → `COMPILE`.
   `LOOK` line 1 chains back to `UNDERG.BAC`, so both need compiling.
4. `RUN UNDERG`.

Answer `N` to "Are you recalling a saved game?" on a first run; it allocates
`UNDER<letter>.DAT` for you and tells you the one-letter password to resume with.

Let the terminal settle between large blocks — the `DATA` block is 88 long
lines, and line 1240 is one very long statement.

## Line length: the 255-character limit

BASIC-PLUS caps a source line at 255 characters. A `THEN` clause runs to the
end of its line, so whether an over-long line can be split depends entirely on
whether it contains `THEN`/`ELSE`:

* **No `THEN`/`ELSE`** — the `:`-separated parts are independent statements.
  Split at any top-level `:` (one outside a string) onto consecutive line
  numbers; control falls through, and only the final fragment keeps any
  trailing `goto`.
* **Contains `THEN`/`ELSE`** — everything after it is inside the clause.
  Splitting silently makes those statements unconditional. These need the
  test inverted instead.

### LOOK — already fixed

Lines 5400, 5700, 10010, 10090 and 10200 exceeded the limit and have been
split (into 5400/5401, 5700/5701, 10010/10011, 10090/10091,
10200/10201/10202). All five were plain print sequences, so the split is
mechanical and safe. Longest line is now 245. Re-verified afterwards: all 87
rooms still resolve, no dangling targets.

### UNDERG — also fixed

Six lines exceeded 255 and have been rewritten:

| Line | Was | Now | How |
|---|---|---|---|
| 1240 | 1035 | 1240–1245 | test inverted to `if A$<>"INVEN" then 1250` |
| 1270 | 260 | 1270–1271 | split before the trailing `ifx%<500%then…else…` |
| 2240 | 400 | 2240–2246 | `IF…ELSEIF` chain unrolled to one test per line |
| 2570 | 363 | 2570–2574 | chain unrolled; `T%=5%` inverted to `ifT%<>5%then2600` |
| 3050 | 356 | 3050–3054 | `ELSE` clause inverted after `ifZ$="N"then3070` |
| 3140 | 955 | 3140–3144 | plain prints, mechanical split |

Longest line is now **250**. Verified after rewriting: the ordered list of
printed string literals is byte-identical to the original in all six blocks,
the 2570 THEN-body statement sequence matches exactly, all 86 branch targets
resolve, and the DATA block still reads 1263 values.

Two notes on the unrolled chains. In 2240 and 2570 each arm ends in a jump, so
turning `IF a THEN x ELSE IF b THEN y` into consecutive single tests is
equivalent — a failed test simply falls to the next line, and a failed *last*
test falls through to where the original `ELSE`-less chain went anyway (2250
and 2600 respectively). In 2570 the original's IF head ended `…then&`, and that
trailing `&` is a blank-line print belonging to the record text, not to the
condition; it is preserved as the first statement of 2573.

## Line 975 — a real bug in the 14-Mar revision

Line 20 arms `ON ERROR GOTO 30` to probe for a free `UNDER<letter>.DAT`, and
the 14-Mar-79 listing **never disarms it** — verified against the scan, which
goes `880 DATA` → `970` → `980` with no 975. The earlier 01-Mar-79 listing has
`975 onerrorgoto` (bare, i.e. `ON ERROR GOTO 0`) in exactly that slot.

With the handler left armed, *any* later error is caught by line 30, which
tests `erl-20%`; since the error is no longer at line 20 it takes `resume60`
and prints **"Password not found."** So a failing `CHAIN` at line 990 — a
missing or uncompiled `LOOK.BAC` — is reported as a bogus password error and
loops back to the start.

`975 ON ERROR GOTO 0` has been restored. This is a deliberate deviation from
the 14-Mar listing, on the evidence of the 01-Mar one; without it the program
misreports its own failures.
