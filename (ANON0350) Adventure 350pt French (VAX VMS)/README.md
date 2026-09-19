# Aventure — Adventure 350pt in French (VAX/VMS)

Recovered from an OpenVMS VAX V7.1 disk image (`rq0-ra80.dsk`), where it
lived as `SYS$SYSROOT:[SYSMGR]ADVENT.EXE`.

## What this is

DEC's **Adventure, Release 3** — the Crowther/Woods 350-point cave, ported to
FORTRAN IV by Kent Blackett and Bob Supnik (see `src_original/ADVENT.DOC`) —
**translated into French**. The executable was linked on 26-AUG-1985.

The French text lives in `ATEXT.DAT`; `AINDX.DAT` is the fast-start index built
from it. The scoring thresholds (35/100/130/200/250/300/330/349) are the
standard 350-point set.

```
Bienvenue dans Aventure!! Aimeriez-vous lire les instructions?
>non
Vous vous tenez a l'issue d'une route, devant un petit edifice en
briques. Autour de vous, il y a une foret. Un petit cours d'eau
s'ecoule depuis l'edifice et tombe dans un egout.
```

## How to play

    port\play.cmd

## How it works

This is **not** a rewrite. `port\vaxvms.exe` is a VAX user-mode emulator with an
OpenVMS shim: it loads the original `ADVENT.EXE` together with the real VMS
`FORRTL.EXE` and `LIBRTL.EXE` shareable images, applies the image activator's
fixups, and interprets VAX machine code. Only the system-service boundary is
native — RMS file and terminal I/O, `$QIO`, `$GETTIM`, `$FAO` and friends are
implemented against Windows.

The data files are byte-for-byte as they were on the VMS disk, still in RMS
record format (variable-length for `AINDX.DAT`, 74-byte fixed for `ATEXT.DAT`),
because the FORTRAN run-time reads them through RMS.

Verified against a transcript captured from the game running under simh on the
original disk image: identical output for the same inputs.

## Contents

| path | what |
|---|---|
| `src_original/` | files as extracted from the VMS disk |
| `port/play.cmd` | run the game |
| `port/build.cmd` | rebuild the emulator (needs gcc) |
| `port/engine/` | emulator source |
| `port/lib/` | the VMS shareable images the game links against |

## About the source

Only `ADVENT.FOR` (the main program, 3 KB) survived on this disk. Release 3 also
needs `AINIT`, `AMAIN`, `ASUB`, `AIOSUB`, `ASUSP` and `AMAC`, and none of them
are present — I scanned every file header in `INDEXF.SYS` and every unallocated
block on the volume and found no trace. That is why this runs the original
binary rather than being recompiled.
