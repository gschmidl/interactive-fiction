# Adventure on the MCAUTO Cyber 74 - Windows console port

`advent.exe` plays both games Tony Jarrett and Paul Zemlin put on the MCAUTO
Cyber 74 in December 1978: the Black Wizard of the High East Tower asks whether
you want the basic cave (**BEG**, 350 points) or the advanced one (**ADV**, 500
points, with the castle north of the forest and the Black Wizard in its
dungeon). Run `run.bat`.

`src/convert.py` edits `ADVENT.txt` into one file gfortran will take - every
edit is a whole line and asserts how often it matches - writes the two
databases out as `databs1.txt` (BEG) and `databs2.txt` (ADV), and writes
`amaint.dat`, the wizard's parameter file. The port's CDC layer is
`src/port/pjaze.f`.

## Verified against the original running under NOS 1.3

FTN 4.7 on DtCyber's Cyber 173 compiles `ADVENT.txt` as it stands, so the
original is the reference:

    python tests\cmpcyber.py        1        IDENTICAL (63 lines, day 233, 0420)
                                    2        IDENTICAL (161 lines, day 233, 0423)
                                    3        IDENTICAL (115 lines, day 233, 0423)
                                    4        IDENTICAL (93 lines, day 233, 0427)

Sessions 1 and 2 are the basic cave - the surface, then thirty turns
underground with the dwarves; 3 and 4 are the advanced cave - the castle, a
dwarf in the courtyard, and the Black Wizard hurling lightning in the dungeon.
`tests\cyber\mkdeck.py` builds each deck and says in its header exactly what
differs from the tape, which is this:

- the three `CALL PFGET` lines become `CONTINUE` - they attached MCAUTO's
  permanent files, which do not exist here - and the deck supplies the
  database and the parameter file as local files;
- the parameter file is written by a ten-line program in the same job, from
  the values `POOF` still carries as comments (below);
- the game is loaded with `LDSET(PRESET=ZERO)`, because it needs its memory to
  start at zero (below).

The generator is the program's own (Palter's `RAN`) and is seeded from the
day and the minute, so `cmpcyber.py` runs the port at the minute the dayfile
says the job started, and the next one. The day is 233: DtCyber's NOS 1.3 has
`JDATE` give 26/09/21 as day 233, a month out, which `tests\cyber\probe4.txt`
shows; it does not matter to the port, only to reproducing a run.

## The site's characters

`ADVENT.txt` and the databases are a transcription in which three characters
of the site's set came out as something else, consistently:

| in the files | meant | example                        | count (source, BEG, ADV) |
|--------------|-------|--------------------------------|--------------------------|
| `%`          | `:`   | `READS%` - READS:              | 78, 15, 17               |
| `\`          | `?`   | `QUIT NOW\` - QUIT NOW?        | 8, 41, 41                |
| `^*`         | `!`   | `CAUTION^*` - CAUTION!         | 23, 97, 141              |

`^` never appears without `*` after it, and the only real `!` in the source is
in the FROG message Jarrett and Zemlin added themselves - their terminal had
one; the text that came from the PDP-11 did not. The port prints what was
meant; `cmpcyber.py` makes the same substitution on the Cyber's print file
before comparing. A fourth character cannot be undone: `"` is both the
apostrophe and the double quote (`WON"T`, `SAYS "MAGIC WORD XYZZY"`), so the
original could not tell them apart either, and the port prints `"`.

## Measured on the machine

