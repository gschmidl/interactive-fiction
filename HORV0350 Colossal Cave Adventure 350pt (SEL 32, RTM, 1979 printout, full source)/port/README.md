# Adventure 350 for the SEL 32 / RTM (Horvath & Norwood) - Windows console port

`advent.exe` plays the 350-point game. Run `play.bat`. The program sets itself up on a first run - see
"How the game builds itself" below - which `build.sh` does for you.

`src/convert.py` splits Arthur O'Dwyer's transcription of the 1979-03-21 printout into program units,
drops the RTM job control and the curatorial markings, and applies the edits gfortran needs; each edit
**asserts how many times it matches**, and the generated tree is in `.build/src/`. The SEL 32's own
run-time is replaced by four modules in `src/port/`: `pchar.f` (character input), `popts.f` (command
line), `psel.f` (the file system, the clock, the message-text array) and `pselc.c` (`ADDR`, `SIZE` and
the block transfer that saves the game).

## Ten things wrong with the transcription

Nine of them stop the game working; the tenth is a doubled letter in a message, and may be on the
printout itself (see below). `../TRANSCRIPTION_ERRORS.md` writes them up for Arthur O'Dwyer, who offers
a bounty for them, with the line numbers and the evidence. Each is marked `C  PORT: TRANSCRIPTION:` in
the generated source, or listed in `convert.py`.

| where | as transcribed | read as |
|---|---|---|
| `ADVENTUR` COMMON | `...CLSSES,HNTMAX` / `PLAC,FIXD,...` (no comma) | the comma - without it the two names run together and `PLAC` is a local array full of stack rubbish |
| `ADVENTUR` | `INVENT=VOCAB(0+'INVEN'),2)` | `VOCAB(CODE1('INVEN'),2)` |
| `ADVENTUR` | `.GAVEUP.=TRUE` | `GAVEUP=.TRUE.` |
| `ADVENTUR` | `CODE1('RESID')` | `CODE1('RESTO')` - nothing could ever match `RESID`, so RESTORE never worked |
| `GETIN` | `1010 WDST=1` | `WDST=I` - with 1 the scan restarts at the beginning of the line and no second word is ever found, so "TAKE KEYS" answers "TAKE WHAT?" |
| `GETIN` | `SUBROUTINE GETIN(WORD1,WORD1X,WORD2,WORD2X)` | `...,NULLOK)` - the comment above it describes `NULLOK`, the body tests it, and all twelve calls pass it |
| `SPEAK` | `IF LINES(K).GE.0)GOTO 10` | `IF(LINES(K).GE.0)GOTO 10` |
| `MOTD` | `CVLTUC(TEXT,K)` | `CALL CVLTUC(TEXT,K)` |
| `RAN` | no label on `R = R * 16807` | `1     R = R * 16807`, the target of `IF (R.NE.0) GOTO 1` |
| database, message 66 | `DDIGGING WITHOUT A SHOVEL` | `DIGGING`, as in MSU's database - but WOOD0350 has the doubled D too, so the printout may have it; the port prints `DIGGING` |

`convert.py` also checks, on every build, that no unit branches to a label it does not define and that
no continued declaration list is missing its separator - the two checks that caught the `RAN` label and
the `COMMON` comma. They are cheap and this is a transcription of paper.

**One more thing the paper cannot have held.** `CVLTUC`'s lower-case alphabet is 26 blanks in the
printout, which would turn every space in the player's line into an "A" - the game would be unusable.
It is restored to `a`..`z` here. `--no-fixes` puts the blanks back, and the game then answers
"I DON'T UNDERSTAND THAT!" to everything, which is the demonstration. (The printer could print lower
case: ` Initializing...` in the main program proves it. So either the source file on the SEL had been
flattened - the `CHRSET` comment says that line was "SCROGGED FOR TSS EDITOR" - or this page of the
listing lost it.)

## Checked against the MSU port's database

`python tests\cmpdata.py` compares this database with the one in
`..\..\(MOOR0350) …` - Woods' 350-point data in the same format, from the same Palter ancestor, and
checked there against the original running on MVS 3.8j. 32 differences, every one accounted for:

- MSU's own additions: the shack and outhouse (rooms 141/142, words SHACK and SHIT, message 202, the
  cond bit and the travel entries for them), "PLEASE DO NOT TRY TO EAT THE WALLS!", a reworded set of
  maintenance messages;
- Palter's additions, which O'Dwyer's README predicts exactly: `G` and `T` for GET/TAKE, `SLAY` for
  KILL, `Q` for QUIT here, `?` for HELP there;
- brackets: `[SIC]` and `[WITT CONSTRUCTION COMPANY]` here, `(SIC)` and `-WITT…-` there, because
  MSU's EBCDIC had no brackets;
- **two places where this database has more than MSU's**, both of which O'Dwyer predicts as the mark
  of an 80-column truncation somewhere in MSU's history: a ninth exit from room 108, and room 87 in the
  list of places where the maze hint may be offered. The SEL read 128-column records (`FORMAT(16I8)`),
  so nothing was lost here;
