# CRYSTAL CAVE — a Windows port

*CRYSTAL CAVE*, a text adventure written in DEC FORTRAN for the
DECsystem-10 and run on Tymshare's TYMCOM-X, by **John Kopf of
Tymshare**. It is the direct ancestor of the Univac 1100 game kept in
`Crystal Caves (Univac 1100, full source)/`, and it is about three months
older than it.

There is no source. This port runs the original binary on a PDP-10
emulator with just enough of the TOPS-10 monitor underneath it, so what
you play is the actual 1979 program, instruction for instruction — not a
rewrite.

```
 WELCOME TO THE CRYSTAL CAVE ADVENTURE!!  WOULD YOU LIKE
 INSTRUCTIONS?
 no

 YOU ARE STANDING AT THE END OF A ROAD BEFORE A BARN.  TO THE
 EAST IS A PASTURE.  TO THE WEST AND NORTH ARE WOODS.
```

## Where it sits in the family

The Univac version's own `DOC.TXT` gives the descent, and it is the
reason this folder exists:

> The Adventure game on which Cave was besed was devised by Willie
> Crowther at Stanford University, and later enhanced by Don Woods,
> Cave was produced from it by **John Kopf of Tymshare**, and enhanced by
> Bob Elman of Four Phase Systems. Cave was converted from DecSystem 20
> FORTRAN to Univac ASCII FORTRAN by Duff Kurland of Information Systems
> Design, Santa Clara, CA.
>
> — revision history: *January, 1980. Converted from DecSystem 20
> FORTRAN to Univac ASCII FORTRAN, adding stuff for compass.*

These are Tymshare's own tapes, and the two 1979 copies predate that
conversion. A second Tymshare game corroborates the attribution
independently: *EXPLORATION*, by Michael Stimac, credits itself to
"the 'CRYSTAL CAVE' by JOHN KOPF".

See [docs/COMPARISON.md](docs/COMPARISON.md) for what changed on the way
to the Univac version, transcript against transcript.

## Build

Needs a C compiler (gcc or clang; MinGW-w64 is fine) and Python 3 to
regenerate the embedded core image.

```bash
make
```

That produces a standalone `bin\cave.exe` — the core image is compiled
in, so the executable needs nothing else at runtime. In particular there
is no data file: on TYMCOM-X the game read its database once, and the
result was SSAVEd. What is on the tape is that saved image, database
already parsed. (The from-scratch build path is still in there — it is
what prints ` INITIALISING...` — but it is unreachable without the data
file it wants, which is not on any of these tapes.)

### The three builds

The tapes hold three distinct builds of one game. `make BUILD=1983` or
`BUILD=1984` selects the others; 1979 is the default.

| build | tape files | date | notes |
|---|---|---|---|
| **1979** | `upl\cave.sav.20`, `.22` | 1979-10-04 / 10-17 | two copies of one build |
| 1983 | `games\cave.sav` | 1983-03-29 | rebuild; addresses shift throughout |
| 1984 | `mpl\cave.sav` | 1984-12-27 | rebuild of the 1983 content, 433 words larger |

The game text is the same in all three apart from one message
(`YOU ARE DEAD!`) moving four lines earlier in 1983, so the Tymshare
line was rebuilt but never given the Univac-era rewrites. 1979 is the
default because it is the earliest and the only one whose repair is
exact — see below.

## Play

```bash
bin\cave.exe
```

You start at a barn at the end of a road, above Crystal Cave park.
Scoring runs to 500 points.

Only the first five letters of a word count, so `NORTHEAST` has to be
typed `NE`. `HELP` explains the parser, `INFO` covers the rules, and
`SCORE` tells you how you are doing — though be careful, in this version
`SCORE` goes on to ask whether you want to quit.

The wizard's magic word is `PHROG`, sitting in the clear in the image.
(The Univac build moved it behind a Fieldata cipher, which is why the
Univac port had to drop it.)

### Suspending

The game has no save file. On TOPS-10 you suspended it the way you
suspended anything else: you typed `SUSPEND`, the game told you to break
out and save your core image, and you did. This port keeps that model —

```bash
bin\cave.exe            # play, type SUSPEND
bin\cave.exe -c         # resume where you stopped
```

— and writes the image to `cave.core` at the moment the game asks for
it. The game refuses to resume an expedition that is too recent, which
was deliberate; `-t HH:MM` tells it a later time if you would rather not
wait. The same option gets you past the opening-hours check.

### Cave hours, the turn limit, and the wait to resume

CRYSTAL CAVE was a program on a commercial timesharing service, and it
polices the clock the way such a program had to. One of its rules still
bites a player today: **90 minutes** must pass before a suspended
expedition may be resumed.

