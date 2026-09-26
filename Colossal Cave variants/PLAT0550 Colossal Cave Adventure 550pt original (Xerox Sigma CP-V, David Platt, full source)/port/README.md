# CP-V Adventure - Windows console port

`adv.exe` is David Platt's CP-V Adventure from the Honeywell Los Angeles
Development Center - the 550-point cave, "almost twice as big as before",
as released on the LADC SST tapes on 7/20/79 with the 12/1/79 patches. It
is the original program: Platt's ANS FORTRAN interpreter running the
database his translator compiled from his cave source. Run `run.bat`.

The cave keeps its 1979 opening hours - weekdays it is closed 9:00-11:30
and 13:30-17:00, when it offers visitors a 30-move demonstration instead
- and a real game ends after 600 moves. `run.bat -u` lifts both, and the
30-minute wait before a saved game can be restored.

## How it is built

`build.sh` (or `build.bat`) does what the LADC jobs did:

1. `src/convert.py` reads the tape image
   (`archive_original\LADC_0012\ladc012_ers.tap.gz`) and writes the
   interpreter (ADVSI + ADV:C) and the translator (MUNGESI + MUNGE:C) as
   gfortran source, and the cave's fourteen D: files as keyed files. Every
   edit asserts how often it matches.
2. The translator, `munge.exe`, is compiled with the port's run-time
   (`src/port/pcpv.f`, `src/port/pcpvc.c`) and run over COMPILE_CAVE's input
   deck, exactly as `!BATCH COMPILE_CAVE` ran it: INCLUDE D:NULLS ... D:ACTION.
   It writes the text file ADVT (`advt.dat`, 2144 records) and the code file
   ADVI (`advi.dat`, 513 records, a 425-word vocabulary).
3. The interpreter, `adv.exe`, is compiled.

The zip conversions of the tape in `src_original` are incomplete (see the
README one folder up); the port does not use them.

## What the source settled

**The dialect.** Xerox Sigma ANS FORTRAN, and `convert.py`'s docstring lists
what gfortran needs done to it: identifiers that begin with `$` (typed by
`IMPLICIT CHARACTER*6 ($)`), `$...$` literals inside a format string,
`REPEAT n, WHILE (c)`, `.EOR.`, `PRINT (u, f)`, `&label` alternate returns
(also on a FUNCTION, and to ASSIGNed labels), INCLUDE'd IMPLICITs after
declarations, names significant to 8 characters (the translator calls the
table `REHASHVALUE` and declares `REHASHVALUES`; it calls `FASTWRITE`, which
is the assembler routine `FASTWRIT`), `Z` in column 1 for debugging lines.
`IF (c) s1; s2` makes both statements conditional - the interpreter's
vocabulary search depends on it, and the Prime version of the translator
in `reference\PLAT0550` writes the same line as two IFs.

**EBCDIC order.** The translator sorts the vocabulary with `.LT.` on the
Sigma, and the interpreter binary-searches it with `.LT.`; NAME prints the
first word of the table with the right value, so the order also decides
which synonym you are shown. The port compares names in EBCDIC order (where
digits come after letters and `?` before `"`), and hashes names with their
EBCDIC codes as MASHSI did.

**Keyed files.** ADVT and ADVI were CP-V keyed files: a text line is a
record keyed by its "line number" (text 4005 is records 4005.000, 4005.001,
...; an object's states and a place's long description begin at .010,
.020, ...), a block of code a record of halfwords (`FORMAT (1024R2)`: R
format writes the rightmost bytes of a word). The port's files keep the same
keys and byte layout, with ASCII for EBCDIC.

**The assembler helpers**, written from their AP source:

- *FASTREAD* gives the translator a line of the cave with its first
  nonblank column and one past its last. A control character as the last
  byte of a record becomes a blank - which is what removes the stray
  carriage return ending "You're at south end of fog-filled room." in
  D:PLACE.
- *CACHESI* keeps code records in memory: a table of 1024 keys over at most
  16 pages of halfwords. It changes nothing a player sees and is kept as it
  was.
- *SVARSI* - system variables. Type 6, the seconds, never stores its answer.
- *PRIMESI* - the opening hours. The day of the week is (365y + y/4 + day)
  mod 7 from M:TIME's two-digit year, which counts a leap year's own 29
  February from 1 January: throughout a leap year it takes every day for the
  next one. The closed spans are
  compared with CLM, both ends inclusive (11:30 is closed, 11:31 open). In
  2028 a Friday is open all day as a Saturday and a Sunday closed in office
  hours as a Monday. ETMF above 6 or more than 999 users also closed the
  cave.