- and `DDIGGING`, listed above (WOOD0350 has it too).

## What the SEL 32 did differently

- **The message text did not fit in memory.** `LINES` is not an array in this port but a *function*
  over a random-access file (`ADV.LINE`), paged 192 words at a time by the assembler module in the
  last job of the printout. The port keeps the file - `adv.line`, written by `SAVLINES` - and holds the
  array in memory between times.
- **The game saves itself by address.** The main program builds a table of the eleven COMMON block
  ranges with the site-supplied `ADDR` and `SIZE`, and `LDCOMN`/`SVCOMN` hand it to the system to be
  written or read. That survives: `ADDR` stores a real 64-bit pointer in the first two of the four
  words the source reserves for each "pointer", and `PXFER` (C) does the transfer. `newgame2.sav` is
  the fresh game the wizard saved; `saves\NAME.sav` is a suspended one.
- **SEL FORTRAN extensions**, all in the modules the port replaces except the last two: `M:OPEN` and
  friends, `INLINE`/`ENDI` assembler (the clock read RTM's date string out of absolute memory at
  `3ZAB0`), `INTEGER*1`/`INTEGER*8` with `EQUIVALENCE`, `nZhhhhhhhh` hex and `nRc` character
  constants, `MM = DD = YY = I = 0`, `;` between statements, `$2` alternate returns - and, in the game
  itself, `.AND.` as a bitwise operator on integers and a `SHIFT` intrinsic (the rest of the program
  already says `IAND`/`ISHFT`, so `BITSET` now does too).
- **Characters in words**: one per word for input and output, five per word in the game's own SIXBIT
  code, with the character in the *high* byte on the SEL. The port puts it in the low byte, which is
  the byte gfortran's `A1` writes; `PGETLN`/`PUNPK` replace the `A1` *reads*, which gfortran treats as
  numeric fields. `"` was the escape character inside a SEL literal (`CAN"'T`, `CODE1('"".   ')`).
- The 64-character set has a backslash and a **bell** (07) in its last two slots, given as hex in the
  source because the TSS editor could not hold them; the comment there says 5F, underbar, but the data
  says 07, and 07 is what the port keeps. No game text uses either.

## Options

    advent [--auto] [--day N] [--time HHMM] [--no-fixes] [-h]

`--day`/`--time` freeze the clock (day 0 is Saturday 1 July 1978, as `DATIME` counts), which makes a
run reproducible: the clock is what seeds `RAN` and what decides prime time. `--auto` walks past the
wizard's test, which is how `build.sh` gets the game to set itself up. `--no-fixes` leaves out the two
fixes that are switchable - `CVLTUC`'s alphabet and `GETIN`'s `NULLOK`; the eight that stop it
compiling or running at all are always in.

## How the game builds itself

With no `newgame2.sav`, the program reads `adv.data`, builds the text and the tables, and calls
`MAINT`, which only a wizard may enter and which ends by saving the fresh game. The magic word is
`DWARF` and the challenge is ten octal digits exclusive-ored into the magic word, typed back as five
characters of the game's own character set. `MAINT` also forces `BLKLIN` true, which is why this port
prints a blank line before every message.

## Checked so far (first pass, 2026-09-21)

- Sets itself up: 9735 of 9800 words of message text, 742 of 750 travel options, 300 of 300
  vocabulary words, 140 locations, 53 objects, 201 random messages, 10 classes, 9 hints, 32 magic
  messages - and none of the database's own `BUG` checks fire.
- A 31-move session out of the well house, through the grate, down to the Hall of Mists and back out
  through `XYZZY`: two-word commands, the grate, the lamp, the bird, the nugget, dwarves, `SCORE`, the
  class message.
- `SUSPEND MINE` writes `saves\MINE.sav`; `RESTORE MINE` on a later run brings the keys and the
  lantern back.
- `tests\cmpdata.py`: 32 differences against the MSU database, all accounted for above.

## Refine pass (2026-09-21)

- 120 random sessions of 300 commands (the game's own words, the clock frozen at random days and hours,
  every other one with `--no-fixes`) end cleanly: nothing to fix.

## Still to do (refine pass)

- Deferred (user, 2026-09-21: bugs and fuzzing only): a play-through to a win; the prime-time machinery
  (`HOURS`, `NEWHRS`, the holiday, the `LATNCY` wait before a restored game); `MOTD(.TRUE.)`.
- Written up for O'Dwyer (user, 2026-09-22): `../TRANSCRIPTION_ERRORS.md` - nine claimed errors, and four
  questions for the printout (`DDIGGING`; `WILLE CROWTHER` in message 1; `YOU'RE IN HALL OF MT KING.`, where
  WOOD0350 and MSU have no full stop; `CVLTUC`'s blank lower-case alphabet). Mailed to O'Dwyer by the user on
  2026-09-22.
