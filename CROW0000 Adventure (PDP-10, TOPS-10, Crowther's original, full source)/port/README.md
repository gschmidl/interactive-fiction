# ADV — a Windows port of Will Crowther's original Adventure

The first Adventure: Will Crowther's, written in FORTRAN on a PDP-10 and
abandoned unfinished in early 1976, before Don Woods found it and turned
it into the 350-point *Colossal Cave*. No score, no treasure to bank, no
save, no endgame — a cave, a lamp, a bird, a snake, three dwarves and
their knives.

What builds here **is that program**: `../src_original/ADV.F4` and
`../src_original/IOFIL.FOR` compiled, not a rewrite. `make` regenerates
`src/adv.f` and `src/iofil.f` from the untouched originals every time,
through `tools/convert.py` and `tools/patches.py`, so every difference
between the 1976 source and what the compiler sees is listed, justified
and auditable — run `python tools/showdiff.py -a` for the line-by-line
account.

**No line of game logic is changed.** Not one: the whole difference is
I/O, the word layout, and one line that makes the initialisation `PAUSE`
conditional.

## Build

Needs gfortran and Python 3.

```
make
```

produces `bin\adv.exe` and copies the database `ADV.DAT` next to it. The
executable reads `ADV.DAT` from the working directory, or from beside
itself, so it runs from anywhere.

## Play

```
bin\adv.exe                  play
bin\adv.exe -2               print exactly what the -10 printed
bin\adv.exe -i               start the way the pack's own ADV.EXE starts
bin\adv.exe -h               usage
```

| option | |
|---|---|
| `-2`, `--double-space` | print exactly what TOPS-10 printed, blank lines and trailing padding and all. By default the port single-spaces the text; see **Why Crowther's Adventure double-spaces** below. `ADV_VERBATIM=1` does the same. |
| `-i`, `--init-pause` | read the database and then stop at `PAUSE INIT DONE` and wait for `G`, which is what `RUN ADV` does on the pack. See **The initialisation pause** below. |
| `--echo`, `--no-echo` | echo typed lines, or do not. By default the port echoes only when its input is not a terminal, as the -10's terminal service did. `ADV_ECHO=0` or `1` does the same. |
| `-V`, `--version` | what this is |

There is no `QUIT`: the program has none. Ctrl-C leaves it, as Ctrl-C did
on TOPS-10.

Two things about playing it that are the program's, not the port's.
**Commands must be upper case** — neither the -10's terminal service nor
Crowther's `GETIN` folds case, so `no` is not `NO` and the game answers
the opening question as though you had said yes. And when you die, or
when the dwarves get you, you do not get a message and a new game: you
get FORTRAN's operator dialogue.

```
PAUSE
GAME IS OVER
Type G to Continue, X to Exit, T To Trace.
*
```

`G` puts you straight back where you were, because that is all `PAUSE`
does. That, more than anything else, is what "unfinished" means here.

## How the 36-bit machine was handled

This is DEC FORTRAN-10 for a 36-bit word. Text is not in `CHARACTER`
variables; it is packed **five seven-bit characters per word** and worked
on with bit arithmetic. The program says so itself, in `GETIN`:

```
DATA M2/"4000000000,"20000000,"100000,"400,"2,0/
```

— those are the low bits of the five character fields, at 29, 22, 15, 8
and 1, and `"774000000000` selects the first character. The port **keeps
that layout**, in an `INTEGER*8`, held sign-extended because bit 35 is
the sign on a -10. Because the bit layout is unchanged, every mask,
`.AND.`/`.XOR.` and five-character literal in the game logic keeps
working as written, and `GETIN` — which finds the gap between two typed
words and slides the second one out across a word boundary — needed no
change at all beyond where its characters come from.

**Crowther's own `SHIFT` is used, not a replacement.** `ADV.F4` contains
it, and it is 36-bit arithmetic that turns out to be exactly right in
64-bit signed integers once the octal constants are sign-extended: the
left shift masks to `"177777777777`, doubles, and adds `"400000000000`
when bit 1 was set, which in 64 bits is `-2³⁵` and lands the result in
`[-2³⁵, 2³⁵)` — precisely the signed 36-bit range. The 350-point port
had to supply its own; this one does not. (`make check-syms` runs `nm`
over the objects to prove that `SHIFT`, `SPEAK`, `YES` and `GETIN` are
all our own code and that nothing was quietly replaced by a library
routine of the same name.)

What the compiler genuinely cannot do goes through `src/runtime.f`:

* the **A edit descriptor** over packed words, both ways;
* DEC's free-format **`G`** descriptor — see below, it decides how the
  whole game prints;
* **`ACCEPT`**, **`ENCODE`**, and `OPEN(...ACCESS='SEQIN')`;
* **FORTRAN carriage control**, which gfortran cannot be told to apply
  to the preconnected unit 6;
* **`PAUSE`**'s operator dialogue;
* the library's **`RAN`**.

## RAN, read out of the machine

Woods wrote his own generator in FORTRAN, so porting his is a matter of
copying it. Crowther calls the **library's** `RAN`, and a port that
substitutes any other generator is a different game: the dwarves move
differently, the knives miss differently, you die in the dark at
different moments.

FORLIB's `RAN` was therefore read off the pack. A LINK map places the
module at 717–745 with two words of state at 746–747; dumping that
memory from a FORTRAN program (a `COMMON` array indexed past its end)
gives the whole routine:

```
RAN:    MOVE  0,746        ; the seed
        MUL   0,744        ; * 630360016, a 70-bit product in AC0:AC1
        DIV   0,745        ; / 2147483647
        MOVEM 1,746        ; seed := the remainder
        MOVSI 0,237000     ; exponent 2**31, fraction zero
        DFAD  0,742        ; + 0.0D0 -- normalises seed/2**31
        POPJ  17,
```

so it is a Lehmer generator, `seed := seed * 630360016 mod 2³¹-1`, and
the value returned is `seed/2³¹` normalised into a single-precision word:
27 fraction bits, truncated. `SETRAN`'s code alongside it shows the
reset seed in 747, `1777777` octal, and that is what 746 is *assembled*
with — so:

**the seed is 524287 at every start, the argument is ignored, and ADV is
completely deterministic.** There is no clock in the program at all.
Every run of the original tells the same story, and so does every run of
the port. That is what makes the verification below possible without
freezing anything.

The truncation matters. The constants `RAN` is compared against were
truncated too — `0.05` assembles as `174631463146` octal, which is
0.0499999998137…, not 0.05 — so the port folds each of the fourteen
constants to the value FORTRAN-10 actually assembled and carries `RAN`'s
result as the exact 27-bit value. A boundary draw then falls on the same
side on both machines. `tools/convert.py` checks its own folding against
the seven bit patterns measured on the pack and fails the build if one
disagrees.

## Why Crowther's Adventure double-spaces everything

Because of where DEC's free-format `G` descriptor leaves the scanner.

Every text line of `ADV.DAT` looks like this — `<TAB>` shown, and note
the blank before the zero:

```
1<TAB> 0YOU ARE STANDING AT THE END OF A ROAD BEFORE A SMALL BRICK
```

and it is read with

```
1004    READ(1,1005)JKIND,(LLINE(I,J),J=3,22)
1005    FORMAT(1G,20A5)
```

The `G` reads the `1`. The twenty `A5` fields then start **after the tab
and after the blank**, so the first character of the record the program
later writes is the `0` — and `0` is FORTRAN carriage control for
"advance two lines". FOROTS ate it and printed a blank line first. Had
the scanner stopped one character earlier, every line of the game would
have begun with a literal `0`; the original prints none, which is how
the rule was established. `GSKIP` in `src/runtime.f` is that one rule,
and the port applies the full carriage-control table rather than the
"every record starts with a blank, take it off" shortcut that was enough
for the Woods port:

| control | |
|---|---|
| `' '` | advance one line |
| `'0'` | advance two lines |
| `'1'` | form feed |

An empty record advances one line, which is what `FORMAT(/)` produces
twice and what puts the blank line between one speech and the next.

The other half of matching the original's output exactly is **trailing
blanks**. The text is printed as whole five-character words, so
`FORMAT(20A5)` pads the last one, and the original's transcripts carry
those spaces:

```
BUILDING . AROUND YOU IS A FOREST. A SMALL␣␣
```

`PUTLL` emits the record at exactly five characters per word. The
records built from a `FORMAT` instead go through `PUTRCS`, which trims —
safe because not one of those formats ends in a blank; they end in `.`,
`!`, `?` or `:`.

### and why by default it doesn't

All of that was fine on a hardcopy terminal and is mostly blank space on a
screen: a three-line room description takes eleven lines, and two-thirds
of any transcript is empty. So **the default output is single-spaced** — a
`'0'` advances one line like a blank, a run of blank lines collapses to
one, and trailing blanks come off. Nothing else changes: the same records
in the same order with the same text.

`-2`, which is what the -10 printed:

```
IN

YOU ARE INSIDE A BUILDING, A WELL HOUSE FOR A LARGE SPRING.



THERE ARE SOME KEYS ON THE GROUND HERE.



THERE IS A SHINY BRASS LAMP NEARBY.
```

the default:

```
IN
YOU ARE INSIDE A BUILDING, A WELL HOUSE FOR A LARGE SPRING.

THERE ARE SOME KEYS ON THE GROUND HERE.

THERE IS A SHINY BRASS LAMP NEARBY.
```

`-2` gets the -10's output back, and `-2` is what the verification below
compares — this is the one thing about the port's output that is
deliberately not the original, so it is not the thing being checked.

Every `FORMAT` statement in the program is left **exactly as written**,
including the string in `FORMAT 67` that runs across a continuation line.
gfortran's own format processor produces the records and `PUTRCS` applies
the carriage control, so the formats are never reinterpreted by hand.
(FORTRAN-10 does not pad a source line out to column 72, so that string
is a plain concatenation: `...DWARVES IN THE` + `  ROOM WITH YOU.`
That was measured, not assumed.)

## The initialisation pause

On the -10 the procedure was: `LOAD ADV.F4,IOFIL.FOR`, then `RUN`, which
reads `ADV.DAT`, stops at `PAUSE 'INIT DONE'` and hands you back to the
monitor, then `SAVE ADV` — and the saved image skips the whole thing next
time, because `SETUP` is no longer `0`.

Nobody did the `SAVE`. The `ADV.EXE` on the pack is 48 blocks, the same
size as a fresh `LOAD` and a fraction of the 380 blocks an initialised
image takes, so `RUN ADV` re-reads the database and stops at `INIT DONE`
every single time. The port has no core image either, so it also reads
`ADV.DAT` at every start; by default it then carries straight on, the way
the saved image would, and `-i` reproduces the shipped one:

```
PAUSE
INIT DONE
Type G to Continue, X to Exit, T To Trace.
*
```

This is the only line of the program the port makes conditional, and
both branches are behaviours the original has.

## Verification against the original

Not "it looks right". The original — compiled fresh **inside TOPS-10**
from the recovered `ADV.F4` and `IOFIL.FOR`, saved as `ADVNEW.EXE` — was
driven through twelve scripted sessions under SIMH, the port was driven
through the same twelve **with `-2`**, and the transcripts were compared
with `diff`.

Because `RAN` is seeded from nothing and the program never asks the time,
**no clock has to be frozen and no seed has to be forced**. Feed both the
same lines and every dwarf, every thrown knife and every death in the
dark must land on the same turn.

**All twelve are identical.** 7869 lines.

| case | what it exercises | lines | result |
|---|---|---|---|
| `open` | the opening, the surface, the building | 150 | identical |
| `instr` | the instructions branch, lost in the forest | 87 | identical |
| `grate` | keys, locking and unlocking, the two refusals | 152 | identical |
| `cave` | Hall of Mists, abbreviated descriptions, LOOK's limit | 284 | identical |
| `dwarves` | 90 turns of dwarves — movement, knives, the axe | 1004 | identical |
| `dark` | the 25%-per-move death, and `PAUSE GAME IS OVER` | 152 | identical |
| `pause` | the `PAUSE` dialogue: `G`, `T`, junk, an empty line, `X`, in both cases | 183 | identical |
| `objects` | every verb, and the objects with side effects | 175 | identical |
| `vocab` | case, unknown words, truncation, two-word forms | 445 | identical |
| `words` | where the second typed word starts, including past the twenty characters `ACCEPT` reads | 132 | identical |
| `fuzz1` | 260 pseudo-random commands above ground | 3059 | identical |
| `fuzz2` | 260 pseudo-random commands from the Hall of Mists | 2046 | identical |

`dwarves` and the two fuzz runs are the ones that matter: some 900 turns
between them, each drawing from `RAN`, and every draw has to fall on the
same side of the same truncated constant as the -10's did. Six thousand
lines of that agreeing is what actually pins the generator down.

Normalising is four things, and `verify.py` reports the last two per case
so the count can be checked:

* CRLF becomes LF;
* the one blank line TOPS-10's `RUN` command prints before a program
  starts comes off;
* one FOROTS **traceback** is deleted, in `fuzz2`, where one random reply
  to a `PAUSE` prompt began with T — and `fuzz2` contains exactly one
  such reply;
* one FOROTS **exit report** is deleted, at the very end of `pause`,
  where the case deliberately ends by answering `x`.

Every blank line the carriage control produced and every trailing blank
of a five-character word is compared as it came out.

The corpus is `../work/tests/cases/`, generated by
`../work/tests/gen.py`; `../work/tests/verify.py` runs both sides and
diffs them. The route into the cave in those cases comes from the travel
table in `ADV.DAT`, not from guesswork.

### The shipped ADV.EXE agrees too

Unlike the pack's `ADVENT.EXE` — which somebody had put through the Woods
version's `MAGIC MODE`, so it greets you as a time traveller and is not
what its source compiles to — `ADV.EXE` is untampered. Eight of the
twelve cases were driven through it as well (`verify.py --shipped-ref`,
then `--shipped`, which runs the port with `-i` and answers the `INIT
DONE` pause with `G`): `open`, `instr`, `grate`, `cave`, `dwarves`,
`dark`, `objects`, `vocab` — 2481 lines, all eight identical, each four
lines longer than the fresh-build run for the `INIT DONE` dialogue. So
the reference is not in doubt from either direction. The other four were
run only against the fresh build; the fuzz pair could not be run against
the shipped image at the time because the older corpus contained `XYZZY`,
and `X` at a `PAUSE` prompt ends the program.

### What two independent runs caught

The first pass showed one differing line in `objects`: the reference's
**echo** of a typed `EXTINGUISH` read `EXITNGUISH`, while the game's reply
was plainly the right one for `EXTINGUISH`. That is the emulated console
scrambling the echo of a fast character burst, not the program — and the
shipped-`ADV.EXE` pass, which typed the same line, echoed it correctly.
`run_original.py` now paces characters at 30 ms instead of 10, and the
transposition has not recurred. Cf. the harness's own comment.

## Differences from the original, and why

**The default output is single-spaced.** Described above. `-2` prints what
the -10 printed, and that is the mode the verification checks, so this
difference is a choice about the default and not an unverified claim.

**The monitor's blank line.** TOPS-10 printed one after you typed `RUN`
and before the program's first output. There is no monitor here, so the
port does not print it; `verify.py` accounts for it.

**`PAUSE`'s `T` reply prints nothing.** The dialogue is otherwise exact,
and it took measuring to get right: only the first letter of the reply
counts, `G…` resumes, `X…` stops the program, and **anything else,
including an empty line, asks again** — so a player who types a command
at the `GAME IS OVER` prompt simply gets the prompt back. `T…` asked
FOROTS for a call traceback and it printed four lines of addresses inside
the saved core image:

```
Name   (Loc)    <<---  Caller     (Loc)        Args  Types
TRACE. (573540) <<---  PAUS.+152  (PAUS.+152)   1     O
PAUS.  (73424)  <<---  MAIN.+2030 (MAIN.+2030)  1     K
```

There is no PDP-10 call stack here and inventing those numbers would be
worse than omitting them, so `T` asks again and prints nothing. That
makes the port's output a strict subset, and `verify.py` deletes exactly
that block from the reference — anchored on the prompt and the reply, and
it reports how many it removed so the count can be checked against the
number of `T` replies in the case.

**An `X` reply ends the program silently.** On the -10, FOROTS then
printed `CPU time … Elapsed time …` and `EXIT` and the monitor took over.
Neither the timings nor the monitor exist here.

**`-finit-local-zero`.** FORTRAN-10's loader zeroed core and the program
relies on it: `SETUP` is tested before it is ever assigned, `RAN(QZ)` is
called with a `QZ` that is never set, and `GETIN`'s `A(5)` is read but
only ever four words are filled. All three are zero here as they were
there.

**Two of the program's own loose ends are left exactly as found.**
Label `39` — a computed destination that sends you to room 66 or 77 — is
never branched to from anywhere; the special-destination list at 21 does
not contain it, so that code and the `RAN` call inside it are dead. And
the loop at 1006 that measures a text line scans `LLINE(I,20)` down to
`LLINE(I,1)`, while the text actually lives in words 3 to 22 — so the
last two words of a record can never be printed, and a line over 89
characters would be silently cut. No line in `ADV.DAT` is longer than
69, so it never shows; the port truncates at the same place regardless.

## What is in here

```
Makefile            build
tools/convert.py    the mechanical FORTRAN-10 -> gfortran conversion
tools/patches.py    the exact-line replacements, each one justified
tools/showdiff.py   line-by-line accounting of the whole difference
src/runtime.f       the machine layer: word packing, RAN, carriage
                    control, free-format input, PAUSE, OPEN
src/adv.f           GENERATED from ../src_original/ADV.F4 by make
src/iofil.f         GENERATED from ../src_original/IOFIL.FOR by make
bin/adv.exe         the game
bin/ADV.DAT         the database, copied unchanged by make
```

`src/adv.f` and `src/iofil.f` are generated; never edit them. Every
change lives in `convert.py` or `patches.py`, and `convert.py` fails the
build if any patch stops matching the original line it quotes.

## The difference, counted

`python tools/showdiff.py` over 763 lines in and 793 out:

| lines | why |
|---|---|
| 557 | DEC tab source form to fixed-form columns |
| 38 | exact-line patches from `patches.py` — 35 quoted originals (28 in `ADV.F4`, 7 in `IOFIL.FOR`), 4 of them deletions |
| 14 | real constant folded to the value FORTRAN-10 assembled, and `RAN` renamed `RAN10` |
| 11 | packed character literal to its word value |
| 9 | `PAUSE 'text'` to `CALL PAUSEM` |
| 9 | octal `"nnn` to the decimal value of those 36 bits |

Nothing is unaccounted for: `showdiff.py` re-derives each changed line
from the original by the same rules and reports `UNEXPLAINED` for
anything it cannot reproduce. There are none.
