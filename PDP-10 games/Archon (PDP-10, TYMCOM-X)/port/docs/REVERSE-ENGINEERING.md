# Reading the ARCHON tape files

What arrived was three files and nothing else:

```
archon.shr    85,455 bytes
archon.low   192,430 bytes
blocke.low   193,665 bytes
```

One high segment and *two* low segments. Compared by address, the two
low segments differ in 407 words out of 38,000, and every one of them is
runtime state: object positions, object states, the clock block, scratch
variables. Not one word of the messages, the travel tables, the zones,
the location conditions, the vocabulary or the initial-placement tables
differs. They are the same build saved at two different moments —
`archon.low` at state 2, nothing carried, the distribution copy; and
`blocke.low` at state −1 with four objects in hand, somebody's suspended
game. The port is built on `archon.low`.

No source, no documentation, no file-format note. This is how they were
decoded, what turned out to be wrong with them, and what the program is.

---

## 1. The word packing

Both lengths divide by five, which is the first hint: the PDP-10 has a 36-bit
word, and the usual way to get one onto eight-bit media is five bytes per word,
one 7-bit character each, with the spare 36th bit tucked somewhere.

A hex dump settles it — the text reads straight off:

```
000000b0: 6f75 2772 6520 6261 636b 2069 6e73 6964   ou're back insid
000000c0: 6520 6120 726f 636b 2061 6761 696e 2121   e a rock again!!
```

So byte *k* holds character *k* right-justified, and the word's last bit is the
high bit of byte 4. The confirmation is arithmetic rather than visual. Under
that rule the first word of `blocke.low` decodes to

```
777777,,000114
```

which is an IOWD — `<-count,,address-1>` — asking for one word to be loaded at
location `115` octal. `115` is `.JBHRL` in the TOPS-10 job data area, the word
that records the high segment's size, and the word actually loaded is

```
041303,,441777
```

`041303` octal is 17,091 decimal, which is exactly the number of five-byte
groups in `archon.shr`. The packing is right, and the two files belong
together.

## 2. The container formats

`blocke.low` is a TOPS-10 `.SAV` low segment: a chain of IOWD blocks, each a
header word followed by that many data words, ending in a `JRST` to the start
address. Walking it gives

```
526 blocks, 38,206 words, loaded at 0115 through 0125600
terminator: JRST 400010
```

and the cross-check is exact: `.JBFF` (location `0121`), the program's own
record of the first free location, holds `0125601` — one past the last word
loaded. Nothing is missing from the block structure.

`archon.shr` has no block structure. It is a raw image of the high segment,
loaded at `0400000`, and its first words are the high segment's vestigial job
data area, whose contents mirror the low segment's.

`.JBHRL`'s right half is worth a second look, because the two low segments
disagree about it. `blocke.low` rounds it up to `0441777`, the top of a
34-page segment. `archon.low` gives `0441303` — and `0400000` plus the
17,092 words of the *repaired* high segment, less one, is `0441303`
exactly. Without the restored `PUSHJ` the segment would be 17,091 words
and would end one word lower. The distribution copy's own job data
corroborates the high segment's repair, independently of the alignment
argument that found it.

## 3. Two lost words

No file was intact. Every one had lost the word at file index **3674** — the
same glitch, in the same place, in each file. Because the two low segments
sit one word apart (`archon.low` starts its big block at `000146`, where
`blocke.low` starts at `000145`), the same file offset lands on adjacent
words of the same sentence, and each copy proves the other's repair.

### the low segment — a missing "h wal"

The game's message table is a linked list. Each node is one link word followed
by the text of that line, packed five characters to a word:

```
0777777770702   <- link
"You'r" "e at " "the b" "ase o" "f the" ...
```

The link is ±(offset from a fixed base) of the next node; the sign marks the
first line of a message, so a message runs from a node with a negative link
through every following node with a positive one. The list ends at a link
of -1.

Chasing the list desynced after 335 nodes: the link at node 3668 pointed one
word past where the next node actually sat. In `archon.low` that node reads

> You're at the base of the nort**l, by** the gate.

and in `blocke.low`, one word the other side of the same gap,

