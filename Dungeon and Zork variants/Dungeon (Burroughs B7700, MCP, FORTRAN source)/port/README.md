# Dungeon V2.0 (Burroughs B7700) - Windows console port

`dungeon.exe` is the DECUS FORTRAN Dungeon (Bob Supnik's translation of MIT's MDL Zork) as a Burroughs B7700 ran it.
It is Chris Wilson's V1.2c code with Tom Fota's international V2.0 text, "This version created 01-DEC-80". Run
`run.bat`. It is the main game only, 500 points. The game's own newspaper, US NEWS & DUNGEON REPORT of 23-APR-80,
says so: "This is a first, trial version of Dungeon on the B7700 based upon the DEC FORTRAN version", without an
endgame, with a simple parser, and "Numerous bugs and spelling errors".

The source reached the INTEREX CSL/1000 library (release 2213, contribution A072, "Dungeons And Dragons Game") with a
note: "This copy has not been edited to work on the HP-1000. Anyone who gets it going please re-submit to the
library". Tom Hutchinson did, as V3.0a: that is `..\..\Dungeon (HP 1000, RTE, FORTRAN source)`. This port runs the
B7700 source itself.

## Build and play

    build.bat          (or sh build.sh; needs gfortran, gcc - MinGW-w64 on Windows - python and sh)
    run.bat

    dungeon [OPTION]...
          --saves=DIR      where SAVE keeps the game (default: saves\ beside dungeon.exe)
          --fixed-clock    the clock counts the lines typed, so that a run repeats exactly
                           (the game draws its random numbers from the clock)
      -h, --help

Unknown options and stray arguments are refused with exit status 2. The port has no fixes, so there is no
`--no-fixes`. Type sentences and press Enter; small letters are taken as capitals. SAVE and RESTORE keep one game,
`saves\ZORK_SAVDATA`. QUIT asks before it leaves. The end of the input ends the game.

Nothing keeps a player out. The game's own gate, PROTCT, lets everybody in on the B7700 too. The game has a debugger:
type GUARDIAN and give the password the source holds, `CHRIS,QUETZAL,27`. A wrong one turns you to dust, as it
always did.

## How it is built

`build.sh`:
1. `src/convert.py` reads the tape image, `..\archive_original\CSL-1000_Rev-2213.zip`, through `src/tftape.py`. It
   takes off the source `&DUNGN` (file a07202, 10,360 cards) and its text `DUNTXT` (a07203, 3,449 lines). It writes
   gfortran source (`.build/dungeon.f`) and the text file under the name the game gives it, `ZORK_DBTXT`. Every edit
   asserts how often it matches; the counts are in `.build/counts.txt`.
2. The source is compiled with the port's run-time, `src/brt.f90` and `src/brtc.c`.
3. The game is run once with no input, as its installer ran it. Its INIT reads the fourteen sections of ZORK/DBTXT
   ("INITIALIZING SECTION # 1" ... "# 14"). It writes the text as the direct-access file ZORK/PTXT and the tables
   as ZORK/PINDX, and later runs load those. Delete them, and the next run builds them again.

The text copies in `..\src_original` are for reading; the build does not use them.

## What the B7700 dialect needed

Much of it is small:
- `$` cards and the FILE declarations become comments; `%` starts a comment.
- Octal constants are written `O+100000` (313 of them, all in DATA statements).
- `COMPL` is NOT; `COMPL(EQUIV(A,B))` is exclusive or; `.IS.` (the same word) is `.EQ.`.
- `CONCAT(0,COMPL(0),35,35,36)` is a mask of 36 ones.
- `FORMAT(" Your score is ",)` ends with a comma where DEC's source had `$`: the score goes on on the same line.
- A literal on the next card needs a comma in front of it, or gfortran joins the two.
- The program's own ITIME is renamed (gfortran has one).

The rest:
- **Words.** A B7700 word is 48 bits and holds six characters, the first on the left. The program keeps characters
  one to a word (A1 input), vocabulary words as two halves of three letters (converted to radix 50 at start-up),
  file titles in arrays. Here every INTEGER, REAL and LOGICAL is 8 bytes. A character constant becomes the number
  whose bytes are its characters, blank-filled, which is what an A1 read gives. So comparisons, DATA lists and input
  agree as they did on the B7700.
- **The word surgery.** Two routines take words apart by bit position: `R50CNV` (is a word characters or a number;
  its n-th character) and ITIME (hour, minute and second out of `TIME(7)`). They get small run-time helpers. `G50`
  compared a character word with `.IS.` after `FLOAT()`, which here would lose the very bits that tell letters apart.
  It compares the words themselves.
- **Files.** `CHANGE(u,TITLE=t,...,MYUSE=IN/OUT)`, `INQUIRE(u,PRESENT=v)`, `CLOSE(u,DISP=KEEP/CRUNCH/DELETE)` and
  `LOCK(u)` are run-time calls:
  - A title under the installer's usercode, "(00661)ZORK/PTXT ON SYMBOL30.", is a file beside dungeon.exe
    (`ZORK_PTXT`); a player's own title, "ZORK/SAVDATA.", is one in the saves folder.
  - `READ(u=r)` and `WRITE(u=r)` are direct access, `DATA=label` is `ERR=`.
  - A file the B7700 closed stayed declared, and the next READ opened it again. The game closes its text file once
    it has built it and goes on reading it, so the run-time opens it again at once.
- **The terminal.** The terminal reads (the command line, yes/no answers, the debugger's) come from the run-time.
  It shows no echo at a console, echoes input when it is not one, folds small letters to capitals (the parser knows
  capitals only), and ends the game at the end of the input. gfortran's A editing ends a field at a comma, so the
  debugger's 10A2 password read copies the characters itself.
- **What the port decided.**
  - Output is what the FORMATs write: a leading blank on every line (a B7700 terminal file had no carriage
    control), and the text lines padded to 78 columns.
  - The prompt `>` is a line of its own.
  - `RESULT=` on a WRITE reported the break key: here nothing breaks.
- **Random numbers.** RND hands `RANDOM` the time of day, `TIME(1)`, at every call. The B7700's generator is not
  known here, so `RANDOM` is a stand-in (a linear congruential step from that seed). Calls in the same sixtieth of
  a second draw the same number, as they did.

## Tests

    python tests/regress.py              options; the reference walk, twice; SAVE and RESTORE; the debugger;
                                         the data base built again; the end of the input
    python tests/regress.py --record     rewrite tests/reference/walk.out from this build
    python tests/fuzz.py [GAMES [TURNS [SEED]]]   random play in a scratch copy (2026-09-22: 50 games of 200
                                         and 200 of 300 sentences, clean)

`tests/reference/walk.out` is this port's own output: there is no B7700 to compare with. The walk opens the mailbox,
enters the house, takes the lamp and the sword and reads the B7700 newspaper. It goes down to the troll, who dies at
the third blow on the fixed clock, and tries a spread of commands. Every test runs in a scratch copy with its own data
base and saves folder.

## Still to do (refine pass)
- Nothing. No B7700 is available for a reference run.
