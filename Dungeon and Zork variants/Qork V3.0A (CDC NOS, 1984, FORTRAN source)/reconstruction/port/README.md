# Qork V3.0A - Windows console port

`qork.exe` is Qork V3.0A, S. O. Lidie's DECUS Dungeon for a Control Data
Cyber under NOS ("WELCOME TO QORK. THIS VERSION CREATED 84/05/30."). The
text's own revision history runs V1.0A, V1.1A (the first public version),
V1.2A and V1.2B (Chris Wilson's bug fixes), V2.0A (Lidie's NOS/BE
implementation) and V3.0A (NOS). The INFO text traces it back through
Tom Kelly's Burroughs 6800 FORTRAN to Bob Supnik's DEC FORTRAN.

It is the original FTN5 program, `qork.src`, compiled with gfortran, and
its own database, `qork.txt`. It has been checked line for line against
the same source compiled with FTN5 on NOS 2.8.7 (DtCyber): seven sessions
come out identical. Run `run.bat`.

## Playing

English commands; only the first six letters of a word count. Lower case
is fine. INFO and HELP explain the rest; BRIEF, SUPERBRIEF and UNBRIEF set
how rooms are described.

- **SAVE and RESTORE** use `saves\QORK.SAV`, one saved game, as the
  player's TAPE3 was on the Cyber. RESTORE before any SAVE starts the game
  over.
- **The dice.** FTN5's RANF starts from the same seed in every run, so on
  the Cyber every game rolled the same dice, and so does the port.
  `--seed N` gives other dice.
- **The end of input** (Ctrl-Z at the console): "I cannot hear you!", and
  the game reads on. At the end of a pipe or file, a second end in a row
  ends the program (exit 0).
- **The ending** is Lidie's own, not the DECUS Dungeon's (which, like the
  Bank, Qork does not have). `tests\win.py` describes it: spoilers.

**GUARDIAN**, the implementers' debugging tool, is reached by typing
GUARDIAN and then the password `EXPLURIBUSONION TKMG`; a wrong one is
fatal. HE lists its commands and EX leaves it.
- **Limits** ("LIMITS:", "IDX,ARY:") are numbers in free form (`1 3`,
  `1,3`), which may run on to the next line; `1/` leaves the second as it
  was.
- **New values** ("NEW=") are read six columns wide with blanks as zeros,
  so type them right-aligned: `     5`. A bare `7` is 700000, on the
  Cyber too.

## How it is built

`build.sh` (or `build.bat`):

1. **`src/convert.py`** edits `qork.src` into gfortran source,
   `.build\src\qork.f`. Every edit asserts how often it matches, and its
   docstring lists what FTN5 needs:
   - `"TEXT"`, `R"X"` and `O"777"` constants become the Cyber's 60-bit
     words as INTEGER*8;
   - FORLIB's word functions and RIO's calls get the port's names;
   - every READ and WRITE that moves characters becomes a call that
     converts display code by NOS's 6/12 ASCII rules;
   - GUARD's reads read a line and apply their own FORMAT to it;
   - GUARD's octal (`Ow`) and list-directed output is written as FTN5
     wrote it;
   - each random draw inside a compound condition is made before the
     condition, as FTN5 made it whatever the rest said (below);
   - CLOCKD keeps its value from call to call, as FTN5 kept it (below).

   The COMPASS in the deck (RIO, the random database file, and CONCAT) is
   left out.
2. **The CDC layer:**
   - `src/port/pqork.f`: display code and 6/12 terminal I/O; SHIFT, COMPL,
     RANF and TIME; sense switch 1; the options and the files;
   - `src/port/pqorkc.c`: RIO, which keeps the text as records of lines
     at the PRU addresses the Cyber gave them.
3. **`qork.exe --build`** does what the site did with sense switch 1
   off: PRS reads `qork.txt` into the random text file `qork.dat` and
   saves the initial state as `qork.ini`. A game restores `qork.ini`.

gfortran warns that some named COMMON blocks (OINDEX and others) differ
in size between routines. The source declares them so; the Cyber's loader
used the longest, and so does the linker.

## What the Cyber settled

Measured on NOS 2.8.7 with FTN5. The probe jobs and their print files are
in `tests\cyber`.

- **RIO** (`mkprobe.py`, `probe.txt`, RIO taken from qork.src
  untouched):
  - the first WRR ends an empty record at PRU 1;
  - a record takes words/64 + 1 PRUs, a line (n+11)/10 words;
  - READS fills the buffer one character a word and pads with blanks;
  - the end of a record comes back as status 0703.
- **RANF** (`ranf.job`, `ranf.txt`): seed × 44485709377909 mod 2^48,
  from 48131768981101 in every run.
- **Every operand of `.AND.` and `.OR.` is evaluated.** Sessions 2 and 3
  parted from the Cyber at the first roll of the dice. A trace of every
  RND call on both machines showed why:
  - the Cyber draws about 160 numbers a turn;
  - the thief's `IF((OADV(I).NE.-THIEF).OR.PROB(70).OR.(OTVAL(I).GT.0))`
    alone draws once for every object;
  - gfortran short-circuited those draws away.

  convert.py makes each of the eleven draws before its condition, keeping
  any label on it. The traces then agree for all 4,831 calls the Cyber's
  printout held, built at -O0, -O1 and -O2 alike.