The probes are in `tests\cyber\` (`probe4.job` and its printout), and the
FORLIB and display code measurements from the ACCA port apply unchanged.

- **The Cyber is ones' complement**: `-MASK(18)` came back as
  00000077777777777777, the bitwise complement, and `-1` as
  77777777777777777776. `A5TOA1` relies on it (`MASKC = -MASKC`), so the port
  says `PNOT`; with two's complement the punctuation after a word
  ("I SEE NO BALL.") comes out wrong.
- **`JDATE`** is five characters of binary zero and then `yyddd`; **`CLOCK`**
  is `" hh.mm.ss."`. `DATIME` takes both apart with `SHIFT` and `DECODE`.
- `"BEG"` is the same word as `3HBEG` (left justified, blank filled), `10H`
  with nothing after it is ten blanks, and `OR`, `AND`, `XOR` and `COMPL` are
  library functions.
- A list directed `PRINT*` starts in column 1 (after the carriage control);
  the two the program has are written as ordinary FORMATs here.

## Things the original does that the port keeps

- **It needs zeroed memory.** Section 9 of the database is read ten locations
  to a line (`READ(1,*)K,(TK(I),I=1,10)`) but walked twenty
  (`DO 1071 I=1,20`), so it depends on `TK(11)`-`TK(20)` being 0. They were on
  MCAUTO's Cyber, or the game could never have started; NOS 1.3's loader
  presets nothing, and without `PRESET=ZERO` the original dies in `INIT` with
  CPU ERROR EXIT 01. gfortran zeroes static storage, so the port needs no
  change.
- **The wizard's challenge is switched off.** "FOR NOW, IF THE PERSON KNOWS
  "DWAR" LET HIM BE A WIZARD": the magic word alone is enough. The challenge it
  skips packs its letters seven bits apart, the PDP-10's ASCII layout, which on
  a 60-bit display code machine would have printed nonsense.
- **MAINT saves the magic number as zero**: it writes `MAGNUM`, which is
  never set, where it means `MAGNM`.

## The wizard's parameter file

`POOF` reads `=AMAINT` - eleven words: the prime-time masks, the next holiday,
the short-game length, the magic word, the magic number and the restart
latency - and stops with `STOP 77` if it cannot. The file is not on the tape,
but `POOF` keeps what it used to set as comments:

    WKDAY="00777400"  WKEND=0  HOLID=0  HBEGIN=0  HEND=-1
    SHORT=30  MAGIC="DWAR"  MAGNM=11111  LATNCY=90

`"00777400"` is an octal constant in the PDP-10 dialect this came from; the
other ports of this program (Prime, SEL 32) have 261888, the same number. So
prime time is 08.00 to 17.59 on weekdays, when only a wizard gets a full game
and everyone else is offered a short one; the magic word is DWARF.
`convert.py` writes `amaint.dat` from those values, and the Cyber's own
parameter file was written from the same ones - the sessions agreeing is the
check.

## What the port had to change to run at all

Besides the mechanical edits (display code, octal constants, the random access
message file, `PFGET`, `BUFFER IN/OUT`):

- `MASKC = PNOT(MASKC)` for the ones' complement, above;
- eight draws from the generator hoisted out of compound conditions, because
  FTN never short circuits and gfortran does at -O2. One of them ends a DO
  loop, so its label stays on the last statement; the first version of this
  port put it on the first, which shortened the loop, and the fourth session
  found it by diffing a trace of every `RAN` call on both machines.

None changes what the game does, so `--no-fixes` has nothing to turn off.

## Options

    advent [--day N] [--time HHMM] [-u] [--no-fixes] [-h]

`--day` (day of the year) and `--time` hold the clock still. The generator is
seeded from them, so with both a run is reproducible; `--time` also decides
whether it is prime time.

`-u` (`--unlimited`) makes it never prime time. Everyone gets a full game at
any hour, and a suspended game can be resumed without the 90-minute wait.
`START` sets `PTIME` false and skips the latency check (two edits in
`convert.py`). Without `-u` the program behaves as on the Cyber. `run.bat`
passes `-u`.

## Checked so far (first pass, 2026-09-21)

- `tests\cmpcyber.py`: four recorded NOS 1.3 sessions, both caves, identical.
- 60 fuzz runs of 400 random commands, thirty in each cave: no crash, no
  `BUG`, no port diagnostic.
- The prime-time path: at 10.00 on a weekday a non-wizard is refused and
  offered the short game; DWARF gets in.
- Builds clean with gcc/gfortran 15, apart from the program's own warnings
  (common blocks declared at different lengths in different routines).

## Still to do (refine pass)

- Play both caves through to a win - the castle's treasures (the ruby, the
  oil paintings, the ivory talisman, the jade statue, the black opals, the
  Dead Sea scrolls, the ermine robe, the crown and the scepter) and the
  endgame are beyond every recorded session.
- `MAINT` and the wizard's new hours, and `SUSPEND`, which in this version
  asks you to save the core image.
- `PRINT*,LOC,RTXSIZ`, the database error path, is written with a FORMAT
  that matches what the Cyber printed for three-digit values only.