The other two are in there but inert. The three prime-time masks in the
copy that survived are all zero, so the cave is open all day, every day;
a wizard can set hours, and after that visitors get a short expedition of
30 turns or nothing at all. An ordinary game is not counted at all --
only the short one is.

`-u` disarms all three, so `cave -c -u` picks a suspension back up the
moment you make it. Nothing else changes: without `-u` the program
behaves exactly as it always did, and `-t` still works if you would
rather lie about the time than lift the rules.

What `-u` writes to is the game's own core: the three prime-time hour
masks and the restart latency go to zero, and the branch that ends a
short expedition becomes `JFCL 0,0`. Patching the branch rather than the
count is what makes it work on a game resumed from a core image, where
the count is already set. The saved image stays honest: the originals go
back before `SUSPEND` writes `cave.core`, so a game saved under `-u` is
an ordinary saved game and continuing it wants `-u` again.

Those five words are not named in the source. This port builds more
than one core image, from compilations that do not agree about where
anything lives: in `cave-1983` the same five sit at
`055427`, `055430`, `055431`, `055443` and `004464`.
They are found instead, from code the hours routine cannot do without:
the `IMULI 2,2640` that turns days into minutes, the one comparison of
the result against the latency, the three `MOVE 2,mask` / `MOVEM 2,temp`
pairs above it, and the single place that steps the turn counter and
tests it against the short-game count. Each has to match exactly once
or `-u` refuses and says so, because poking a guessed address is worse
than not poking. `-v` prints what it found.

### Options

```
  -c, --continue      resume from a saved core image
  -f FILE             use FILE instead of cave.core
  -t HH:MM            tell the game it is HH:MM
  -q, --no-delay      skip pauses the game asks for
  -u, --unlimited     ignore cave hours, the turn limit and the wait
                      before a suspended game may resume
  -e, --echo          keep the program's own echo of what you type
  -v, -vv, -vvv       report monitor calls in increasing detail
  -T                  trace every instruction (very slow, very loud)
  -w ADDR             report every change to that octal core address
```

## The repair

Every one of the four tape files is exactly one 36-bit word short, and
the word is always missing from one of the same two places — file word
1850 or file word 3674. That is the identical defect the EXPLOR tapes
carry, at the identical offsets, in a different game, so it is an
artifact of how this collection was transferred rather than anything
about either program.

```
cave-upl22.sav  lost the word at 1850      cave-games.sav  lost 1850
cave-upl20.sav  lost the word at 3674      cave-mpl.sav    lost 3674
```

Each pair therefore repairs the other. For the 1979 pair this is exact:
the two files are copies of one build, so reconstructing from either
direction gives the same 29575-word sequence, and deleting either word
back out gives each original file again. `tools/mkimage.py` enforces
that round trip rather than assuming it.

For 1983 and 1984 it is not exact — those are two *different* builds
repaired from each other, so there is nothing to reconcile against. What
can be said is that each result parses as a clean `.SAV` chain landing
exactly on its `JRST` with one pad word to spare, and that both then
boot and play. That is strong, but it is not proof, and the tool says so
when you build them.

Two checks run on every build, because a one-word misalignment shifts
every packed character by 7 bits and turns the prose to noise:

* room 1 must decode to its known text, and
* the object mnemonic table must read `KEYS, LAMP, SEARS, RICK, …` — all
  39, in object order, ten characters apart.

`tools/mkimage.py`'s docstring carries the full derivation.

## Layout

```
raw/             the four tape copies the image is built from
src/pdp10.h      machine definitions
src/cpu.c        the processor: instruction set, floating point, byte pointers
src/monitor.c    the TOPS-10 side: UUOs, terminal, buffered device I/O
src/main.c       driver, core-image save and restore
src/image.c      generated — the core image as C data
tools/mkimage.py builds src/image.c from raw/, applying the repair
docs/            the comparison with the Univac version, and transcripts
```

`cpu.c` and `monitor.c` come from the EXPLOR port unchanged (bar the
default core file name). Both games are DEC FORTRAN programs off the
same tapes and ask the monitor for the same things.

## Notes and known limits

* **The image is a distribution copy, not a snapshot** — a fresh run
  deals a fresh game.

* **The program echoes your typing, not the terminal.** On the
  DECsystem-10 the host did not echo; F40's runtime carried the record
  it had just read into the output buffer when a `READ` was followed by
  a `WRITE` on the same unit. A Windows console echoes as you type, so
  the line would otherwise appear twice. When standard input is an
  interactive console the port drops the runtime's copy; when input is
  piped it keeps it, so transcripts read properly. `-e` forces it on.

* **It is 1979, so it is upper case.** Nothing is wrong with your
  terminal.
