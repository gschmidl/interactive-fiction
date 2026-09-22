# "ADVENTURE - FORTRAN FROM MSU" (Doug Moore, IBM MVS) - Windows console port

`advent.exe` plays the 350-point game. Run `play.bat`. `advwiz.exe` is the installation program the
game needs (`$ADVDOC` steps 4-6); `build.sh` runs it for you, and `adventst.exe` is the `ADVENT2`
member, "MAIN PROGRAM (TEST VERSION WITH SAVE/RESTORE)".

Every FORTRAN member is compiled from the PDS as it stands. `src/convert.py` applies the edits
gfortran needs and **asserts how many times each one matches**, so if a member ever changes the build
stops instead of quietly producing a different game; `.build/src/` holds what it generated. The port's
own four modules are in `src/port/`: `pchar.f` (character input), `popts.f` (command line), `getdtm.f`
(the clock, replacing the one assembler member) and `pgmdir.c` (where the data files are).

## Verified against the original on MVS 3.8j

MVS 3.8j (TK5) under Hercules ran the original members, compiled with FORTRAN IV G, and the port
matches it line for line:

| | |
|---|---|
| `ADVWIZ` - reads all 12 database sections, reports table use, the wizard's test, the whole maintenance dialogue, writes the initialization file | **identical, 60 lines** |
| `PROBE` - the routines only `ADVENT` calls, and 25 checksums over the state both machines' initialization files hold | **identical, 96 lines** |

