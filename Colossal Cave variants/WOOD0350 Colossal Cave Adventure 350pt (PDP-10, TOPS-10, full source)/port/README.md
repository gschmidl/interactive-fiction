# ADVENTURE 350 — a Windows port of the TOPS-10 FORTRAN-10 original

Don Woods' complete 350-point *Colossal Cave Adventure*, in the DECUS
conversion Paul T. Robinson made at Wesleyan University in June 1980,
recovered from the TOPS-10 pack of *TOPS-10 in a Box* v1.1.

What builds here **is that program**: `../src_original/ADVENT.FOR`
compiled, not a rewrite. `make` regenerates `src/advent.f` from the
untouched original every time, through `tools/convert.py` and
`tools/patches.py`, so every difference between the 1977 source and what
the compiler sees is listed, justified and auditable — run
`python tools/showdiff.py -a` for the line-by-line account.

**No line of game logic is changed**, with one stated exception: three
conditions gain `.AND.UNLIM().EQ.0` (or `.OR.UNLIM().NE.0`), which is
what `-u` acts on. Without `-u` that function returns zero and those
three lines behave exactly as written.

## Build

Needs gfortran and Python 3.

```
make
```

produces `bin\advent350.exe` and copies the database `ADVENT.DAT` next to
it. The executable reads `ADVENT.DAT` from the working directory, or
from beside itself, so it runs from anywhere.

## Play

```
bin\advent350.exe                 play
bin\advent350.exe game.sav        resume a suspended game
bin\advent350.exe -u              lift the cave hours and the waits
bin\advent350.exe -h              usage
```

| option | |
|---|---|
| `-u` | lift the prime-time lockout, the demonstration-game turn limit, and the wait before a suspended game may be resumed. **Nothing else**: scoring, the random number generator and the game logic are untouched by it. |
| `-s FILE` | where `SUSPEND` writes (default `ADVENT.SAV`). |
| `-d DD-MMM-YYYY`, `-t HHMM` | freeze the clock. `ADVENT_DATE`, `ADVENT_TIME`, `ADVENT_SAVE` do the same. |
| `-v` | show the database initialisation report. |
| `ADVENT_ECHO` | `0`/`1` to force echoing of typed lines off or on; by default the port echoes only when its input is not a terminal, as the -10's terminal service did. |

Without `-u` the game behaves exactly as the source says, cave hours and
all: on a weekday between 08:00 and 17:59 it is closed to everybody but
wizards, who may offer you a thirty-turn demonstration game.

## How the 36-bit machine was handled

This is DEC FORTRAN-10 for a 36-bit word. Text is not in `CHARACTER`
variables; it is packed **five seven-bit characters per word** and worked
on with bit arithmetic. The program says so itself:

```
GETIN:   DATA MASKS/"4000000000,"20000000,"100000,"400,"2,0/
A5TOA1:  DATA MASK,BLANK/"774000000000,' '/
```

— the low bit of each of the five fields is at 29, 22, 15, 8 and 1, and
`"774000000000` selects the first character. The port **keeps that
layout**, in an `INTEGER*8`, held sign-extended because bit 35 is the
sign on a -10 and the program tests it (`A5TOA1` inserts a blank between
its second and third words only `IF(C.LT.0)`, which is true exactly when
the word's first character is 100 octal or above). Because the bit
layout is unchanged, every mask, `SHIFT`, `.AND.`/`.XOR.` and
five-character literal in the game logic keeps working as written, and
`GETIN` — which folds case arithmetically and slides the second word out
across a word boundary — needed no change at all beyond where its
characters come from.

`tools/convert.py` turns `"nnn` octal literals and `'ABCDE'` word
literals into the decimal values of those same 36 bits, and `TYPE n`
into `PRINT n`. What the compiler genuinely cannot do goes through
`src/runtime.f`:

* the **A edit descriptor** over packed words (`C2W`/`W2C`/`WSTR`), for
  the fifteen or so reads and writes that use it;
* the DEC **free-format `G` descriptor** (`RDFRE`), which reads integers
  from one record and leaves the rest of the list zero — the program
  depends on that zero fill, because a section 3 record carries a
  variable number of motion verbs;
* **`ACCEPT`** (`RDA5`), and the DEC `OPEN(...NAME=...,ACCESS='SEQIN')`;
* **`SHIFT`**, whose left shift would leave a result un-sign-extended in
  64 bits;
* `CALL DATE` and `CALL TIME` — see below.

`RAN` is **not** machine-specific and is kept exactly as written, a pure
integer LCG seeded only from the clock. It is renamed `RAN10`, because
plain `RAN` is a gfortran intrinsic and the program would have been
silently linked against the library's. (`make check-syms` runs `nm` over
the objects; every game routine is defined in our own objects and
nothing was replaced by a builtin.)

