# Adventure 385 points (HP 1000 / RTE) — Windows port

A working Windows port of the HP 1000 **385-point Adventure**: the version
Beasley adapted at Haystack Observatory in 1978-79 for HP 1000 series
minicomputers and submitted to the HP1000 International Users Group's
contributed software library (CSL/1000), running here from the RTE-6/VM
build made in September 2025.

Run `build\adven.exe` from the `build\` folder. It needs `#ADVZZ` (the game
database) beside it; `build.bat` copies it there.

## Building

```
build.bat
```

Needs a MinGW-w64 gfortran (developed against GCC 15.2) — the same toolchain
as the Univac ports, no WSL2 needed. `build.bat` points at the compiler by
absolute path (Strawberry Perl's bundled gfortran by default); edit the
`GFORTRAN=` line if yours lives elsewhere.

On its first run the game reads `#ADVZZ` and writes `#ADVXX`, a
random-access copy of every message keyed by record number. That is what it
prints `NOW LOADING SECTION n.` for, and it does it on every run — see
*Fast-start cache*, below.

## Verifying

```
regress.bat
```

replays two recorded walkthroughs through the port and diffs the result
against transcripts captured from the *original* `ADVEN.RUN` executing on an
emulated HP 1000 (SIMH `hp2100`, booted off the RTE-6/VM disc image in
`..\src_original\`). Both currently come out **identical**, line for line —
130 and 115 lines of game output covering the surface, the grate, the bird
chamber, the Hall of Mists, the gold nugget, XYZZY and PLUGH, scoring and
the quit sequence.

To recapture the reference transcripts, start the simulator in a console
window of its own (SIMH refuses to run when its command console is a pipe)
using `tests\simh-console.sim`, which sets `console telnet=2326`, then:

```
python tools\simdrv.py 2326 tests\walk1.sim tests\walk1.simh.out
```

## Where the source came from

The two `HP-2100` folders in the eXo collection's *Colossal Cave Adventure
(1976)* package ship
a SIMH `hp2100` simulator, a 50 MB HP 7920 disc image, and a saved machine
state that drops you at the game's prompt. The disc image turned out to hold
the **complete FORTRAN and assembly source**, not just the built program, in
an RTE-6/VM CI directory `/ADVEN`.

`..\src_original\rte6vm_extract.py` is the file-system reader written to get
it out; `..\src_original\disc\` is everything it recovered, and
`..\src_original\disc_listing.txt` is the full volume listing. The layout it
decodes is documented at the top of that script — briefly: the image stores
16-bit words byte-swapped, the FMP logical unit starts at image sector
0xC4E0, blocks are 128 words, the root directory is at block 0x24, and
record-structured files frame each record with a leading and trailing length
word whose top bit flags an odd character count.

The recovered release is:

```
ADV01.FTN   outer block: all the named COMMON, plus VOCAB, BUG and RAND
ADV03.FTN   segment ADV1, the initialisation pass over the database
ADVX2.FTN   segment ADV2, the first half of MAIN
ADVY2.FTN   segment ADV3, the second half of MAIN
ADV05.FTN   data-structure routines (MOVE, CARRY, DROP, JUGGL, PUT, ...)
ADVF4.FTN   SPEK, PSPEK, RSPEK, GETIN, YES, YESX
ADVG4.FTN   the HP "Q-string" package (ARW, 1976-77)
ADV14/24/34/44/54/64.ASM   assembly string primitives
ADV15.ASM   RANDM;  IOF/IRP/LINKRET/NARG/SUSP.ASM   RTE interfaces
#ADVZZ      the game database
COMPILE.CMD, LINKIT.CMD, MERGEIT.CIN, LADVEN, ADVEN.MAP, SUBMIT.TXT
```

`SUBMIT.TXT` is the CSL/1000 submission form, with Beasley's note of 14
February 1979 describing the HP 1000 adaptation. Above that the source
carries the lineage: Crowther and Woods on the DECsystem-10, FORTRAN IV-PLUS
under IAS on the PDP-11/70, then Kent Blackett (Digital, 15-JUL-77) and Bob
Supnik (21-OCT-77) for FORTRAN IV under RT-11. A further set of edits marked
`DWH` dates from September 2025, when the RTE-6/VM build was made.

385 is not a constant in the program: `MXSCOR` is added up at run time from
the treasures and bonuses the database defines.

### Two files were damaged on the disc

`ADV03.FTN` and `ADVX2.FTN` had their tails overwritten by later files —
`MERGEIT.CIN` sits inside `ADV03.FTN`'s extent, and a stale relocatable
overwrote `ADVX2.FTN`'s last 34 blocks. The disc carries three generations
of the `/ADVEN` directory; the two damaged files were reassembled from the
surviving part of the September 7th copy plus the corresponding tail of the
September 6th `.GUT` copy, spliced at the last record the two agree on.
The results are `..\src_original\disc\*.FTN.recovered`, and the differences
between the generations in the surviving regions are only commented-out
debug lines and a couple of statement labels moved onto `CONTINUE`s.

## What was changed

`src\*.f` are **generated** from `..\src_original\disc\` by
`tools\convert.py`; rerun it any time. It applies two stages.

`tools\hp2gfortran.py` handles the mechanical HP FTN4 dialect differences:

- compiler-control lines (`FTN4`, `FTN5`) and `END$` become comments;
- HP defaults INTEGER and LOGICAL to 16 bits, so `IMPLICIT INTEGER (A-Z)`
  becomes `IMPLICIT INTEGER*2 (A-Z)` and bare `INTEGER`/`LOGICAL` gain the
  `*2`. That width is not cosmetic here: the Q-string package and the `A2`
  edit descriptors pack two characters per word, and the `EQUIVALENCE`s
  between the buffers depend on it;
- Hollerith constants become packed `INTEGER*2` literals, two characters per
  word in memory order — which is what the `EQUIVALENCE`d `CHARACTER`
  variables and `A2` descriptors around them expect on a little-endian host;
- octal literals become decimal, `MAX0`/`IABS`/`FLOAT` and friends become
  the generics, and the game's own `RAND` is renamed `RNDM` so it does not
  collide with gfortran's `RAND` intrinsic;
- a trailing `_` in a FORMAT — HP's "no newline" marker, used for the `>`
  prompt and for phrases the code continues with an object name — becomes
  gfortran's `$` edit descriptor.

Only columns 7 onwards are treated as statement text, which matters more
than it sounds: a continuation marker in column 6 in front of a name reads
beautifully as a Hollerith count, and `     2HINTLC,CHLOC,...` in the middle
of a `COMMON` list is not a two-character constant.

The `FIXUPS` table in `tools\convert.py` then applies the edits that needed
judgement, each commented in place:

- **Segments.** RTE runs `ADV1`, `ADV2` and `ADV3` as disc-resident segments
  of one program, loaded by `SEGLD` and sharing the outer block's COMMON.
  Here they are ordinary subroutines and `SEGLD` becomes a `CALL`; `SEGRT`
  becomes a no-op that the following `RETURN` completes.
- **Memory I/O.** HP FTN4 arms the next `READ` for memory with `CALL CODE`
  and then reads from an INTEGER array. In standard Fortran the unit being
  a `CHARACTER` variable says the same thing, so each buffer gains a
  `CHARACTER` overlay and `CALL CODE` becomes a comment.
- **`A` editing into INTEGER destinations.** This one is a trap. gfortran
  reads an `A` field into a non-`CHARACTER` item the way it reads a numeric
  field: a comma ends the field and the rest is blank filled. Every message
  in the database goes through `FORMAT(I5,1X,40A2)`, so location 3 came out
  as "a building  a well house" with its comma eaten. The seven such reads
  now call `A2GET`, which does the plain character copy the HP compiler
  does. The number always sits in columns 1-5 with its separator in column
  6, because the `CLRQ`/`IPOSQ`/`NSRTQ` loop just above each read shifts the
  record until that is true.
- **`.AND.` / `.OR.` as bitwise operators** on integers, in `RAND`'s seeding
  and in the `BITST` statement function, become `IOR` and `IAND`.
- **Statement functions.** gfortran type-checks their arguments strictly and
  its integer literals are 32-bit, while everything here is 16-bit. The
  dummies that only ever receive literals are declared 32-bit, and `BITST`'s
  and `PCT`'s are renamed so they stop colliding with the real variables
  `L` and `N`.
- **Terminal case.** The HP console runs in upper case — SIMH's
  `set TTY0 UC` — so the game never folds what it reads, and every
  vocabulary entry is upper case. A pipe does not fold, so `GETIN` now does.
- **End of file.** An RTE terminal never reports one; a redirected standard
  input does, so `GETIN` gets an `END=` branch and prints `[end of input]`.
- **`LCO` debug block.** Nine addresses taken in `ADV2` for `WRITE`s that
  are themselves commented out. `LCO` has no portable meaning and nothing
  reads the results, so the assignments are commented out too.
- **Duplicate routines.** `ADV05.FTN` also defines `VOCAB`, `BUG` and
  `RAND`. `ADVEN.MAP` shows RTE's loader satisfying those three from
  `ADV01` and taking only the data-structure routines out of `ADV05`, so the
  three copies in `adv05.f` are commented out.

`src\hprte.f` is new code, not translated. It replaces three things that
only exist on the machine:

- **The Q-string package.** A Q-string is an INTEGER array whose first word
  is a character count, with the characters running two per word from the
  second word on. `CLRQ`, `DLETQ`, `NSRTQ`, `MOVEQ`, `IPOSQ`, `MOVQQ`,
  `PUTQ` and `JASCQ` are reimplemented from the documented semantics in
  `ADVG4.FTN` and the headers of `ADV14/24/34/44/54.ASM` — the originals are
  1976 HP code built on `EXEC` calls, `REIO` and byte-move instructions.
  Every one of them indexes characters in memory order, which is also what
  gfortran's `A2` uses, so the string package and the formatted I/O around
  it agree on any host.
- **The FMP file interface.** `#ADVZZ` is read as an ordinary text file, one
  line per record. `#ADVXX` — which the game creates and then reads at
  random through `FmpSetPosition` — is a direct-access file of 88-byte
  records, matching the `:::2:344:44` type-2, 44-word-record file the
  original asks RTE for. `FMPREAD` transfers only as many characters as the
  caller asks for, as FMP does; several calls in `ADV03` ask for `44/2`
  or `44` where the buffer holds 88, which looks like a slip in the 2025
  mechanical conversion of `READF` to `FmpRead`. Reading the full record
  instead makes no difference to either recorded walkthrough, so the port
  keeps FMP's own semantics.
- **The executive calls and FORLIB** — `RMPAR` (which reports the terminal
  LU, here unit 6), `EXEC` code 11 for the clock, `SUSP`, and `RANDM`. The
  HP generator's exact stream cannot be reproduced off the machine, and the
  game only ever seeds it from the clock, so `RANDM` is an ordinary 32-bit
  linear congruential generator.

Nothing in the game logic itself was touched.

### Fast-start cache

`ADV1` can skip the whole database pass by reading `#ADVYY`, a raw image of
the COMMON area written out with a length taken off the RTE load map. That
depends on the linker laying every COMMON block out contiguously in one
particular order, which the source itself warns about at length — the 2025
notes in `ADV03.FTN` describe finding FMP's own buffers sitting *inside* the
program's COMMON on RTE-6/VM. gfortran gives no such guarantee, so the port
drops the cache and reads `#ADVZZ` every time. That costs milliseconds here
and is what a cold start on the real machine did anyway; the only visible
difference is that the `NOW LOADING SECTION n.` lines print on every run,
where the shipped disc image has a warm cache and prints none.

## Notes on the game

- 385 points, and a plain 1977-vintage Adventure otherwise: no `SAVE`, no
  hint menu, no maintenance mode. `SUSPEND` suspends the RTE program; the
  port prompts for RETURN instead.
- Input is folded to upper case, and output is upper case in places
  (scoring, some error messages) because the 1978 code was written for an
  upper-case-only terminal.
- The game writes `#ADVXX` into the current directory, so run it from
  `build\` — or from wherever `#ADVZZ` lives.
