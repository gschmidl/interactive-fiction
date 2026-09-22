# Adventure 350 on a Prime 50-Series (PRIMOS FORTRAN) - Windows console port

`advent.exe` plays the game exactly as it was installed on the Prime the SBD003 tape came from. Run
`play.bat`.

`src/convert.py` splits `ADVENTURE.FTN` (the game, Gary Palter's portable Adventure) and `ADVSUB.FTN`
(the site-supplied half his conversion guide asks for) into program units and applies the edits
gfortran needs, each asserting how many times it matches; `.build/src/` holds what it generated. The
port's own support is `src/port/pprime.f` and `pprimec.c`.

## Verified against the original running on PRIMOS 23.4

The Prime emulator (p50em) still runs the pack this tape was restored onto, and **`ADVENTURE.UFD` with
its build scripts, its binaries and its installed run file is on it**, so the original itself is the
reference. `FTN` and `SEG` are on the pack too.

    python tests\cmpprime.py        1        IDENTICAL (107 lines)
                                   2        IDENTICAL (123 lines)

`tests\prime1.txt` and `prime2.txt` are what the original printed over a telnet line
(`tests\prime\primeplay.py`); `cmpprime.py` replays the same commands here and diffs them after
taking out PRIMOS's echo of each command and its `**** STOP`. Between them the two sessions cover the
opening, the well house, the grate, the valley and the slit, `INVENTORY`, `SCORE`, `HOURS`, the long
instructions text, eating, drinking, filling the bottle, `BRIEF`, `XYZZY`, `QUIT` answered both ways -
and the mixed-case output, which is this port's own character.

Both sessions stay out of the cave, because the generator is seeded from the clock on the Prime and
nothing the game prints on the surface depends on it. Inside the cave the dwarves do.

## What the original does that no other port of this family does

- **It prints in mixed case.** `ULCASE` lower-cases everything except the first letter of a sentence,
  and it is not quite right: a sentence that begins on a continuation line stays lower case, which is
  why the first room reads "... down a gully. **a**round you is a forest."
- **`PSPEAK` still has its debugging in it.** `CALL TOOCT(MSG)`, `T1OU('  ')`, `TOOCT(SKIP)` print
  `000000 000000` in front of every object, and `TODEC` prints `-    1` while it walks the message
  chain. `TOOCT` and `TODEC` take an `INTEGER*2` while the program is compiled `-INTL`, so on a
  big-endian machine they print the *high* halfword of a 32-bit value - which for a small number is
  zero. The port reproduces all of it, because that is what the installed game does.
- **`PSPEAK`'s loop was mid-rewrite too**, so an object at a location is described by its *inventory*
  name: "Set of keys" where every other port says "There are some keys on the ground here."
- **The wizard's password is the day of the week.** `WIZARD` prints `WAZU` and expects the magic word
  with its second and fourth letters replaced by the two-letter code for today (`SA`, `SU`, `MO`, ...),
  so on a Saturday `DWARF` becomes `DSAAF`.
- `BADVENTURE.FTN` beside it is the same program with `TRACE` turned on and `SPEAK`'s output commented
  out - the debugging build. `ADVENTURE.FTN.001` is older and calls `LEGAL`, which deletes the run
  file if it is not being run from the `GAMES` directory. Neither is what was installed.

## The Prime's own state, converted

The game does not build its tables at startup: it loads them, with the eleven COMMON block ranges its
main program describes with `ADDR` and `SIZE`, from a file the wizard once saved. That file - `ADVCOM`
- is on the tape, so `tests\advcom.py` converts it into `advcom.dat` and the port starts from the
state the Prime was actually in, text, vocabulary, hours, magic word and all.

Two things differ from the port's memory, and the script handles both field by field:

- the Prime is big-endian;
- compiled `-INTL` its `INTEGER` is four bytes but its `LOGICAL` is still **two**, packed with no
  padding. That is why the file is eighteen words shorter than the port's eleven ranges: one halfword
  for `BLKLIN`, twenty for `HINTED`, six for `DSEEN`, nine for nine `LOGICAL` scalars - 36 halfwords.
  `advcom.py` derives the field list from the source's own declarations and checks that the arithmetic
  comes out at exactly the file's length; it does, to the byte.

`/IOSCOM/` is one of the saved blocks, so the file also carries the Prime's unit numbers (1 for the
terminal, 5 for the database); the port puts its own back after loading and leaves `BLKLIN` alone.

## Measured on the emulator, not guessed

- **`RAND$A`** sets `R = MOD(16807*R, 2**31-1)` and returns `R/(2**31-1)` - Lehmer's minimal standard.
  Seed 1 gives 16807, 282475249, 1622650073, ... which is what the machine printed.
- **`MCHR$A(A,I,B,J)`** puts the J'th character of B into the I'th of A.
- **`T1OU`** types the low-order byte of its halfword; **`TOOCT`** six octal digits; **`TODEC`** six
  columns, and a negative number as a minus sign and then five (`-    1`, `-32768`).
- **`TIME$A`** is the time in hours and **`DOFY$A`** is `(year-1900).(day of year)`, which the
  original turns into days since 1 January 1977 - the arithmetic the port keeps.
- Probe programs are in `tests\prime\`; they were put on the machine with `ED` and read back to check
  (`edput.py`).

## Options

    advent [--seed N] [--day N] [--time HHMM] [--auto] [--no-fixes] [-h]

`--seed` starts the generator at a known value instead of from the clock, which is what makes a run
reproducible; `--day`/`--time` hold the clock still (day 0 is 1 January 1977, a Saturday, as `DATIME`
counts). `--auto` is for the wizard's test. There are no behavioural fixes in this port, so
`--no-fixes` currently changes nothing.

## Checked so far (first pass, 2026-09-21)

- `tests\cmpprime.py`: both recorded PRIMOS sessions replay byte-identically.
- `SUSPEND MINE` writes `saves\MINE.sav` (13553 words, the same eleven ranges); `RESTORE MINE` reads
  it back and then refuses to continue until `LATNCY` minutes have passed, which is the installed
  game's own rule (it is set to 1).
- Builds clean with gcc/gfortran 15 (`-Wall -fdollar-ok`; the `$` is in the Prime's own names).

## Refine pass (2026-09-21)

- 120 random sessions of 300 commands (the game's own words, `--seed`, the clock frozen at random days
  and hours, every other one with `--no-fixes`) end cleanly: nothing to fix.

## Still to do (refine pass)

- Deferred (user, 2026-09-21: bugs and fuzzing only): a session inside the cave compared against the
  Prime (the generator matches; the original's clock seed would have to be read out of a suspended
  game's file and fed to `--seed`); a play-through to a win; the prime-time machinery (`WKDAY=130560`,
  09:00-17:00 on weekdays); `MAINT`, the wizard's dialogue and `MOTD(.TRUE.)`.
- `IT.FTN` and `ADVSUB2.FTN` on the tape are further variants; `SCOTT>ADVENTURE.LIST` is a compiler
  listing of a third. None is ported.
