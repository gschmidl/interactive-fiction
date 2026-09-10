# ADVENTURE 751 — a Windows port

**ADVENTURE < 6.1/ 3>, 14-Jan-82** — the 751 point Colossal Cave, the
one that grew at Carnegie-Mellon out of Don Woods's 350 points by way of
a 501 point version, with a safe behind the poster, matches, a cloth
bag, a castle, and a cave that used to keep opening hours.

It is a **FORTRAN-10 program**, and there is no source for it anywhere.
What survived is `<GAMES>ADVENTURE.EXE` on a TOPS-20 pack. So this is
not a rewrite and not a translation: it is the original 1984 binary,
running on an emulated DECsystem-10 processor, with the operating system
underneath it replaced by about fifteen hundred lines of C.

## Build

Needs gcc and Python 3.

```
make
```

produces `bin\adv751.exe`, which is self-contained — the game, the
FORTRAN runtime and all four of the game's databases are compiled into
it. Nothing needs to be installed and nothing needs to sit beside it.

## Play

```
bin\adv751.exe
```

Options:

```
  -p, --player NAME   play as NAME.  It goes in the scoreboard and the
                      gripe log; the default is your Windows user name.
      --no-delays     do not pause where the game pauses.
  -e, --echo          echo typed lines, as the terminal did.  On by
      --no-echo       default when input is a file, off at a console,
                      which echoes for itself.
      --time HH:MM    pretend it is this time of day.
  -v, --verbose       trace operating system calls to stderr.  Repeat
                      for more.
```

`SUSPEND` writes a saved game to a file in the current directory —
`MYGAME.DAT` if you call it MYGAME — and `RESUME` reads it back.
`GRIPE` appends to `ADVGRP.LOG`, and finishing well updates
`ADVWIN.LOG`; both start from the copies that were on the pack, and once
written they are picked up from disk in preference to the built-in ones.

## What is actually running

Four pieces of original code, none of them modified:

| | |
|---|---|
| `<GAMES>ADVENTURE.EXE` | the game — 143 pages, 73728 words |
| `<SUBSYS>FOROTS.EXE` | the FORTRAN-10 object time system, 22 pages |
| `<GAMES>ADVTXT.BIN` | the text, 25745 words, and encrypted |
| `<GAMES>ADVVAR.BIN` | everything else about the world, 15121 words |

plus `ADVWIZ.DAT`, which is the game's configuration file and, in its
last five lines, the list of people allowed to be wizards.

The game starts, sets up a stack, and asks the operating system for its
high segment:

```
127652  MOVEI 1,127661
127653  CALLI 1,40           ; GETSEG
127660  JRST  400010         ; into FOROTS
127661  SIXBIT "SYS"
127662  SIXBIT "FOROTS"
```

On the real machine TOPS-20 answered that through PA1050, the TOPS-10
compatibility package, which mapped `<SUBSYS>FOROTS.EXE` at 0400000.
Here `monitor.c` does the same thing from a copy of those pages built
into the program. Everything the game does afterwards goes one of two
ways: through FOROTS and out as a TOPS-10 UUO, or straight to a TOPS-20
JSYS. `monitor.c` answers the first and `jsys.c` the second, and PA1050
is not needed at all — which is the whole reason this is fifteen hundred
lines rather than fifteen thousand.

## Reading the machine instead of guessing at it

Several things about the interface between FOROTS and the monitor are
not written down anywhere I could reach, and getting them wrong produces
a program that runs perfectly and prints nonsense. Three of them were
settled by reading the original code, and two by asking the original
machine.

**IN and OUT take their error return by skipping.** This is the opposite
way round from LOOKUP and ENTER, and the opposite of what I expected.
FOROTS settles it by laying the two returns out in the open:

```
402373  XCT   0,0            the OUT
402374  JRST  402411         carry on
402375  XCT   4,413623       report an error
```

