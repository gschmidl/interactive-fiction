# EXPLOR — a Windows port

*EXPLORATION*, a large text adventure written in FORTRAN (DEC F40) for the
DECsystem-10 and run on Tymshare's TYMCOM-X, by **Michael Stimac at
Tymshare, Cupertino CA**. The game says so itself:

```
 THIS PROGRAM IS BASED ON THE ORIGINAL 'ADVENTURE' BY WILLIE CROWTHER
 AS EXTENDED BY DON WOODS, AND THE 'CRYSTAL CAVE' BY JOHN KOPF.
 THIS VARIATION IS BY MICHAEL STIMAC AT TYMSHARE. CUPERTINO CA.
```

There is no source. This port runs the original binary on a PDP-10
emulator with just enough of the TOPS-10 monitor underneath it, so what
you play is the actual program, instruction for instruction — not a
rewrite. Roughly 96,000 characters of the author's prose are in there,
and every word of it comes off the tape.

See [docs/REVERSE-ENGINEERING.md](docs/REVERSE-ENGINEERING.md) for how the
file was decoded, including the two 36-bit words that had been lost in
transfer and how each surviving copy was used to restore the other.

## Build

Needs a C compiler (gcc or clang; MinGW-w64 is fine) and Python 3 to
regenerate the embedded core image.

```bash
make
```

That produces a standalone `bin\explor.exe` — the core image is compiled
in, so the executable needs nothing else at runtime.

## Play

```bash
bin\explor.exe
```

```
 WELCOME TO EXPLORATION ! DO YOU NEED INSTRUCTIONS ?
 NO

 YOU ARE IN THE MIDDLE OF A SMALL TOWN. THERE ARE MANY BUILDINGS
 AROUND YOU. A NEARBY STREET SIGN SAYS YOU'RE AT 7TH & PINE.
```

You start in a small town — Plovertown, with its 7th & Pine, its state
house, its library and its financial district — and work your way toward
the cave the game is really about. Scoring runs to 900 points.

Only the first five letters of a word count, so `NORTHEAST` has to be
typed `NE`. `HELP` explains the parser in the author's own words, `INFO`
covers the rules, and `SCORE` tells you how you are doing and what the
next rank needs.

### Suspending

The game has no save file. On TOPS-10 you suspended it the way you
suspended anything else: you typed `SUSPEND`, the game told you to break
out and save your core image, and you did. This port keeps that model —

```bash
bin\explor.exe            # play, type SUSPEND, answer yes
bin\explor.exe -c         # resume where you stopped
```

— and writes the image to `explor.core` at the moment the game asks for
it. The game refuses to resume an exploration less than 45 minutes old,
which was deliberate on the author's part; `-t HH:MM` tells it a later
time if you would rather not wait.

### Cave hours, turn limits, and the wait to resume

EXPLOR was a program on a commercial timesharing service, and it
polices the clock the way such a program had to. Out of the box:

* **Cave hours.** Monday to Friday the cave is shut 9-12 and 13-17 --
  the prime time the paying customers wanted the machine for. Turn up
  then and you get *"I'M TERRIBLY SORRY, BUT MYSTIC CAVE IS CLOSED"*,
  and from there you are a wizard, or a visitor allowed a 50-turn
  short exploration, or nothing at all.
* **A 900-turn limit** on an ordinary game, after which a wizard
  appears in green smoke and declares that *"THE EXPLORATION HAS
  LASTED TOO LONG"*.
* **45 minutes** before a suspended exploration may be resumed.

`-u` disarms all three, so the cave is open all day every day, a game
runs as long as you like, and `explor -c -u` picks a suspension back
up the moment you make it. Nothing else changes: without `-u` the
program behaves exactly as it always did, and `-t` still works if you
would rather lie about the time than lift the rules.

What `-u` writes to is the game's own core: the three prime-time hour
masks at `102614`-`102616` and the restart latency at `102630` go to
zero, and the two branches at `004605` and `004610` that act on the
turn counter become `JFCL 0,0`. Patching the branches rather than the
counts is what makes it work on a game resumed from a core image,
where the counts are already set. The saved image stays honest: the
originals go back before `SUSPEND` writes `explor.core`, so a game
saved under `-u` is an ordinary saved game and continuing it wants
`-u` again.

### Options

```
  -c, --continue      resume from a saved core image
  -f FILE             use FILE instead of explor.core
  -t HH:MM            tell the game it is HH:MM
  -q, --no-delay      skip pauses the game asks for
  -u, --unlimited     ignore cave hours, the turn limit and the wait
                      before a suspended game may resume
  -v, -vv, -vvv       report monitor calls in increasing detail
  -T                  trace every instruction (very slow, very loud)
  -w ADDR             report every change to that octal core address
```

## Layout

```
raw/             the two tape copies the image is built from
src/pdp10.h      machine definitions
src/cpu.c        the processor: instruction set, floating point, byte pointers
src/monitor.c    the TOPS-10 side: UUOs, terminal, buffered device I/O
src/main.c       driver, core-image save and restore
src/image.c      generated — the core image as C data
tools/mkimage.py builds src/image.c from raw/, applying the repair
tools/peek.py    disassemble the loaded image at an octal address
tools/dumptext.py dump the game's text, optionally filtered
docs/            the write-up
```

## Notes and known limits

* **The image is a distribution copy, not a snapshot.** Its state word
  (core location `102633`) reads 2 — "world tables built, no exploration
  in progress" — so a fresh run deals a fresh game with no fiddling.
  The from-scratch table build path, which prints ` INITIALISING`, is
  still in there but unreachable without the data file it wants.

* **The program echoes your typing, not the terminal.** On the
  DECsystem-10 the host did not echo; F40's runtime carried the record it
  had just read into the output buffer when a `READ` was followed by a
  `WRITE` on the same unit, and that carried copy is what put your
  command back on the screen. A Windows console echoes as you type, so
  the line would otherwise appear twice. When standard input is an
  interactive console the port drops the runtime's copy — the redundant
  one — and when input is piped it keeps it, so transcripts read
  properly. `-e` forces it back on.

* **Play is deterministic.** Nothing observed reseeds the generator from
  the clock, so a fresh game opens the same way every time and diverges
  as soon as your choices do.

* One TYMCOM-X-specific `CALLI 77740` at startup is not implemented; the
  program installs a trap handler with it and carries on regardless.
  Nothing else in a full playthrough goes unanswered.