## The clock, and why a 1977 program still gets 2007 right

`DATIME(D,T)` returns days since 01-JAN-1977 and minutes past midnight.
The epoch matters: `START` uses `MOD(D,7).LE.1` to detect a weekend, and
01-JAN-1977 was a Saturday. The routine gets those numbers by decoding
the ASCII that DEC's `CALL DATE` and `CALL TIME` hand back, and **that
decoding is the only machine-specific part**, so the port keeps the whole
routine — leap years, `BUG(28)` and all — and replaces only the two
library calls, with routines that produce exactly the ASCII a TOPS-10
FOROTS produces.

The year is the interesting part. FOROTS writes `dd-mmm-yy` where `yy` is
*year-1900* formatted as two characters — so 2007 comes out as
`06-Jan-:7`, the tens digit having overflowed past `9` into `:`.
`DATIME`'s helper takes the low four bits of each character, so `:` reads
as 10 and the year decodes as 107, which is correct. The program is
accidentally Y2K-safe until about 2070.

This was not guessed. The original was run under SIMH with the pack's
clock frozen, and `MAGIC MODE`'s challenge word — which is computed
straight out of `D` and `T` — read back as `TEWYF` on 06-JAN-2007 12:00
and `EPZOP` on 23-NOV-2010 09:37. Those pin `D`=10962, `T`=720 and
`D`=12379, `T`=577: exactly the true days since 01-JAN-1977. The port
prints the same two challenge words.

Because `RAN` is seeded from `DATIME` and from nothing else, **freezing
the clock makes the whole game deterministic**, and the port and the
original produce identical random behaviour on the same date and time.
That is how this port is verified.

## Three things the terminal did, which the port now does

**Carriage control.** FOROTS took the first character of every formatted
record as carriage control and did not print it; a blank meant "one new
line". Every record this program writes starts with a blank — explicitly
`(' ',...)` or from an `nX` — so the rule is exactly "one blank comes off
the front of every record". gfortran cannot be told
`CARRIAGECONTROL='FORTRAN'` on the preconnected unit 6, so `convert.py`
applies the rule to the FORMAT statements instead (47 records). Without
it every line of the game would be indented one column further than the
original printed it.

**Tab stops.** 577 lines of `ADVENT.DAT` use a tab to separate sentences,
and two use tabs to indent. The -10's terminal service expanded them to
eight-column stops, so `WELCOME TO ADVENTURE!!<tab>WOULD YOU LIKE
INSTRUCTIONS?` printed with exactly two spaces. `SPEAK` and `MOTD` route
their text through `TABX`, which does the same.

**Echo.** The -10's terminal service echoed what you typed, which is why
the commands appear in a transcript of the original. A Windows console
echoes for itself, so the port echoes a line it has read **only when its
input is not a terminal** — that is, when a transcript is being captured.
`ADVENT_ECHO=0` or `=1` forces it either way.

## SUSPEND, and the core image

