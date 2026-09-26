# Dungeon V3.0a (HP 1000 RTE) - Windows console port

`dungeon.exe` is Dungeon V3.0a, "Initial version for HP 1000 by Tom
Hutchinson" (Dome Petroleum, Calgary, 18 Nov 1982): the DECUS FORTRAN
Dungeon - Bob Supnik's translation of MIT's MDL Zork - as Hutchinson
adapted it to RTE-IVB and sent it to the INTEREX CSL/1000 library
(release 2240, contribution F042, "DUNGEONS AND DRAGONS"). Run `run.bat`.

It is the main game only: 500 points, and when you have them all a wraith
welcomes you to the chosen of Zork and leaves a sign - "Soon to be
constructed on this site: A complete modern Dungeon endgame". The game's
own US NEWS says as much: "Lack of an endgame".

## How it is built

`build.sh` (or `build.bat`):

1. `src/convert.py` reads the seven FTN4X source files straight off the
   tape image - `archive_original\CSL-1000_Rev-2240.zip`, through
   `src/hptape.py` - and writes them as gfortran source
   (`.build/src/dunga.f` ... `dungl.f`), and the messages file `@DUNGN`.
   Every edit asserts how often it matches, and the counts are printed.
2. They are compiled with the port's run-time, `src/port/pdng.f` and
   `src/port/pdngc.c`.
3. The game is run once: its INIT reads `@DUNGN` and writes the text file
   `@DUNGT` and the index `@DUNGI`, as it did on its first run on the HP.
   Delete them and the game builds them again ("CREATING NEW '@DUNGT'").

The text copies of the tape's files in `..\src_original` (MANIFEST.txt has
their md5s) are for reading; the build does not use them.

## What the source settled

**The dialect.** FTN4X (`convert.py`'s docstring has the list): `!`
comments, quoted strings in DATA for INTEGER arrays, octal `nB` - one
16-bit word, so `177777B` is -1 - `.AND.` of an integer and a constant,
`,(TWH) 1982-11-09` after a FUNCTION statement, `FTN4X,L` and `$` cards.
The vocabulary is DATA for INTEGER*4 arrays, three letters to an element
(`'NOR','TH'`), turned into radix-50 words at the start by R50CV.

**Sixteen bits.** Every INTEGER is INTEGER*2 and every LOGICAL LOGICAL*2:
SAVE writes stretches of COMMON by their length in words (`OFLAG1,1440`
is nine arrays of 160), and the radix-50 arithmetic overflows 16 bits by
design (`-fwrapv`). gfortran's integer literals are 32 bits, so a literal
handed to a statement function (PROB, VALID1-3) is written `80_2`.

**Two characters to a word.** The HP puts the first character in the
high byte; here it is first in memory, which is what gfortran's
Hollerith constants and byte offsets give, so Hutchinson's byte routines
(A2A1, A1A2, UMOVE) carry over as they are. Three statements took words
apart by arithmetic and are edited: the version string (`2H0. + VMAJ*256`,
`VEDIT/256`) and R50CV's test for a text entry (its first byte, the high
one on the HP).

**Segments.** The main program calls the segments B (initialisation), C
(parser), D (debugger), E (save/restore) and F (actions) through DLINK,
Hutchinson's assembler linker: parameters by value in COMMON /SEG/, the
segment runs and `CALL RETRN` comes back. SAVE and RESTORE are reached
from F with DLIN2, which keeps the main program as the return point - so
RETRN goes straight back to it, past F, and the main program finds in
IZRESZ the result of the call before. The port does the same with
setjmp/longjmp (`pdngc.c`).

