# Getting EXPLOR off the tape

Notes on decoding `explor.sav` and making it run. Everything here was
derived from the files themselves; nothing was borrowed from another
game.

## The files

Eight copies of EXPLOR survive in the Tymshare tape collection
(`tym-original.7z`):

| path | bytes | words | date |
|---|---|---|---|
| `tapes/carl/explor.sav.1` | 202,800 | 40,560 | 1983-09-18 |
| `tapes/carl/explor.sav.4` | 202,800 | 40,560 | 1983-09-18 |
| `tapes/games/explor.sav` | 201,595 | 40,319 | 1983-03-29 |
| `tapes/upl/explor.sav.17` | 201,595 | 40,319 | 1981-07-15 |
| `tapes/upl/explor.sav` | 158,015 | 31,603 | 1980-08-04 |
| `tapes/upl/explor.sav.6` | 158,015 | 31,603 | 1980-08-04 |
| `tapes/upl/explor.sav.20` | 26,295 | 5,259 | 1981-07-16 |
| `tapes/oper/explor` | 5,682 | — | 1972-08-22 |

The port is built from the 1983-03-29 and 1981-07-15 copies, for reasons
that become clear below. The 1980 pair is an earlier, smaller build; the
1983-09-18 pair is later but damaged.

## Word format

Every file length is a multiple of 5, which is the first clue: a 36-bit
PDP-10 word is stored in five bytes, each holding one 7-bit septet
right-justified, with the word's spare 36th bit in the high bit of byte 4.
This is the "5 septets per word" ANSI-ASCII tape mode.

Two independent things confirm it. Only byte 4 of each group of five ever
has its high bit set — exactly what a 7,7,7,7,7-plus-one-spare-bit layout
predicts. And decoding on that basis turns

```
28 2f 27 20 4f  4b 41 59 2c 20  22 27 2c 32 30  41 31 29 20 20
```

into `(/' OKAY, "',20A1)  `, a FORTRAN FORMAT statement, five characters
to the word.

## File format

`.SAV` is TOPS-10's blocked load format: a chain of IOWD headers
`<-count,,addr-1>`, each followed by `count` data words, ending with a
`JRST` to the start address and one trailing pad word.

Walking that chain on either 1983-03-29 or 1981-07-15 fails. Block 8 —
the big one, loading at `000642` — has a header claiming 6962 data words,
but only 6961 follow it: at the position where word 6962 should be, the
*next* block's IOWD is already sitting. The chain loses sync there and
never reaches a terminator. **Each copy is one word short, and short
inside block 8.**

## The repair

Compare the two copies word by word and the damage localises itself.
Outside a single window they are identical. Inside it — file words
1850..3673, which is block 8 addresses `003616..007255` — every word of
the 1983 copy equals the *next* word of the 1981 copy, all 1823 of them.

That is the signature of one dropped word on each side, at the two ends
of the window:

| copy | lost | at address |
|---|---|---|
| `games/explor.sav` | `200100026110` `MOVE 2,26110` | `003616` |
| `upl/explor.sav.17` | `474100000000` `SETO 2,0` | `007256` |

Splice each file's missing word back in and both reconstructions give the
same 40,320-word sequence, which then parses cleanly: **473 blocks,
39,845 words loaded into `000120..124213`, landing exactly on the
`JRST 016314` terminator with one pad word to spare**, and block 8 finally
holding the 6962 words its header always claimed.

Both restored words are also the right *kind* of word for where they go.
`MOVE 2,26110` sits directly above a `MOVEM 2,17425` — an ordinary
assignment. `SETO 2,0` is the middle instruction of F40's standard
logical-assignment idiom, `CAMx / TDZA ac,ac / SETO ac, / MOVEM`, which is
otherwise missing a limb.

### Checking the splice went in at the right place

A one-word error anywhere in block 8 still yields a file of the right
length, so the length check alone proves nothing about *position*. Two
things pin it down.

**FORMAT literals.** F40 compiles a FORMAT into a literal preceded by a
`JRST` that jumps over it, and loads the literal's address with
`MOVEI 1,<the JRST>` followed by an `017` LUUO. In the repaired image all
37 of those loads point at a `JRST`. In the unrepaired 1983 copy the four
formats below `003616` still do, but the fifteen above it point one word
further on, at the literal text itself — because the text is sitting one
word low. `tools/mkimage.py` enforces this check on every build.

