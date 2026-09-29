# Getting HAUNT.EXE off the pack

## The package

The eXo collection ships Haunt as a ready-to-run Windows package: SIMH's
`pdp10` (KS10) simulator, two 315 MB RP07 packs, a 5 MB save state, and a
config that attaches the disks, restores the state and tells you to type
`r haunt`.
It already runs. The point of this port is a standalone native binary
with no 630 MB of disk image behind it.

The second pack (`dskc`) holds no Haunt content at all — it is TOPS-10
only. Everything below concerns `dskb`.

## Reading the pack

SIMH stores 36-bit PDP-10 words as 8 bytes, little-endian. That is why a
plain `strings` over the disk image finds nothing: ASCII is spread five
septets to a word and never lands contiguously.

The TOPS-10 HOME blocks confirmed both the packing and the geometry
immediately — `SIXBIT/HOM/` at byte 1024 and byte 10240, i.e. blocks 1
and 10 at 128 words per block, exactly where TOPS-10 puts them. Structure
names `DSKB`/`DSKC`, unit ids `DSKB0`/`DSKC0`.

## Finding the file

Searching the pack for the 36-bit word `SIXBIT/HAUNT /` (`504165566400`)
gave 12 hits on `dskb` and none on `dskc`, several of them followed by
`SIXBIT/EXE/` in the next word's left half.

Decoding the whole pack as packed 7-bit ASCII then located the game text
itself — `Copyright (C) 1979`, `max score is 440`, `John E` — in three
places, at blocks 151024, 152560 and 220615.

Scanning for `.EXE` page-map directory headers (left half `1776`) found
202 of them across the pack — every `.EXE` file on the disk — of which
exactly one, at **block 220351**, sits immediately before a copy of the
game text. The other two copies of the text have no `1776` header near
them and are not `.EXE` files.

TOPS-10's own `DIRECT` confirmed the target independently:

```
.direct sys:haunt.*
HAUNT   EXE  1432  <155>   10-Jul-99    121     DSKB:   [1,4]
```

`SYS:HAUNT.EXE` = `DSKB:[1,4]HAUNT.EXE`, **1432 blocks**.

## Why no filesystem parser was needed

TOPS-10 files are normally scattered, and reading one properly means
following the RIB's retrieval pointers. That work was started and turned
out to be unnecessary: this pack was built by restoring a BACKUP tape, so
files are laid out contiguously. Blocks 220351 … 220351+1431 are the
file.

A magtape dump from inside TOPS-10 was attempted as an independent check
and abandoned: `?BKPCOM Can't OPEN mag tape`. TOPS-10 fixes its device
tables when the monitor boots, and the save state we restore was taken
from a system booted with no tape controller, so attaching `TU` in SIMH
afterwards is invisible to the monitor. Cold-booting the pack would fix
it but buys nothing given the checks below.

## Why the extraction is known to be right

**1. The directory is self-consistent.** Word 0 is `1776,,17`; word 17 is
`1777,,1`, the end marker, exactly where a 17-word directory puts it.

**2. The page map accounts for every block.** Entries are pairs of
`<flags,,file page>` and `<count,,process page>`, where the page count is
the top 9 bits of the second word's left half, plus one:

| file pages | process pages | n | |
|---|---|---|---|
| 1–18 | 0–17 | 18 | |
| zero fill | 18 | 1 | |
| 19 | 19 | 1 | |
| zero fill | 20 | 1 | |
| 20–41 | 21–42 | 22 | |
| zero fill | 43 | 1 | |
| 42–252 | 44–254 | 211 | |
| 253–357 | 256–360 | 105 | flags `600000` — high segment |

File pages 1–357 are used contiguously. With page 0 for the directory
itself that is 358 pages of 4 blocks = **1432 blocks**, precisely the
size `DIRECT` reports, with nothing unaccounted for and nothing missing.

**3. The region is bracketed.** The word immediately before block 220351
and the word immediately after block 220351+1431 are identical
(`777653000041`) — a clean allocation boundary at both ends of exactly
the right length.

**4. It matches the running machine.** A SIMH save state was taken with
the game loaded and a few turns played under the original TOPS-10. Twenty
of twenty-four 512-byte chunks taken from across the assembled core
appear **verbatim** in that state. The four that do not are at process
pages 2, 16, 31 and 46 — all low-segment, holding JOBDAT, the stacks and
OPS4's working memory, which are precisely the pages a running game
rewrites. Had those matched, something would have been wrong.

**5. JOBDAT agrees with the page map.** Independently of any of the
above, the loaded image's own job data area says:

```
.JBSA   120 = 376777,,003230     start address 3230, first free 376777
.JBREL   44 = 000000,,376777     top of low segment
.JBHRL  115 = 151000,,550777     top of high segment
```

`376777` is 130559, exactly the last word of process page 254. `550777`
is 184831, exactly the last word of process page 360. Both segment limits
match the page map's own arithmetic, and the first word of the high
segment repeats `,,003230`, corroborating the entry point.

## Loading it

`tools/mkimage.py` applies the page map, flattens the two segments into
one address space, and emits the non-zero runs as `src/image.c` — 360
pages, 168,011 non-zero words in 85 runs, top word `547042`.

`cpu_reset()` needed no changes. Its two loader fixups — installing
`.JBREL` and copying `.JBS41` into location 41 — are no-ops here, because
a `.EXE` carries a correct JOBDAT of its own and both values already
match.

The image ran correctly on the first attempt.
