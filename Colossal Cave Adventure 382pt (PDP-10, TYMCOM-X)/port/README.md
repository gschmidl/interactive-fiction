# ADVENTURE, 382 points — Tymshare's DECsystem-10 build, as a Windows port

Don Woods' *Colossal Cave Adventure* as **Tymshare** ran it on TYMCOM-X,
extended in-house from 350 points to **382** with a rope, a mithril mail
coat and a ring of adamant. There is no source. This port runs the
original FORTRAN binary on a PDP-10 emulator with just enough of the
TOPS-10 monitor underneath it, so what you play is the actual program,
instruction for instruction — not a rewrite.

```
 WELCOME TO ADVENTURE!!  WOULD YOU LIKE INSTRUCTIONS?
 no

 YOU ARE STANDING AT THE END OF A ROAD BEFORE A SMALL BRICK BUILDING.
 AROUND YOU IS A FOREST.  A SMALL STREAM FLOWS OUT OF THE BUILDING AND
 DOWN A GULLY.
```

## Which 382 is this?

Not the one already in this collection. `..\Colossal Cave Adventure
382pt (PL1, mainframe, full source)\` holds Gary Palter's PL/I *Version
4.0*, which also totals 382 — and gets there with **a great red ruby and
Orac the super computer**, a teleport bracelet and a force field. Its
database contains no rope, no ring and no mail coat; the word for
"mithril" or "adamant" appears in it zero times.

Tymshare's 382 spends its extra 32 points on Tolkien instead:

| | Woods 350 | Tymshare 382 | PL/I "Version 4.0" 382 |
|---|---|---|---|
| extra treasures | — | ring of adamant, mithril mail coat | great red ruby, Orac |
| extra tool | — | 50-foot coil of rope | teleport bracelet |
| new verbs | — | TIE, UNTIE, CUT | — |

Two independent expansions of the same 350-point game that happen to
land on the same total. See [docs/COMPARISON.md](docs/COMPARISON.md).

The tapes say so themselves. `..\dump_original\advent.hlp.jms-info` is a
contemporary walkthrough sitting in another user's directory on the same
collection:

> ALSO, THESE INSTRUCTIONS ARE ALSO FOR THE EXPANDED VERSION TO BE FOUND
> AT TYMSHARE, INC. OF CUPERTINO, CALIFORNIA, AND THERE IS A GRAND TOTAL
> OF 382 POINTS THAT YOU MUST GET TO BECOME AN ADVENTURE GRANDMASTER.
> [...] THE NEXT SERIES OF ___ STEPS NEEDS ONLY TO BE EXECUTED IN THE
> GAME'S 382 PT. VERSION.

— and the steps it then lists are `GET ROPE`, `TIE ROPE`, `CUT ROPE`,
`GET COAT`, `GET RING`. That walkthrough is what
[docs/NEW-AREAS.md](docs/NEW-AREAS.md) was checked against, and the
scripted sessions in `tests/` replay it.

## Build

Needs a C compiler (gcc or clang; MinGW-w64 is fine) and Python 3 to
regenerate the embedded core image.

```bash
make
```

That produces a standalone `bin\advent-1979.exe` — every executable is
named for the build it came from — and the core image is compiled in, so
it needs nothing else at runtime. In particular there is no data file: on TYMCOM-X the game read its database once, and the
result was SSAVEd. What is on the tape is that saved image, database
already parsed.

`make verify` prints the evidence for the repair described below without
building anything.

### The builds

The five tape files hold **two compilations of one game**, and the
Makefile can build either — plus the hand-patched copy of the later one,
which is a third thing worth having on disk even though it plays the
same.

Plain `make` builds whichever one you ask for with `BUILD=`, 1979 if you
say nothing. **`make builds` builds all three at once**, and that is all
`bin\` ever holds — one file per build, no plain `advent.exe` that would
only be a duplicate of one of them:

```
bin\advent-1979.exe     the reference build
bin\advent-1981.exe     the hand-patched copy of it
bin\advent-1978.exe     the earlier compilation
```

| `BUILD=` | executable | reconstructed from | |
|---|---|---|---|
| **1979** *(default)* | `bin\advent-1979.exe` | `upl\advent.sav.20` (1979-10-17)<br>+ `pointerc\madven.sav` (1985-02-20) | the reference. Two copies of one compilation that lost different words, so they reconcile exactly |
| 1981 | `bin\advent-1981.exe` | `games\advent.sav` (1981-02-25)<br>+ `upl\advent.sav.20` | the same program with two instructions patched by hand — see *The KI-10 patch* below |
| 1978 | `bin\advent-1978.exe` | `upl\advent.sav.17` (1978-11-22),<br>the only copy of its compilation | an earlier compilation of the same game; different addresses throughout, start `014007` rather than `014002` |

The dates are the tape files'. `pointerc\madven.sav` is six years
younger than `upl\advent.sav.20` and still the same compilation: repair
each from the other and the two results agree at every one of their
26793 words. That is what makes the pair the reference.

They play identically. `make test` builds all three, drives them with
the same scripted input and checks the transcripts match byte for byte —
they do, through the whole rope puzzle. To play a particular one, run it
directly:

```bash
bin\advent-1978.exe          # the 1978 compilation
```

`carl\advent.sav` (1988) is a fourth copy of the 1979 build with one
512-word tape record destroyed; it is kept because it independently
corroborates the repair, not because anything is built from it.

## Play

```bash
bin\advent-1979.exe
```

Scoring runs to 382. Only the first five letters of a word count, so
`NORTHEAST` has to be typed `NE`. `HELP` explains the parser, `INFO`
covers the rules, `HOURS` gives the cave's opening times (this image was
saved with them set to OPEN ALL DAY, so you will never be turned away),
and `SCORE` tells you how you are doing — though be careful, `SCORE`
goes on to ask whether you want to quit.

The magic words are the usual `XYZZY`, `PLUGH`, `PLOVER` and
`FEE FIE FOE FOO`.

### Not deterministic — and how to make it so

The program seeds its random numbers from the time of day. Two runs a
minute apart take different dwarf rolls from the same input; two runs
within the same minute agree. `-t HH:MM` pins the clock, which is what
makes the scripted sessions in `tests/` reproducible.

### Suspending

The game has no save file. On TOPS-10 you suspended it the way you
suspended anything else: you typed `SUSPEND`, the game told you to break
out and save your core image, and you did. This port keeps that model —

```bash
bin\advent-1979.exe       # play, type SUSPEND
bin\advent-1979.exe -c    # resume where you stopped
```

— and writes the image to `advent.core` at the moment the game asks for
it. The game refuses to resume an expedition suspended less than 90
minutes ago; `-t HH:MM` tells it a later time if you would rather not
wait.

Resume too early and it offers the wizard's door instead: *ARE YOU A
WIZARD? / PROVE IT! SAY THE MAGIC WORD!* The word this image wants is
**DWARF** — every other word is answered with *FOO, YOU ARE NOTHING BUT
A CHARLATAN!*, while DWARF gets the bluff (*THAT IS NOT WHAT I THOUGHT
IT WAS. DO YOU KNOW WHAT I THOUGHT IT WAS?*, to which the honest answer
is no). Behind that is a second challenge — the program prints a
five-letter word and wants it transformed by the "magic number", which
is not recoverable from the tape, so wizard mode stays shut.

### Cave hours, the turn limit, and the wait to resume

ADVENTURE was a program on a commercial timesharing service, and it
polices the clock the way such a program had to. One of its rules still
bites a player today: **90 minutes** must pass before a suspended
expedition may be resumed.

The other two are in there but inert. The three prime-time masks in the
copy that survived are all zero, so the cave is open all day, every day;
a wizard can set hours, and after that visitors get a short expedition of
30 turns or nothing at all. An ordinary game is not counted at all --
only the short one is.

`-u` disarms all three, so `advent -c -u` picks a suspension back up the
moment you make it. Nothing else changes: without `-u` the program
behaves exactly as it always did, and `-t` still works if you would
rather lie about the time than lift the rules.

What `-u` writes to is the game's own core: the three prime-time hour
masks and the restart latency go to zero, and the branch that ends a
short expedition becomes `JFCL 0,0`. Patching the branch rather than the
count is what makes it work on a game resumed from a core image, where
the count is already set. The saved image stays honest: the originals go
back before `SUSPEND` writes `advent.core`, so a game saved under `-u` is
an ordinary saved game and continuing it wants `-u` again.

Those five words are not named in the source. This port builds more
than one core image, from compilations that do not agree about where
anything lives: in `advent-1978` the same five sit at
`050644`, `050645`, `050646`, `050660` and `004121`.
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
  -f FILE             use FILE instead of advent.core
  -t HH:MM            tell the game it is HH:MM
  -q, --no-delay      skip pauses the game asks for
  -u, --unlimited     ignore cave hours, the turn limit and the wait
                      before a suspended game may resume
  -e, --echo          keep the program's own echo of what you type
                      (kept anyway when input is piped, not a console)
  -h, --help          this text

diagnostics:
  -v, -vv, -vvv       report monitor calls in increasing detail
  -T                  trace every instruction (very slow, very loud)
  -w ADDR             report every change to that octal core address
```