**The terminal.** REIO writes records; one that ends in `_` (the ` >`
prompt, "ENTER FILE NAME: _") is not followed by CR LF. A message record
that starts with `\` pauses with "HIT RETURN TO CONTINUE". The lexer knows
only capitals and digits (anything else is "Greek to me"), as the
terminals sent them; input is folded to capitals. A one-character read
fills the other half of its word with a blank, as RTE's driver did.

**The libraries.** Routines the program called but the contribution did
not include: UMOVE (a byte move; Larry Schuck, 1979, in contribution F017,
"subroutines used by our other contributions"); CNUMD and KCVT from RTE's
system library; IXOR; and SSEED/URAN from HP's math library (see below).
KCVT's code, read from DL.RUN on the RTE-6/VM disc of the HP 1000
Adventure ports, converts with `$CVT3` for decimal and keeps the last word;
that the conversion gives leading blanks, like CNUMD's six characters, is
assumed here ("Elapsed playing time =  0 hours &   5 minutes."). AND, OR and ITIME are the program's own but are gfortran
intrinsics too, so they are renamed KAND, KOR and ITIMX.

## Options

    dungeon [--time HH:MM[:SS[.mmm]]] [--date YYYY-MM-DD] [-h]

The dice are seeded from the time of day (hours x minutes x seconds), so
`--time` makes a game repeat. `-u` and `--no-fixes` are accepted, as by
the other ports, and change nothing: there are no hours, no move limit and
no fixes.

Files: `@DUNGN`, `@DUNGT` and `@DUNGI` beside `dungeon.exe`; SAVE asks for
a file name and writes `saves\<name>`, RESTORE reads it.

## Debug mode

The implementers' GDT: type `GUARDIAN`, and to "State your name, cat,
and serial number" answer `CHRIS,QUETZAL,27`. `HE` lists the commands:
`AH` puts the player in any room, `TK` gives any object, `AA` alters an
adventurer (entry `1,2` is the player's score), `DA`/`DO`/`DR` display the
tables, `NT`/`NR`/`NC` remove the troll, thief and cyclops, `EX` returns to
the game. An impostor is killed.

## Bugs in the original, kept

- Conditions such as `(PPTR.EQ.0).OR.(PRPVEC(PPTR).NE.0)` read element 0
  of an array: old FORTRAN evaluates both sides. They read the word below
  the array and do nothing with it, here as on the HP.
- The messages' own spelling ("I could't find anything.").

## What the port changes

- **URAN is a stand-in.** HP's uniform random numbers (URAN and SSEED,
  module 24998-1X456 in `$MATH`) are not yet reproduced: the port uses its
  own generator, seeded the same way, so a game repeats from the same time
  but its dice differ from the HP's.
- **Local variables keep their values** between calls (`-fno-automatic`),
  as the DEC original expected. On the HP a segment was read from the disc
  again when another had run in between, and its locals started over. The
  one difference found: YESNO's answer word, when a yes-or-no question is
  answered with an empty line - here the previous answer stands, there the
  question was asked again.
- At the end of piped input the program stops, where the game would wait
  for ever.

## Checked so far (first pass, 2026-09-21)

- `tests\regress.py`: the opening (mailbox, leaflet, VERSION " V3.0A",
  TIME, QUIT); SAVE in one run and RESTORE in the next; GDT and an
  impostor; the win - GDT sets the score to 490, the kitchen's 10 points
  make 500, and 15 moves later the wraith and the sign appear; the data
  base rebuilt from `@DUNGN`.
- `tests\consoleplay.py`: a game at a real (pseudo) console - commands and
  the file name typed on the line of their prompts, no record ending in
  `_`, QUIT and Y end the program with exit code 0.
- `tests\fuzz.py`: 3300 games of 400-500 random commands from the game's
  own vocabulary, with GDT sending the player to random rooms and handing
  out random objects - no crash, no hang, nothing on stderr. A build with
  bounds checking finds only the element-0 reads above.

## Still to do (refine pass)

- Nothing. The Burroughs source this came from (V1.2c code, V2.0 text) is
  ported on its own since 2026-09-22:
  `..\..\Dungeon (Burroughs B7700, MCP, FORTRAN source)`.
- Decided not to do (user, 2026-09-22):
  - a reference run on the HP under SIMH's RTE-6/VM
  - HP's URAN, which stays a stand-in, and whether CNUMD and KCVT pad with
    blanks or zeros
  - segment reload (locals starting over when a segment is reloaded,
    YESNO above)
  - the "CDS DUNGEON" of release 2830 (an FMGR tape)
  - playing to 500 points without the debugger's help