- **A function's value lives in a word of its own** (`fres.job`,
  `fres.txt`). A call that does not set it hands back the last value it
  had, and 0 before the first. CLOCKD sets its value only when a clock
  event fires. So WAIT, which runs the clock three times unless CLOCKD
  says an event fired, waits three turns until the game's first event
  and one turn from then on. gfortran would return whatever was in a
  register.
- **`Ow` output** is the rightmost w of the word's 20 octal digits,
  leading zeros and all: `062000` where gfortran writes ` 62000`.
- **List-directed output** writes each value in as few columns as it
  takes, one blank apart (GUARD's DS).
- **List-directed input** looks for missing values on the next line,
  skips empty lines, and stops at a slash. A short line read with
  `(BZ,I6)` is padded with blanks, which BZ makes zeros.
- **The print file** adds a blank to a line of odd length (NOS keeps
  characters in pairs). `cmpcyber.py` takes it off, and maps `^X` to
  lower case.

## Bugs in the original, kept

- **One move** is reported as "[TOTAL OF  550  POINTS], IN 1 MOVES."
  (FORMAT 130 has an extra blank).
- **Three out-of-range subscripts**, all harmless. Each reads a word that
  is the same on both machines, and nothing uses the result:
  - RSPSB2 trims a blank text line down through `B1(0)` and `B1(-1)`;
  - TAKE and TAKEIT evaluate `OADV(X)` with X = 0 (that is OROOM(160))
    beside `X.NE.0`;
  - SPARSE reads `PRPVEC(0)` (O2 of /PV/) beside `PPTR.EQ.0`.
- **SVERBS declares `/PRSVEC/` as PRSA, PRSI, PRSO**, the other way round
  from every other routine, as the DECUS source does.
- **In GUARDIAN,** a bare number for NEW= is multiplied by BZ, and a list
  of limits waits for its second number.

## What the port changes

- **The terminal** is a 6/12 ASCII terminal. Typed lower case arrives as
  `^` and the letter, which the program strips, and output pairs are
  shown as lower case: "I cannot hear you!" is FORMAT 401's
  `^C^A^N^N^O^T ^H^E^A^R ^Y^O^U!`. Input is masked to seven bits;
  NULs and carriage returns are dropped.
- **The end of input.** On the Cyber a batch job's end of input made the
  program say "I CANNOT HEAR YOU!" until the time limit. Here a second
  end in a row ends it, while a console can type on after a Ctrl-Z.
- **In GUARDIAN,** a limits line that is not numbers ends the read as the
  end of input would; FTN5 stopped the program with a fatal error.
- **Files.** `qork.dat`, `qork.ini` and `saves\QORK.SAV` sit beside the
  program; `qork.txt` is needed only by `--build`.
- **No fixes.** `--no-fixes` is accepted and changes nothing.

## Options

    qork [--seed N] [--build] [--no-fixes] [-h]

## Checked (first pass, 2026-09-21)

- **`tests\cmpcyber.py`**: seven sessions identical with the Cyber's
  print files. `tests\cyber\mkdeck.py` makes the card deck: the control
  statements, `qork.src` as it is, `qork.txt` and the commands.
  `job287.js` submits it from a DtCyber NOS 2.8.7 tree (`up287.js` and
  `down287.js` start and stop it). The sessions:
  - 1: the opening;
  - 2: a troll fight to the death;
  - 3: INFO, HELP, SAVE and RESTORE, and verbs;
  - 4: GUARDIAN's displays, ALTER values and TK;
  - 5: a tour of the house, attic, cellar and troll (a death and a
    patch), TAKE ALL, DROP ALL, SAVE, RESTORE and the BRIEF modes;
  - fuzz: 302 random commands;
  - win: the ending.
- **`tests\regress.py`**: 26 checks, then cmpcyber, win and fuzz. It
  covers:
  - the options and refused ones;
  - lower case;
  - QUIT and YESNO;
  - the end of input three ways;
  - the dice, and `--seed`;
  - SAVE and RESTORE across games;
  - `--build` making identical `qork.dat` and `qork.ini`;
  - missing files;
  - the GUARDIAN password.
- **`tests\win.py`**: 600 of 600, "GOODBYE, IMPLEMENTER!", and the
  program ends there. GUARDIAN skips the middle of the game; the ending
  is played, and the Cyber played it too.
- **`tests\fuzz.py`**: 250 games of up to 1,000 random commands from the
  game's own vocabulary, in both cases, with other dice. None ended with
  a run-time error or a hang. A bounds-checked build played 600 more:
  only the three subscripts above.
- **`tests\consoleplay.py`**: a real console (ConPTY). It checks:
  - the room is drawn before the first read;
  - lower case is played;
  - Ctrl-Z then more play;
  - QUIT and y end the program.

## Still to do (refine pass)

- **More of the main game on the Cyber.** The comparisons cover the
  opening, the house and cellar, the troll, GUARDIAN and the ending. A
  full walkthrough (the dam, the thief, Hades, the maze, the Alice rooms)
  played on both machines would cover the rest.
- **An interactive NOS terminal.** The 6/12 input and output are
  modelled, not measured. A session through DtCyber's IAF would settle
  lower case and what a terminal's end of input did.
- **Lidie's later V5.0a** (eXo, 2022) could be compared with this text for
  the ~120 lines that changed.