**The constant pool** at the end of block 8. Repaired, it reads 8, 2, 1,
100, `EXPLOR    `, and those constants are referenced 3, 12, 96, 2 and 1
times respectively — the constant 1 being the most-used constant in the
program, as you would expect. Unrepaired, everything shifts by one and
the program appears to reference the constant 100 ninety-six times and
never to reference 8 at all.

Getting this wrong is not subtle in play, incidentally: with the pool
shifted by one, answering `YES` to "DO YOU NEED INSTRUCTIONS?" prints
message 100, "THE BUGLE EMITS AN UGLY BLARE AS YOU BLOW IT."

### The third copy

`tapes/carl/explor.sav.1` and `.4` are a later build (1983-09-18) and are
byte-identical to each other but for one pad word. They cannot be used:
two regions are zero-filled, 4095 words at `000124..010122` and 1024 words
at `012123..014122`, wiping out the entry path. They were still useful as
an independent witness — the layout of the end of block 8, `8/2/1/144`
then `EXPLOR    `, matches the repaired image exactly.

## Making it run

Three things stood between a correct image and a running game.

**Location 41.** The program's first instruction is an `015` LUUO. F40
calls its object time system through LUUOs (opcodes 001-037), which trap
through location 41 — and nothing in the image ever stores there, so the
first library call went into empty core. The answer is in JOBDAT: word
`000122` is `.JBS41`, where SAVE stashes the program's LUUO trap
instruction because location 41 itself is monitor territory that GET/RUN
rewrites. It holds `JSR 114436`, and `114437` is a routine that picks the
LUUO out of location 40 and dispatches on its opcode field. `cpu_reset`
puts it back, the way the loader would. The rest of JOBDAT corroborates
the reading: `000120` is `.JBSA` (`124214,,016314` — first free, start
address), `000121` is `.JBFF` (`124214`), and `000133` is `.JBCOR`, whose
left half `124213` is exactly the top of the image.

**KI-10 arithmetic.** With the trap installed, the runtime got as far as
printing `?KI-10 CODE WILL NOT RUN ON A KA-10` and exiting. Its test is

```
114453: SETO  0,          ; AC0 = -1
114454: AOBJN 0,.+1
114455: JUMPE 0,ok
```

`AOBJP`/`AOBJN` add one to each half of the AC. Carry the right half into
the left, as a KA-10 does, and -1 becomes `000001,,000000`; keep the
halves independent, as a KI-10 does, and it becomes zero. The emulator now
does the latter. It makes no difference to an ordinary `AOBJN` loop, where
the right half is an address that never wraps.

**Buffered terminal I/O.** The runtime drives the terminal with buffered
`INPUT`/`OUTPUT`, not `TTCALL`, and it is fussy about three things:

* `DEVCHR` must report `DVIN` (`200000`). At `120716` the runtime tests
  `TLNN 5,200020` before any `WRITE` that follows a `READ` on the same
  unit; without `DVIN` or `DVMTA` it decides the unit is a file it would
  have to reposition, prints `WARNING! FORMATTED READ FOLLOWED BY WRITE
  MAY FAIL.` and abandons the transfer. For a program whose whole life is
  read-a-command / write-a-reply, that means it never gets a command.

* `OPEN` must not build buffer rings. The runtime asks `DEVSIZ` for a
  size and lays out its own ring, temporarily pointing `.JBFF` at its
  storage and then issuing `INBUF`/`OUTBUF`. Allocating at `OPEN` time
  took core the runtime then handed out again, so the two rings
  overlapped. Allocation is now lazy, and only if the header is empty.

* The left half of each buffer's link word must hold the buffer's length
  (every word but the link itself). At `120731` the runtime reads it as
  the word count for the `BLT` that copies a line between buffers; left
  at zero, the `BLT` gets a backwards range and walks the address space.

After that the game runs: 400 random moves without a fault, including
dying and being re-incarnated, and `SUSPEND` / `explor -c` round-trips
through a saved core image with the author's 45-minute wait intact.

## Memory map of the running program

| range | contents |
|---|---|
| `000120..000137` | JOBDAT |
| `000140..016323` | main program, its FORMAT literals and constant pool |
| `016325..~077777` | game data: vocabulary, room text, message corpus |
| `~100000..107777` | game subroutines (F40, `JSA 16` linkage) |
| `110000..124213` | the F40 object time system (MACRO-10, `PUSHJ 17`) |

Messages are indexed through a table at `026070`: entry *n* holds an
offset, and the text begins at `026710` plus that offset. Entry 65 is the
welcome banner, entry 1 the instructions. That two-level layout is what
made the constant-pool misalignment so visible.