## How the port works

`src\cpu.c` is a KA/KI-10 emulator, `src\monitor.c` answers the TOPS-10
UUOs that FOROTS (the FORTRAN-10 object time system) makes, and
`src\image.c` is the repaired core image compiled to a C array. All
three come from the EXPLOR and CRYSTAL CAVE ports in this collection
unchanged apart from the default core-file name: those are DEC FORTRAN
programs off the same tapes, and they ask the monitor for the same
things.

### The repair

Every one of the five tape files is exactly one 36-bit word short, and
the word is always missing from the same two places — file word **1850**
or file word **3674**. That is the identical defect the EXPLOR and
CRYSTAL CAVE tapes carry, at the identical offsets, so it is an artifact
of how this collection was transferred and not something about any one
game.

Two copies that lost the *same* word agree everywhere. Two that lost
*different* words agree outside `[1850, 3674)` and, inside it, the
early-loser reads exactly one word ahead of the late-loser. That shift
window is what tells the two groups apart, and `tools\mkimage.py` checks
it word for word rather than assuming it:

```
lost 1850:  advent-games.sav, advent-pointerc.sav
lost 3674:  advent-upl20.sav, advent-carl.sav, advent-upl17.sav
```

For the 1979 build each side therefore supplies what the other is
missing, and the restored words come out the same from two independent
donors each:

```
word 1850 = 325000003621   from upl20  and again from carl
word 3674 = 321100007276   from pointerc and again from games
```

Rebuilding from either direction gives the same 26793-word sequence;
deleting the restored word gives each original back; and the result
parses as a clean `.SAV` chain — 373 blocks, 26418 words at
`000120..072342`, landing on `JRST 014002` with exactly one pad word to
spare.

**The 1978 build has no donor.** It is the only copy of its
compilation. It lost its word at 3674 — inserting a placeholder at 1850
instead yields an image that prints the first room and then stops dead,
while 3674 plays. The hole lands at address `007256`, in the middle of
this:

```
007244  MOVEI 3,107        007250  MOVEI 4,101       007254  MOVEI 5,76
007245  CAME  3,014627     007251  CAME  4,014627    007255  CAME  5,014627
007246  TDZA  3,3          007252  TDZA  4,4         007256  <missing>
007247  SETO  3,0          007253  SETO  4,0         007257  SETO  5,0
```

so the missing word is `TDZA 5,5` = `634240000005`. The 1979 build
carries the same routine five words lower, and its `007251` — a word
proven by donor reconciliation, not inferred — is exactly
`634240000005`. Two independent lines of evidence, and nothing borrowed
from anywhere the file did not already point.

### The KI-10 patch

FOROTS opens with the standard processor test:

```
        SETO  0,            ; AC0 := -1
        AOBJN 0,.+1         ; a KA-10 carries between halves, a KI-10 does not
        JUMPE 0,ok          ; zero => KI-10
        OUTSTR "?KI-10 CODE WILL NOT RUN ON A KA-10"
        EXIT
```

and stores the same answer as a flag for the floating-point routines.
In `games\advent.sav` — and in none of the other four — **both** AOBJN
instructions are replaced by `SETZ 0,`, which hard-wires the answer to
"KI-10" and skips the test. Someone patched that copy by hand.

It changes nothing here: the emulator increments the two halves
independently, as a KI-10 does, so all five images take the same path.
`BUILD=1981` exists because the patch is real history, not because it
plays differently.

## Layout

```
bin\        one executable per build: advent-1979, -1981, -1978
raw\        the five tape files, renamed by the directory they came from
src\        the emulator, the monitor, the driver, and the generated image
tools\      mkimage.py -- repairs the tape files and writes src\image.c
tests\      scripted sessions; runall.sh checks all three builds agree
docs\       the transcripts those tests produce, and the write-ups
```
