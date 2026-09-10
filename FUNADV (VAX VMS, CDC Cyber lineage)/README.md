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

## Contents

| path | what |
|---|---|
| `src_original/` | files as extracted from the VMS disk |
| `port/play.cmd` | run the game |
| `port/build.cmd` | rebuild the emulator (needs gcc) |
| `port/engine/` | emulator source |
| `port/lib/` | the VMS shareable images the game links against |

## About the source

There is none on this disk. I checked every file header in `INDEXF.SYS` and
scanned all 63,870 unallocated blocks for FORTRAN source markers; the string
`FUNADV` appears only in the index file, the directory, and the game's own three
files. Hence the binary is run rather than recompiled.