- *GETACCTSI*, *DECSI/ENCSI* (CP-V file encryption), *BREAKSI*, *OUTSWPSI*
  have nothing to do in the port.

**The dice** are `RND`: SEED = |SEED/10000| + |SEED*452| + the clock's
thousandths, in 32-bit arithmetic that overflows. Every draw reads the
clock, so `--time` holds the clock still and makes a game repeat.

**The terminal**, as ANS FORTRAN B08 showed it on CP-V C00 (the program was
taken from the F00 system volume in `bitsavers.org/bits/SDS/sigma/cp-v/f00`
and run on Ken Rector's SIMH kit): a record is printed as it stands,
carriage-control blank included, so every line of the game starts with a
blank; `STOP` prints ` *STOP* ` and its text (` *STOP* 0` when the game ends).
The cave's text uses CP-V's control codes: two bells after the welcome, and
X'20', "line feed only", in a handful of messages (drowning, falling), which
step down the screen without returning (the console is set not to return on
a line feed).

## Bugs in the original, kept

- The weekday of a leap year (above), and SVAR 6.
- D:VERBS has a stray NAK character (Ctrl-U), probably meant as a blank:
  `VERB <NAK> CAVE` defines an untypable verb, with CAVE as its synonym.
- FORMAT 5110 (a glitch message) has a comma in column 73; ANSF never saw
  it, and neither does the port.
- Out-of-range subscripts that land in COMMON: the NAME instruction looks at
  `ARGWORDS(1..4)` of a 2-element array, which are ARG1, ARG2, NOBJ and
  NPLACE; the port is compiled `-O0` so they are still read.

## What the port changes

Nothing about the game. `-u` works by patching two constants in the code
as their records are read: `SET I,600` in the first REPEAT block (the move
limit) and `IFGT I,30` in RESTORE (the wait) become -1. Both instructions are
found by pattern, and the port says so if either is not found exactly once.
At the end of piped input the first read takes FORTRAN's END= branch
(` *STOP* Eof`); a read after that ends the program quietly - the
original's yes/no question would ask again for ever. The translator's two
output files are closed at exit, as CP-V closed them: listing off, it stops
without closing them itself. `--no-fixes` is accepted and changes nothing.

Files: `advt.dat` and `advi.dat` live beside `adv.exe`. SAVE writes
`saves\advfreeze.dat`, the keyed file `*ADVFREEZE`; the key is the account
(the port's is `ADVENTUR`) and the first four letters of the name after
SAVE - `SAVE GAME1` and `SAVE GAME2` are the same save, as they were.

## Options

    adv [-u] [--time HH:MM[:SS[.mmm]]] [--date YYYY-MM-DD]
        [--sense-switch N] [--no-fixes] [-h]

`--sense-switch 1` sets CP-V sense switch 1, which the WIZARD command asks
for (along with a magic word the cave knows).

## Checked so far (first pass, 2026-09-21)

- The translator compiles the whole cave with no error or warning.
- Played: the opening, instructions, the well house, the lamp, XYZZY, NAME
  and VALUE substitution (`I see no dragon here.`,
  `I don't know the word "fnord".`), SCORE, NEWS, HOURS, QUIT.
- The hours: a Monday at 10:00 is closed, prints the hours and offers the
  demonstration; `-u` opens it.
- SAVE, RESTORE within 30 minutes refused, `-u` RESTORE, and the image
  deleted after a restore (`I can't find any saved game`).
- `tests\fuzz.py 400 500`: 400 games of 500 random commands from the
  game's own vocabulary - no glitch report, no crash, no hang.

## Still to do (refine pass)

- **A reference run of the original on CP-V.** Most of the way there:
  `ANSFORT` and `ANSL` copied from F00 volume `00f0` compile, link (LYNX)
  and run under the kit's C00 monitor (a scratch copy of it; answer the
  mount request with the switch key-in `MOUNT A80.A81`; end on-line ANSF
  input with ESC F), and PCL reads the LADC tape (serial PTCH, account SST)
  directly. Left: the kit's `:SYS` granule limit (make room or add an
  account), the AP system file `FLIBMODE` that five helpers name and no
  tape here has (FORTLIB plus R0-R15 equates is the likely stand-in), and
  whether C00's `:LIB`/`:BLIB` hold ANSF's run-time routines (F00's are on
  the same volume). Then ADVJOB, MUNGEJOB, COMPILE_CAVE, and a session
  compared line for line.
- The prompt, if any, that CP-V showed for a FORTRAN read from the terminal
  (the port shows none), and the value M:TIME left in register 10 (taken as
  the milliseconds of the second).
- Play to a win with the wizard's help, and compare the cave with Platt's
  1984 version in `reference\PLAT0550`.