A UUO executed by XCT skips the word after the XCT, so the skip return
is the one that lands on the error reporter.

**The file status word lives in the right half.** With `IO.EOF` put in
the left half instead, end of file never gets through: the program reads
to the end of the last buffer and then loops forever asking for a record
that will never come. FOROTS gives it away by clearing `IO.ACT` with a
`TRZ`, which only reaches the right half.

**A buffer's byte pointer is word-shaped in the binary modes.** In ASCII
the header holds `440700,,buf+2`, an ordinary seven bit pointer sitting
one byte before the data. In the word modes it is `004400,,buf+1`, and
FOROTS does not even ILDB it — at 0402130 it steps the pointer with an
`AOS` and takes the word it then addresses. Point it at `buf+2` and
every binary record is read one word late, so the FORTRAN record control
word is missed and a 15000 word database looks like garbage.

**DEVCHR and DEVTYP** were not guessed. A short MACRO-10 program
assembled and run on the same pack —

```
        MOVE  1,[SIXBIT /TTY/]
        CALLI 1,4               ;DEVCHR
        ...print AC1 in octal...
```

— printed the exact words PA1050 was handing FOROTS, and those are the
constants in `monitor.c`. It matters more than it looks: with a
plausible-but-wrong terminal word, FOROTS decides the terminal is a file
and writes FORTRAN records to it verbatim, so every line of the game
comes out with the carriage control character still on the front:

```
     -- ADVENTURE < 6.1/ 3>, 14-Jan-82  --      wrong
    -- ADVENTURE < 6.1/ 3>, 14-Jan-82  --       right
```

The same probe answered `GETTAB` item 17 of table 11, the word FOROTS
looks at before it will run at all — without it, `%FRSSYS MONITOR not
built to support FOROTS`.

## Verifying it

The same scripts were played on the original TOPS-20 system under a
DECsystem-10 simulator and on this port, and the transcripts compared
line by line. `tests\` holds five of them — walking about, handling
objects, magic words, nonsense verbs, `HOURS`, being teleported into the
dark by `PLUGH`, dwarves, and two deaths down a pit — 1044 lines of
output in all, and every line matches. The only systematic difference is
that the real system echoed what you typed in upper case, because that
terminal was set to; the comparison folds case.

```
tests\run.cmd
```

replays them. The game is deterministic — the same commands produce the
same play whatever the clock says — which is what makes a comparison
like this possible.

## Differences from the pack

* **The player's name.** The game asks the monitor who is logged in and
  puts the answer in the scoreboard. Here that is your Windows user
  name, or whatever `--player` says.
* **Wizards.** The last five lines of `ADVWIZ.DAT` are the numbers of
  the CMU accounts allowed to be wizards, and the game also asks an
  access control job for permission. There is no such job here — and
  there was none on the pack either, so `GETOK%` answers the way it did
  there. Handing the game one of those five numbers instead of the
  player's own changes nothing that can be seen from outside, so there
  is no switch for it.
* **Opening hours.** The cave keeps hours, but they were switched off in
  this build; `HOURS` says so itself.
* **The year.** The game prints two digit years by dividing, so a date
  in 2026 comes out as `<6`. That is what the original does with a clock
  this far past 1982, and the machine it came off does the same.

## Files

```
src/cpu.c        the processor: the full non-privileged instruction set,
                 floating point, byte instructions, ADJBP, and the two
                 call gates -- UUO and JSYS
src/monitor.c    the TOPS-10 side: terminal, channels, buffer rings,
                 disk files, GETSEG
src/jsys.c       the TOPS-20 side: the fifteen JSYSes the game makes
                 for itself, including the PMAP it reads its text with
src/image.c      generated: the game and the FOROTS high segment
src/data.c       generated: the game's data files
tools/mkimage.py builds those two out of ../dump_original
```

`..\dump_original` holds the files exactly as they came off the pack,
five eight-bit frames to a 36-bit word unpacked into eight-byte
big-endian words.