> You're at the base of the **h wal**l, by the gate.

Between them the line is *"You're at the base of the north wall, by the
gate."*, so `archon.low` has lost `"h wal"` (`0641016760730`) and
`blocke.low` has lost `" nort"` (`0203355771350`).

Neither needs the other to be sure of it. `blocke.low` can copy its `" nort"`
from the two rooms immediately after, "You're at the base of the north
tower." and "…the northwest…", which encode as `" nort"` + `"h tow"` /
`"hwest"`. `archon.low` has no such donor — no other word in the file reads
`"h wal"` — and does not need one: the sentence and the room's own name fix
the five characters, and five characters of A5 pack to exactly one word.

Putting it back does three things at once: the chase runs all 2,533 nodes to
the sentinel and yields 1,437 clean messages; the big block's data-word count
matches the count in its own IOWD header; and the loaded data stops at
`.JBFF` (four words short of it in `archon.low`, where SAVE dropped a
trailing run that was zero). Three independent checks that were all failing
now all pass.

### the high segment — a missing PUSHJ

The high segment gives no such linked structure, so the gap had to be found by
alignment. FORTRAN-10 puts each subprogram's SIXBIT name in the word
immediately before its entry point, so any `PUSHJ 17,X` should have a
name-shaped word at `X-1`. Scanning every call target:

```
400015 TOTING   shift 0
400026 AT       shift 0
...
414575 FIGHT    shift 1      <- one word late
421731 DATIME   shift 1
422344 BUG      shift 1
```

Everything below roughly `0407000` was aligned and everything above was one
word late, so exactly one word was missing in between. Bisecting on jump
targets — a jump may not land on the `PUSHJ` of a `MOVEI 16,args / PUSHJ
17,routine` pair, because that would skip the argument setup — narrowed it to
`0407132`, in the tail of a loop:

```
407124  MOVE  2,117513        ; implied-DO output loop
407125  MOVEI 16,414426
407126  PUSHJ 17,423144       ; write one item
407127  AOS   2,117513
407130  AOSGE 0,116403
407131  JRST  407124
        <- gap
407132  JRST  403434
```

There are twenty-nine other implied-DO output loops in the segment. Twelve of
them end identically:

```
404032  AOS   2,117513
404033  AOSGE 0,113056
404034  JRST  404027
404035  PUSHJ 17,423145       ; FOROTS: end of I/O statement
404036  JRST  403434
```

The missing word is `PUSHJ 17,423145` — `0260740423145` — and restoring it
realigns the entire segment. That both files lost word 3674 is what makes the
diagnosis comfortable: one extraction glitch, applied twice.

Both repairs live in [`tools/mkimage.py`](../tools/mkimage.py), which refuses
to run if the files do not look the way it expects.

## 4. What the program is

FORTRAN-10, compiled and linked with FOROTS, the FORTRAN object time system —
its strings are unmistakable:

```
END OF EXECUTION
CPU TIME:
ELAPSED TIME:
Subscript range error on line
? Job aborted
```

The high segment holds FOROTS *and* the game. The compiler left every
subprogram's name in SIXBIT ahead of its entry point, so the program's own
structure reads out of the binary:

```
TOTING  AT      HERE    LIQ     LIQ2    HNHERE  ATRIB   FORCED
WZHERE  OUTDOR  BRIGHT  LIQLOC  GOAWAY  BANNED  NIGHT   LITE
DARK    OTEST   OFOUND  OWEAPN  OARMOR  OENEMY  OROT    OCARRY
...     PCT     MSG     SPEAK   GETIN   FIGHT   YES     CIAO
```

`TOTING`, `AT`, `HERE`, `DARK`, `PCT` and `FORCED` are the logical functions of
Crowther and Woods' ADVENTURE, carried over name for name. `TOTING` compiles to
exactly what you would expect:

```
400015  SETOM 121563          ; TOTING = .TRUE.
400016  MOVE  1,@0(16)        ; the argument
400017  MOVN  0,65323(1)      ; -PLACE(OBJ)
400020  CAIE  0,1             ; is PLACE(OBJ) = -1 ?
400021  SETZM 121563          ; no: TOTING = .FALSE.
400022  MOVE  0,121563
400023  POPJ  17,0
```