On TOPS-10 `SUSPEND` and `MAGIC MODE` both ended in `CIAO`, which stopped
the program and told the user to type `SAVE <file>` at the monitor
prompt; running that image again re-entered the program at its first
statement with `SETUP` still `-1` (a suspended game) or `2` (a version
tweaked in maintenance mode), and the program picked itself up at 8305 or
at 1. There is no core image on Windows, so **the program writes the file
itself** and says so instead of repeating magic message 32 ("BE SURE TO
SAVE YOUR CORE-IMAGE..."), which is no longer true:

```
 Your adventure has been saved in ADVENT.SAV.
 To resume it, run the game with ADVENT.SAV as its argument.
```

The capture is complete by construction rather than by inspection. The
whole core image used to persist, so every static variable had to: all
eight COMMON blocks, `MOTD`'s message of the day and `RAN`'s seed `R`
(both moved into COMMON blocks of their own for the purpose), and **every
variable of the main program**, which `convert.py` works out from the
declarations — 18 arrays and 127 scalars — and puts into a generated
`COMMON /ADVSTA/`. It then generates `STATIO`, which reads or writes all
of it, one call per variable: 13404 words, listed in `src/state.map`.
Nothing can be missed by hand, because nothing is listed by hand.

The latency rule is kept honest: the image records when it was made, and
`START` refuses an early resume exactly as the source says — under
`LATNCY/3` it says *EVEN WIZARDS HAVE TO WAIT LONGER THAN THAT!* and
stops, between `LATNCY/3` and `LATNCY` it lets a wizard through. Only
`-u` lifts it.

### The acceptance test

`python tools/savetest.py` plays a game to turn N, suspends, resumes and
plays on to turn M, and separately plays the same input straight through
to M with the clock frozen. It passes:

```
turns 1..N produce 82 identical lines
B has 72 lines after that; A and B share their last 75 lines
the whole continuation is identical after the resume: True
A prints 2 extra lines at the resume (label 8305 null move):
   |
   | YOU'RE IN DEBRIS ROOM.
state at turn M: 4 of 13404 words differ between the two runs
   ADVSTA/LIMIT     1 word(s)
   ADVSTA/TURNS     1 word(s)
   RANCOM/R         1 word(s)
   WIZCOM/SAVET     1 word(s)
```

Every line of the continuation is identical. The two extra lines are the
original's own design: 8305 does `K=NULL` and `GOTO 8`, a null move, so
the room is described again on the way back in. The test then goes
further than the transcript and compares the two *states* at turn M, word
by word, naming them from `src/state.map`: the four that differ are
exactly the four that must, because run A executed one extra `SUSPEND`
command — one more turn, one more tick of the lamp, one more call to
`RAN`, and a different suspension time. (If the final `LOOK` is dropped
from the test, `ABB` for the room joins them, by one: the null move's
description. That too is the original's behaviour.)

## MAGIC MODE — a worked example

`src_original/advent-magic-mode.py` computes the expected reply from the
challenge, the time and the magic number. It is an executable spec for
`WIZARD`, and the port accepts precisely what it computes. With the
clock frozen at 06-JAN-2007 12:00 and the shipped magic number 11111:

```
> MAGIC MODE
ARE YOU A WIZARD?
> YES
PROVE IT!  SAY THE MAGIC WORD!
> DWARF
THAT IS NOT WHAT I THOUGHT IT WAS.  DO YOU KNOW WHAT I THOUGHT IT WAS?
> NO
TEWYF                             <- the challenge
> PSEUO                           <- advent-magic-mode.py -m 11111 -t 12:00 TEWYF
OH DEAR, YOU REALLY *ARE* A WIZARD!  SORRY TO HAVE BOTHERED YOU . . .
```

(The shipped script is Python 2 — under Python 3 its one `map(ord, ...)`
dies — so `tools/magicmode.py` is the copy you actually run. Same
arithmetic. A second worked example, in prime time so that the cave
turns you away until you prove yourself: `advent350 -d 06-JUN-2007
-t 1000` challenges with `COBPX`, `magicmode.py -t 10:00 COBPX` answers
`MNOJV`, and the wizard is let into the closed cave.)

Maintenance mode then lets a wizard set the hours, the holiday, the
length of the demonstration game, the magic word and number, the latency
and the message of the day — and `CIAO` writes the result as a saved
version, which is what `RUN`ning it with `advent350 <file>` picks up.
That is exactly how the pack's own `ADVENT.EXE` came to greet you with
*HELLO, TIME TRAVELER* and to be open all day.

### Under `-u`, the game answers its own challenge

Doing the above by hand is awkward: the reply depends on the clock
rounded down to ten minutes, so it expires between reading it and typing
it. That is what the distribution's README means by "the timing is
tricky". With `-u`, the game simply tells you, as you are asked:

```
PROVE IT!  SAY THE MAGIC WORD!
 -u: the magic word is DWARF

THAT IS NOT WHAT I THOUGHT IT WAS.  DO YOU KNOW WHAT I THOUGHT IT WAS?

YZHCR
 -u: the reply is BSFRH
```

Both, because both are secrets — maintenance mode lets a wizard *change*
the magic word, and a changed one is otherwise unrecoverable. On a live
clock it also gives the next ten-minute window's answer
(` -u: from 21:50 it becomes KJDGI`), since that is exactly the trap;
with a frozen clock the answer cannot expire, so only one is shown.

This is not a second implementation of the rule. `WIZARD` already
computes the expected reply in order to check it, and `WZREPL` in
`src/runtime.f` is that loop copied statement for statement — so if a
wizard changes the magic number, the hint and the check follow it
together and cannot drift apart. It costs two lines in the generated
source, both calls that return immediately without `-u`, and nothing is
printed without it.

## Verification against the original

Not "it looks right": the original program, compiled fresh inside TOPS-10
from the recovered `ADVENT.FOR`, was driven through ten scripted
sessions, and the port was driven through the same ten and the
transcripts compared with `diff`.

The reference is a *fresh build*, not the `ADVENT.EXE` that shipped on the
pack. That one has been through MAGIC MODE — it greets you with "HELLO,
TIME TRAVELER" and was reconfigured for 24-hour access, neither of which
is in the source. Comparing against it would be comparing against
somebody else's edits.

What makes a byte-exact comparison possible at all is that **`RAN` is
seeded from the clock and nothing else**. Freeze the clock on both sides
and the whole game — dwarves, pirate, pit falls, which of three
interchangeable refusals a nonsense word earns — becomes deterministic.
Every test therefore pins an explicit date and time, and each reference
transcript was generated twice and kept only if the two runs agreed.

| session | what it exercises | lines | result |
|---|---|---|---|
| `open`    | opening text, first moves             |  95 | identical |
| `instr`   | the instructions branch               |  94 | identical |
| `walk`    | 130-command walkthrough, 5 treasures  | 545 | identical |
| `dwarves` | dwarves active — axes, pursuit        | 260 | identical |
| `vocab`   | nonsense, long and lower-case words   | 249 | identical |
| `hours`   | the prime-time lockout                | 231 | identical |
| `hours2`  | the boundary hours                    |  21 | identical |
| `death`   | dying, the reincarnation offer        | 111 | identical |
| `score`   | scoring and quitting                  | 139 | identical |
| `wizard`  | MAGIC MODE end to end                 |  51 | identical but for one line |

`dwarves` is the one that matters most: dwarf movement, whether a dwarf
throws, and whether the axe hits are all drawn from `RAN`, so 260 lines of
it agreeing turn for turn is what actually pins the generator down.

The one line is the deliberate difference: where TOPS-10 says `BE SURE TO
SAVE YOUR CORE-IMAGE...`, the port has no core image to save and says
where it actually put the file instead. See **Differences** below.

The corpus, the normaliser and the frozen clocks are in `../work/tests/`.

### The bug this found: .AND. is not short-circuit on a -10

Six of the eight matched almost at once. `vocab` did not, and the reason
turned out to be the most valuable thing in the whole exercise.

DEC FORTRAN-10 evaluated a logical expression whole. gfortran is free to
stop as soon as the result is settled, and does. That is invisible for
pure operands — but `PCT(N)` is `RAN(100).LT.N`, and `RAN` has a side
effect: it advances the generator. So every `PCT` that gfortran skipped
was a draw the -10 had taken, and the port's random stream slid quietly
out of step with the original's. Seven sites are affected:

```
    IF(LOC.LT.15.OR.PCT(95))GOTO 2000
6001    IF(PCT(50).AND.SAVED.EQ.-1)DLOC(J)=0
    IF(ODLOC(6).NE.DLOC(6).AND.PCT(20))CALL RSPEAK(127)
    IF(WZDARK.AND.PCT(35))GOTO 90
    IF(LOC.EQ.33.AND.PCT(25).AND..NOT.CLOSNG)CALL RSPEAK(8)
14  IF(NEWLOC.NE.0.AND..NOT.PCT(NEWLOC))GOTO 12
    IF(RAN(3).EQ.0.OR.SAVED.NE.-1)GOTO 9175
```

`WZDARK.AND.PCT(35)` is the one that bites first: it is reached every
turn, and `WZDARK` is false until you are in the dark, so the port was
one draw short before the player had done anything at all.

This is not a difference you would ever notice by playing — it produces a
game that behaves plausibly in every respect. It was caught only because
a frozen clock makes the original reproducible enough to diff against.
Each site now evaluates its `PCT`/`RAN` into a temporary first, so the
draw is taken whatever the rest of the condition does; the tests
themselves are unchanged, and `PCT` appears once in each, so forcing it
cannot alter the result of the expression — only the draw count. With
that fixed, `vocab` went from 89 differing lines to zero -- and `walk`
and `dwarves`, generated afterwards, matched first time.

## Differences from the original, and why

**The message of the day, and the cave hours.** The pack's `ADVENT.EXE`
is a *core image* somebody saved in 2011 after going through maintenance
mode: it greets you with `HELLO, TIME TRAVELER. WELCOME TO ADVENTURE ON
THE PDP-10.` and it reports the cave open all day. Neither of those is
in `ADVENT.FOR`; the source's `MOTD` message is empty (`DATA MSG/100*-1/`)
and `POOF` sets prime time to 08:00–17:59 on weekdays. A port of the
*source* cannot print a message nobody typed, so it does not. Type it in
through `MAGIC MODE`, save the version, and you have the 2011 image back.

**The startup report.** `Initializing...`, the table-space report and
`PAUSE 'INIT Done'` belong to the one-off first run, after which the
operator typed `SAVE ADVENT` and nobody saw them again. This port has no
core image to keep the database in, so it reads `ADVENT.DAT` at every
start; it does that silently, and `-v` shows the report. Nothing else
about the startup changed.

**`GETIN`'s `DIMENSION A(5)` became `A(6)`.** When a second word starts
in the fourth input word, `WORD2X` reads `A(J+2)` with `J=4`. On the -10
that read the word after the array, which happened to be `MASKS(1)`;
here it reads a zero. Only the echo of a command longer than fifteen
characters in an error message can tell the difference.

**Two declarations DEC did not insist on.** `FORCED` and `PCT` are
statement functions whose value is a logical but which are not in the
program's `LOGICAL` statement, and `WIZARD` is a `LOGICAL FUNCTION` not
declared in the two routines that call it. FORTRAN-10 tested such values
as logicals anyway; gfortran wants the declaration. Six words added to
three lines; no value changes.

**`PROP(SPICES)` is still wrong, and still there.** `SPICES` is never
assigned in this program — the Brown 448-point version defines it, this
one does not — so `IF(PROP(SPICES).LT.0)` at the troll bridge reads
`PROP(0)`. The bug is left exactly as found. It is now a read of the
word before `PROP` inside `COMMON /ADVSTA/` rather than of whatever the
-10's loader put there, which is at least well-defined.

**`-fdec` and `-finit-local-zero`.** The first gives the DEC bitwise
operators on integers, the second the zeroed core the program relies on:
FORTRAN-10's loader zeroed everything, and the program tests variables it
never assigns.

## What is in here

```
Makefile            build
tools/convert.py    the mechanical FORTRAN-10 -> gfortran conversion
tools/patches.py    the exact-line replacements, each one justified
tools/showdiff.py   line-by-line accounting of the whole difference
tools/savetest.py   the SUSPEND acceptance test
tools/magicmode.py  answers a MAGIC MODE challenge (Python 3)
src/runtime.f       the machine layer: word packing, clock, I/O, images
src/advent.f        GENERATED from ../src_original/ADVENT.FOR by make
src/state.map       GENERATED: what is in a saved image, word by word
bin/advent350.exe   the game
bin/ADVENT.DAT      the database, copied unchanged by make
```

`src/advent.f` and `src/state.map` are generated; never edit them. Every
change lives in `convert.py` or `patches.py`, and `convert.py` fails the
build if any patch stops matching the original line it quotes.

## The difference, counted

`python tools/showdiff.py` over 2949 lines in and 3220 out:

| lines | why |
|---|---|
| 176 | generated `STATIO`, the complete state capture (new code) |
| 102 | lines inserted by the patches below |
| 93 | packed character literal → its word value |
| 68 | exact-line patches from `patches.py` |
| 35 | carriage control: FOROTS ate the first blank of a record |
| 21 | `SHIFT` removed, superseded by `runtime.f` |
| 20 | `TYPE n` → `PRINT n` |
| 14 | page-separator form feeds removed |
| 11 | declarations added (`WSTR`, small buffers, `DAYS`) |
| 9 | `RAN` → `RAN10` |
| 5 | DEC octal literal → decimal |
| 3 | generated `COMMON /ADVSTA/` |
| 2 | combinations of the above |

Nothing is unaccounted for: `showdiff.py` re-runs the conversion rules on
each changed line and reports `UNEXPLAINED` for anything it cannot
reproduce. There are none.
