# Adventure 366 on a CDC Cyber under NOS 1.3 - Windows console port

`advent.exe` plays the game as Bill Hein and Shelley Hobson's version ran at
ACCA: 366 points, the gazebo in the woods and the palantir you peer into.
Run `play.bat`.

`src/convert.py` edits `adventure.src` into one file gfortran will take - every
edit is a whole line and asserts how often it matches - and copies
`adventure.txt`, the text database, out beside the game exactly as it is on the
tape. The port's own CDC layer is `src/port/pcdc.f`: display code, the R and A
edit descriptors, SHIFT and MASK, the word addressable files, and FORLIB's
random number generator.

## Verified against the original running under NOS 1.3

DtCyber runs a Cyber 173 with the Florida State NOS 1.3 system, and **FTN 4.7
compiles this source as it stands** - 0.6 CP seconds for the whole deck - so the
original itself is the reference:

    python tests\cmpcyber.py        rnd      IDENTICAL (16 draws)
                                    1        IDENTICAL (56 lines)
                                    2        IDENTICAL (161 lines)
                                    3        IDENTICAL (122 lines)

`tests\cyber\mkdeck.py` builds a card deck of four records - the control
statements, the source, the database, and the commands the game reads from
INPUT - and `tests\cyber\job.js` submits it through DtCyber's card reader.
`tests\cyber1.txt` and its friends are the print files that came back;
`cmpcyber.py` cuts the game's own output out of them and diffs it against the
port playing the same commands. Between them the three sessions cover the
surface, the well house, the wizard's password, thirty turns in the cave with
the dwarves throwing knives, the gazebo, the palantir, the hint machinery,
`INVENTORY`, `SCORE`, `HOURS`, unknown words, and `QUIT` answered both ways.

**The one line of difference:** the deck is built with `mkdeck.py --seed`,
which changes `CALL RANSET(SECOND(1.))` to `CALL RANSET(0.5)`. The game seeds
itself from the CPU clock, so it is not reproducible on the Cyber either - two
identical jobs of the untouched source give different dwarves, which is in
`tests\cyber\` as well. With the seed fixed, the port started with
`--seed 140737488355329` (what `RANSET(0.5)` leaves behind) matches to the byte.

## The 366

The folder used to say 350. The game computes its own maximum as it scores, and
what it printed on the Cyber is

     YOU SCORED  32 OUT OF A POSSIBLE 366, USING    9 TURNS.

Sixteen treasures, not Woods' fifteen: object 65 is the PALANTIR(ORB), and
since it is above the chest it is worth 16, which takes 350 to 366.

## Measured on the machine, not guessed

The probes are `tests\cyber\probe.job`, `probe2.job` and `probe3.job`, with what
they printed beside them.

- **`RANF`** is `seed = seed * 44485709377909 mod 2**48` (the multiplier is
  octal 1207264271730565), and the value is `seed / 2**48`. The multiplier came
  out of five independent (seed, next) pairs, all agreeing.
- **The seed a job starts with is a constant**, 48131768981101 (octal
  1274321477413155) - the same in two different runs.
- **`RANSET(x)`** takes the 48 bit coefficient of `|x|` normalised as a
  fraction and forces bit 0 on: 1.0, 2.0, 4.0, 0.25 and 0.5 all give
  04000000000000001, 1.5 and 3.0 give 06000000000000001, 0.6 and 0.075 give
  04631463146314633. `RANSET(0.)` leaves the seed alone. Read back with
  `RANGET`, which is how the seed is knowable at all.
- **`RND`** truncates `RANF*RANGE`; `tests\prnd.f` calls the game's own RND with
  a measured seed and reproduces all sixteen numbers the Cyber printed.
- **FTN never short circuits.** `IF(WD1.EQ.4RWEST.AND.PCT(10))` calls `PCT` -
  and so draws from the generator - even when the first test has already
  decided the answer; probe3 shows it for all six shapes in this program.
  gfortran short circuits at -O2 and not at -O0, so `convert.py` hoists each of
  those draws out into a statement of its own. Without it the dwarves turn up at
  different times, and the port's behaviour would depend on the optimisation
  level.
- **Display code** is the 64 character set, `:` = 00 and `;` = 77 octal: `4RTEST`
  came back as 00000000000024052324 and `2R:A` as 1. `nR` constants are right
  justified with zero fill, `nH` and `nL` left justified and blank filled.
- **`MASK(n)`** is the n leftmost bits; **`SHIFT`** left is end around
  (`SHIFT(1,60)` = 1) and right is an ordinary shift.
- **`TIME(1.)`** is one word of display code, `" hh.mm.ss."` - a leading blank
  and then nine characters, which is why the program masks the top six bits off
  before comparing it with `9R06.00.00.`.
- **`SECOND(1.)`** is the job's CP seconds, and at the start of a job it really
  can be exactly 0.0.

## What the port had to change to run at all

Besides the mechanical edits (display code, octal constants, the overlays
becoming subroutines, the record manager becoming two arrays), two things are
not mechanical, and both are in `convert.py` with the reason written out:

- the six `PCT` draws hoisted out of compound conditions, above;
- `ISHFT`, the program's own, masked its argument **in place**
  (`VAR=VAR.AND.177777B`), so `ISHFT(1,N)` wrote 1 back over the constant 1.
  On the Cyber that is harmless, since 1 masked to 16 bits is still 1. Here it
  is a segmentation fault, so the mask goes into a local.

Neither changes what the program does, so there is nothing for `--no-fixes` to
turn off; it is accepted for the sake of the other ports in this collection.

## Options

    advent [--seed N] [--time HHMM] [--no-fixes] [-h]

`--seed` is the 48 bit seed, instead of `RANSET(SECOND(1.))` from the clock;
given, it also stops the game reseeding itself. `--time` holds the clock still,
which matters because **the cave is shut** from 06.00 to 11.30 and from 13.30 to
15.30 - the E.P.A. has stated that Middle Earth may not be entered during those
hours. The way in during prime time is the way the site meant: answer `YES` to
"perchance, are you a wizard?" and give the password, which is `WORMTONGUE`.

## Checked so far (first pass, 2026-09-21)

- `tests\cmpcyber.py`: three recorded NOS 1.3 sessions and the generator, all
  identical.
- The database loads with no `BUG` calls, 366 points, and the scoring, hints,
  palantir and prime-time paths all run.
- 40 fuzz runs of 400 random commands each (16000 commands): no crash, no
  `BUG` call, no port diagnostic.
- Builds clean with gcc/gfortran 15. The warnings that are left are the
  program's own: it passes the scalar `WSA3` where `GET` and `PUT` expect an
  array (as it did on the Cyber), and `RSPEAK` declares `/TXTCOM/` shorter than
  everyone else does.

## Still to do (refine pass)

- Play it through to a win, and a session with the cave closing and the
  repository - the endgame is the part no recorded session reaches yet.
- `MAINT`, the maintenance mode behind the magic words in section 12, and
  `HOURS` outside the open hours.
- The two long texts - the instructions (`Y` at the first question) and the
  `PKIHMN` inscription - are in session 3 only in part.
- The port keeps the message text in memory rather than in two files; writing
  TAPE2 and TAPE3 out would let them be compared with the Cyber's own copies.