`python tests\cmpmvs.py` replays both against the captured MVS output (`tests\mvs_advwiz.txt`,
`tests\mvs_probe.txt`); `tests\mvs\` is the harness that captured them - see "Doing it again" below.
Between them they cover the SIXBIT packing (`CODE1`/`CODE2`/`DCODE1`/`A5TOA1`), `SHIFT`/`AND`/`OR`/`XOR`
on negative values, `RAN`/`PCT` sequences, `VOCAB`, `CVSTB`, `CVLTUC`, `GETIN`/`GET12`, every predicate,
`CARRY`/`DROP`/`MOVE`/`JUGGLE`/`DSTROY`/`PUT`, `SPEAK`/`RSPEAK`/`MSPEAK`/`PSPEAK`, `HOURS`/`HOURSX`/
`NEWHRX` and `MOTD`.

**`ADVENT` itself cannot be compiled on MVS 3.8j.** FORTRAN IV G stops with `IEY031I ROLL SIZE
EXCEEDED` and FORTRAN H with `IEK800I SOURCE PROGRAM IS TOO LARGE`, at every region size from 1 to
8 MB - the 2009-line main program is past both compilers' internal limits. `$ADVFORT` names
**`IGIFORT`**, VS FORTRAN G1, which has larger tables and is not on this system, so `PROBE` is how
everything below the main program is checked instead.

## What had to change, and why

**Characters in integers.** The program keeps one character per word ("A1 format") and five to a word
in its own SIXBIT code. Two things about that do not survive:

- gfortran accepts `A` editing on an integer but **reads it as a numeric field**, so a comma would end
  the field and lose the rest of the line. Every such *read* - `GETIN`, `MAINT`, `MOTD`, `WIZARD` and
  the two database readers - goes through `PGETLN` + `PUNPK` instead. Output is untouched: gfortran's
  `A1` writes the low-order byte, which is where `PUNPK` puts the character (the 370 used the high
  byte; nothing in the program cares which, only that reading and writing agree).
- Hollerith and `'x'` constants going into integers (`1HA`, `DATA BLANK/' '/`, the 64-entry `CHRSET`
  tables, `HOURSX`'s headings) become the byte values, generated from the characters in the member.

**The SIXBIT table has two characters that no longer exist.** Slots 60 and 63 of `CHRSET` hold, in
the copy on the tape, the bytes `A2` and `FF`: under cp037 those are EBCDIC `4A` (a cent sign) and
`DF`. The port keeps the bytes as the copy has them, so the cent sign in the `POOF` message still
prints as one. On MVS the cards are re-encoded to the original EBCDIC before submission, because
Hercules' own ASCII-to-EBCDIC table has no entry for either and differs for `[ ] ^ |` as well - the
reader would have turned them into codes FORTRAN rejects (`IEY013I SYNTAX`).

**Bitwise operations.** `AND`, `OR` and `XOR` were built out of a `LOGICAL*4` `.AND.` over an
`EQUIVALENCE`, which is a 32-bit bitwise and on the 370. gfortran normalises logicals to 0/1 first, so
the three bodies say `IAND`/`IOR`/`IEOR`. `SHIFT` needed only `Z'80000000'` for `Z80000000`.

**`RAN` is a gfortran intrinsic** - a REAL function that writes back to its argument - so without
`EXTERNAL RAN` the program's own `RAN` is never called and `RAN(100)` tries to modify a literal. The
build checks with `nm` that it is the program's own.

**The DD cards.** `IOINIT` opens the files beside the executable: `advtdata.txt` (the database,
FT01F001), `advent.ini` (what `advwiz` writes and `advent` reads, FT02F001) and `saves\advent.sav`
(FT03F001, used only by `adventst.exe`).

**The clock.** `GETDTM` was the one assembler member (`TIME BIN`); the port's returns the same
`YYDDD` and hundredths of a second. The two-digit year is kept, so `START`'s day-of-week sum is as
Y2K-blind as it always was.

## Options

    advent [--date YYDDD] [--time HHMM] [--no-fixes] [-h]
    advwiz [--auto] [--date YYDDD] [--time HHMM] [--no-fixes] [-h]

`--date`/`--time` freeze the clock, which is what seeds `RAN` and what decides prime time, so a run
becomes reproducible; that is how the MVS comparison is made. `--date` has a two-digit year, as the
program's clock had: `START`'s day-of-week sum is Y2K-blind, so for dates after 1999 the weekday is
wrong and the weekend rule may not fire. `--auto` walks `advwiz` past the wizard's guessing game, which
is how `build.sh` makes the initialization file. `--no-fixes` leaves the fixes below out.

## The fixes, and one thing left as it is

1. **`ADVWIZ` clears `MTDTXT` to zero**, but `MOTD` reads it as a chain that ends at a *negative* word
   - `ADVENT`'s own copy of that initialization loop sets `-1` - so a game set up with no message of
   the day never prints its first line. Confirmed on MVS: `PROBEM` there printed `MTDTXT(1..3) = 0 0 0`
   and then spun until the job hit its time limit (`ABEND S322`). The port sets `-1`.
2. **`ADVENT2` reads the initialization file before `IOINIT` has set `DBINIT`**, so it reads unit 0 -
   the one place that member differs from `ADVENT` and is wrong. The port calls `IOINIT` first.
3. **`ADVENT2`'s SUSPEND after a RESTORE** (refine pass, 2026-09-21). The save file is sequential and
   was never rewound: RESTORE read the first game and left the file after it, so a later SUSPEND added
   a second game that the next RESTORE never read - the newer game was lost. The port rewinds before
   writing, so the file holds the one game last suspended. (`adventst.exe --no-fixes` cannot show
   the old behaviour: without fix 2 it does not start.)

Whatever the options (refine pass): **`CARRY`** walks the list of things at a location until it finds
the object. Handed a location the object is not at, it reached the end of the list and read `LINK(0)`,
outside the array; on MVS the walk went on through whatever lay before it in `/PLACOM/` (that is how
the first probe hung). It now stops at the end of the list, and at a location outside `ATLOC`.

Left alone: `ADVWIZ` sets `DLOC(3)=19,27,3,44,64`, where `ADVENT`'s own copy of the same loop has
`33`. The game takes its dwarf positions from the initialization file, so dwarf 3 starts at location 3
- outside the cave, in the valley - and not in the Hall of Mists. That is what the original does.

## Setup the build uses

`build.sh` answers `advwiz` with: no prime time (`99` is out of range, which is how `NEWHRX` is told
"never"), no holiday, the lengths left alone, no message of the day, `BLKLIN=F` as `IOINIT` sets it.
MSU's own default was `WKDAY=130944` - the cave closed 07:00 to 17:00 on weekdays, when only a wizard
could play and everyone else got a 30-move demonstration game. `advwiz` interactively puts that back:
the magic word is `DWARF`, and the challenge is the ten octal digits exclusive-ored into the magic
word, typed back as five characters of the game's own character set (`tests\cmpmvs.py` computes one).

## Checked so far (first pass, 2026-09-21)

- `tests\cmpmvs.py`: both MVS references replay byte-identically.
- A 33-move session from the well house into the Hall of Mists: grate, lamp, dwarves, the snake,
  inventory, score and the class message all behave.
- `build.sh` verifies with `nm` that `AND`/`OR`/`XOR`/`SHIFT`/`RAN`/`GETDTM` are the program's own.
- Builds with gcc/gfortran 15 (`-Wall`); the remaining warnings are the original's unused variables
  and unreachable statements (FORTRAN H reports the same two unreachable ones).

- Refine pass (2026-09-21): 120 random sessions of 300 commands (the game's own words, the clock frozen
  at random hours, every other one with `--no-fixes`) end cleanly; SUSPEND, RESTORE, SUSPEND, RESTORE
  with `adventst.exe` gives back the second game.

## Still to do (refine pass)

- Deferred (user, 2026-09-21: bugs and fuzzing only): a play-through to a win; the prime-time machinery
  (`HOURS`, holidays, the `LATNCY` wait) with MSU's office hours. No comparison with other ports: only
  identical setups are compared.

## Doing it again on MVS

`tests\mvs\` drives TK5 (MVS 3.8j TurnKey 5, `MVS-TurnKey5.zip`, extracted
to a scratch copy; `start.ps1` starts it in daemon mode, and jobs go in through the socket card reader
on port 3505):

    python mkjobs.py                     # j1src.jcl, j1data.jcl - source and database
    python mkjobs2.py PROBE PROBEM       # j2cl.jcl - compile and link
    python mkjobs.py add PROBE ../probe.f
    python mkjobs.py wiz wizin.txt       # j3wiz.jcl
    python mkjobs.py adv probein.txt PROBE MOORRUN
    python sub.py j1src.jcl MOORSRC 240  # submit, wait, collect the printer

`sub.py` leaves the printer output in `last.prt`; `prtnorm.py` pulls a program's own lines out of it.
Three things to know: a JCL statement must end by column 71 or the card is silently truncated;
`IEBGENER` needs a `SYSIN DD DUMMY` even with nothing to say; and the printer file uses CRLF with a
lone CR where the carriage control said not to advance, so Python's universal newlines invent blank
lines - read it as bytes.