The game confirms the lineage itself, in its own introduction:

> This program is based on, and intended as a sequel to, the "ADVENTURE" Game,
> originally developed by Willie Crowther at Stanford, and the many existing
> sequels, such as "CAVE" by John Kopf, and "EXPLORE", by Michael Stimac, both
> at TYMSHARE.

And it will tell you how big it is, if you let it print its table report:

```
 23839 of  25999 words of messages
  3457 of   3599 travel options
   504 of    579 locations
   129 of    169 objects
   509 of    549 vocabulary words
```

## 5. The program's own state word

Location `0110467` is how the program knows what to do when it starts:

| value | meaning |
| --- | --- |
| `0` | tables not built — read `ARCHON.DAT` and build them |
| `1` | tables built; run the **full world reset**, then fall into 2 |
| `2` | greet the player and start playing — *no reset* |
| `3` | an exploration is in progress |
| `-1` | an exploration was SUSPENDed |

`archon.low` holds `2`, which is what the author's maintenance mode leaves
behind when it says *"OKAY.  YOU CAN SAVE THIS VERSION NOW."* — a copy built
from `ARCHON.DAT`, given its hours, and saved before anyone played it. The
port sets nothing and the game opens on its greeting. `blocke.low` holds
`-1` instead: a suspended session, which is why it is not the one to build
on. `--tables` winds the word back to `1` to see the program print its own
table-space report on the way in.

This is also why `ARCHON.DAT` is not needed. The tables it builds are already
in the core image; that is how the game was distributed.

## 6. The emulator

`src/cpu.c` implements the non-privileged PDP-10: the full instruction set the
program uses, single and double precision floating point in the native
excess-128 format, byte pointers, `BLT` (including the self-overwriting
"restore all accumulators" idiom, which is the one instruction whose exact
semantics the program depends on), and UUO dispatch.

`src/monitor.c` is the TOPS-10 side. FOROTS is a demanding guest and told us
what it wanted by aborting until it got it:

* **`DEVCHR`** — the left half must say the device is a terminal (`020000`),
  is available (`000040`) and can do both input and output (`000003`); the
  right half is a bit mask of the data modes the device supports, ASCII and
  ASCII-line here. Get the terminal bit wrong and FOROTS drives the TTY with
  buffered `IN`/`OUT` and never sees a typed line, because it reads terminals
  a character at a time with `TTCALL INCHWL` instead.
* **`DEVTYP`** (`CALLI 53`) — the low six bits are the device type; `12` octal
  is a terminal.
* **`DEVSIZ`** (`CALLI 101`) — returns the buffer size, which FOROTS stores and
  then uses, so the monitor's buffer rings have to be built to match it.

Also implemented: `TTCALL` in full, buffered device I/O with proper TOPS-10
buffer rings, `OPEN`/`INIT`/`LOOKUP`/`ENTER`/`IN`/`OUT`/`CLOSE`/`RELEAS`,
`GETSTS`/`SETSTS`/`STATO`/`STATZ`, and the CALLIs the program uses —
`RESET`, `CORE` (which grows `.JBREL`, without which FOROTS's first act is to
clear core to a garbage limit), `EXIT`, `DATE`, `MSTIME`, `RUNTIM`, `SLEEP`
(the game uses it to pace combat) and `APRENB`.

Anything unimplemented reports itself rather than failing quietly; `-v` turns
those reports on, `-vv` logs every monitor call, `-T` every instruction, and
`-w ADDR` watches a core location — which is how the DEVCHR bits and the
corrupted free list were found.

## 7. What is not there

* The from-scratch table build needs `ARCHON.DAT`, which was not on the tape.
  It is not needed to play; only that one path is unreachable.
* Play is deterministic. Nothing observed reseeds the random number generator
  from the clock — faking the date or the time of day changes nothing — so a
  fresh game opens identically every time and diverges only as your choices do.
* `ADJBP` and `EXTEND` are not implemented. The program never executes them;
  they will say so if it ever does.
