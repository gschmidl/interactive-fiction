# HORV0350: errors in the transcription, for Arthur O'Dwyer

These turned up while building a working port of HORV0350 straight from your transcription (the game now plays; the
port is in this folder's `port/`). Line numbers are those of `HORV0350/transcribed-code.txt` and
`HORV0350/transcribed-data.txt` in Quuxplusone/Advent at commit d38e825 (2026-09-15).

Nine are claimed as errors: five because the code as transcribed does not compile, four because it compiles but the
game cannot work. Four more are questions only the printout can settle.

## 1. The code does not compile as transcribed

| line | transcribed | should read |
|---|---|---|
| 623 | `INVENT=VOCAB(0+'INVEN'),2)` | `INVENT=VOCAB(CODE1('INVEN'),2)` - the parentheses do not balance. WOOD0350 has `VOCAB(0+'INVEN',2)`, so this looks like a half-finished edit of the WOOD0350 line; the rest of this program writes `CODE1('...')`. |
| 2188 | `.GAVEUP.=TRUE` | `GAVEUP=.TRUE.` (as in WOOD0350) |
| 2572 | `IF LINES(K).GE.0)GOTO 10` | `IF(LINES(K).GE.0)GOTO 10` |
| 3430 | `CVLTUC(TEXT,K)` | `CALL CVLTUC(TEXT,K)` |
| 3654 | `R = R * 16807` | `1     R = R * 16807`. Line 3647 in RAN says `IF (R.NE.0) GOTO 1`, and nothing in RAN has the label 1. This line comes just after the page break at `==p68==`. |

## 2. It compiles, but the game cannot work as transcribed

- **Lines 53-54, `COMMON /MSCCOM/`.** Line 53 ends `...,CLSSES,HNTMAX` with no comma before line 54's `2PLAC,FIXD,...`.
  FORTRAN ignores blanks, so this declares a single variable `HNTMAXPLAC`. HNTMAX and PLAC drop out of the block;
  PLAC becomes a local array of the main program and is left out of saved games. Should read `...,CLSSES,HNTMAX,`.
  This block is Palter's, not in WOOD0350, so it was typed from the printout.
- **Line 1016, `CODE1('RESID')`.** Should be `CODE1('RESTO')`. Nothing can match `RESID`, so RESTORE could never be
  used.
- **Line 2747, `1010 WDST=1`.** Should be `1010 WDST=I`. With `1`, GETIN's scan for the second word starts again at the
  beginning of the line and never finds it. No two-word command works: TAKE KEYS answers TAKE WHAT?
- **Line 2637, `SUBROUTINE GETIN(WORD1,WORD1X,WORD2,WORD2X)`.** Should end `...,WORD2X,NULLOK)`. The evidence:
  - the comment at 2645 describes NULLOK;
  - line 2651 declares it LOGICAL;
  - lines 2679 and 2680 test it;
  - all twelve calls pass a fifth argument (`.TRUE.` or `.FALSE.`).

  WOOD0350's GETIN has exactly this four-argument header. So this is the "left matching WOOD0350" kind of slip your
  README warns about.

## 3. Please check against the printout

- **Data line 1486, message 66: `DDIGGING WITHOUT A SHOVEL`.** WOOD0350 has the same doubled D, but MSU's port (also
  descended from Palter's) has `DIGGING`. If the printout says DIGGING, the transcription kept WOOD0350's typo; if it
  says DDIGGING, Palter's version kept it too.
- **Data line 1377, message 1: `WILLE CROWTHER`.** Willie? This message is Palter's text (WOOD0350's message 1 is
  different), so it was typed from the printout.
- **Data line 332, room 19: `YOU'RE IN HALL OF MT KING.`** WOOD0350 and MSU have no full stop there.
- **Lines 2464-2465, CVLTUC's `LOWER` table.** The lower-case alphabet is 26 blanks (`1H ,1H ,...`). As transcribed,
  CVLTUC turns every blank in a typed line into an A, and the game understands nothing. The printer could print lower
  case: see ` Initializing...` at line 296. So if the page shows `1Ha,1Hb,...`, the transcription lost the letters. If
  it really shows blanks, the source file had lost them before 1979. The "SCROGGED FOR TSS EDITOR" comments at 2344,
  2395 and 2436 show that editor did mangle character-set lines in this program.
