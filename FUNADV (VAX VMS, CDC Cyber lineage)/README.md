# FUNADV (VAX/VMS, from the CDC Cyber lineage)

Recovered from an OpenVMS VAX V7.1 disk image (`rq0-ra80.dsk`), where it
lived as `SYS$SYSROOT:[SYSMGR]FUNADV.EXE`.

## What this is

An extended Adventure variant. Its own banner says it best:

```
This is the first public release of FUNADV since it was ported
from the CDC Cyber Fortran.  Please report any bugs or problems
via mail to cts@dragon.com.
Good Luck & Have Fun!
                              - - -
Cave last modified:  February 24, 1992.
```

It is a 350-point game but not the standard cave: the vocabulary carries
MONGOOSE, POTION, RUNES, CHAIR, LISTINGS and MAIL, things can be worn, and it
matches ten letters per word "unlike adventure and newadv". It opens by asking
`Are you The Wizard ?`.

## How to play

    port\play.cmd

## How it works

`port\vaxvms.exe` is a VAX user-mode emulator with an OpenVMS shim. It loads the
original `FUNADV.EXE` plus the real `FORRTL`, `LIBRTL`, `MTHRTL` and `PASRTL`
shareable images, applies the image fixups, and runs the VAX code; only system
services (RMS, `$QIO`, and so on) are native. Nothing was rewritten.

`FUNADV.COMMON` is the game's fast-start dump of its FORTRAN COMMON blocks and
is what it actually reads at startup; `FUNADV.DAT` is the source database it was
built from. Both are byte-for-byte as they were on the VMS disk, still in RMS
variable-record format.

Verified against a transcript captured from the game running under simh on the
original disk image: identical output for the same inputs, including the
80-column padding.

## Terminal timing

The game's screen effects are written for a terminal on the far end of a
serial line, and they need the line's own delays to work.

Falling down the shaft (message 34) and the relief after `PISS` (message 210)
are sent one short record at a time, and are meant to arrive that way. Both of
them, and the lightning at the switch (message 211), flash the screen by
setting reverse screen mode — DECSCNM, `ESC [ ? 5 h` — and clearing it again a
few characters later:

    .<ESC>[?5h<ESC>[?5l
    -
    .<ESC>[?5h<ESC>[?5l

    <ESC>[?5h<ESC>[?5l<ESC>[7m-===RELIEF===-<ESC>[0m

     ->> <ESC>[?5h<ESC>[7mzap!    !!<ESC>[0m<ESC>[?5l <--   .... ... .. . Oh no!

The game asks for no delay of its own: it makes no timer call and burns no
cycles, so at 1200 baud the set and the clear were a fifth of a second apart
and at infinite speed they are not apart at all. Sent to a modern terminal the
fall, the pauses and every flash collapse into nothing, and only the `ESC [ 7 m`
inverse text survives — which is why `-===RELIEF===-` looked like it had always
just been inverse video.

So the emulator puts the line back. Terminal output is charged wire time at
`-baud` bits per second, ten bits to the character, and reverse screen mode is
held on the screen for at least `-flash` milliseconds however few characters
lie between the set and the clear.

| option | default | |
|---|---|---|
| `-baud n` | 9600 | line speed; `0` sends everything at once |
| `-flash n` | 120 | milliseconds a flash stays on screen; `0` for none |
| `-demo` | | play the three effects and exit, without starting the game |
| `-effects f` | effects.txt | rules for added flashes; `none` for the game as written |

`port\play.cmd -baud 2400` gives the longer pauses of a dial-up line;
`port\play.cmd -baud 0 -flash 0` is the behaviour before any of this.

Both are off when standard output is not a terminal, so captured transcripts
are unaffected — the timing changes when a byte arrives, never which byte. The
flash itself is the terminal's job: Windows Terminal and conhost both honour
DECSCNM, and the emulator turns on the console's escape-sequence processing at
startup so they get the chance to.

### Added effects

`port\effects.txt` layers extra flashes on top of the game's own. It is an
addition, kept in a file of its own so it is obvious and reversible — the game
data is untouched, and `-effects none` (or deleting the file) gives exactly
what FUNADV was written to show. One rule per line:

    <flashes>  before|after  <text the line has to contain>

It ships with one rule: three flashes after `--=splat=--`, so the landing at
the bottom of the shaft answers the two flashes on the way down.

## Contents

| path | what |
|---|---|
| `src_original/` | files as extracted from the VMS disk |
| `port/play.cmd` | run the game |
| `port/build.cmd` | rebuild the emulator (needs gcc) |
| `port/effects.txt` | extra flashes layered on the game; an addition, not the game |
| `port/engine/` | emulator source |
| `port/lib/` | the VMS shareable images the game links against |

## About the source

There is none on this disk. I checked every file header in `INDEXF.SYS` and
scanned all 63,870 unallocated blocks for FORTRAN source markers; the string
`FUNADV` appears only in the index file, the directory, and the game's own three
files. Hence the binary is run rather than recompiled.
