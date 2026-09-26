# ZK: SPITBROOK Interactive Fiction (VAX/VMS, 1985)

Recovered from an OpenVMS VAX V7.1 disk image (`rq0-ra80.dsk`), where it
lived in `DUA0:[SYSMGR]` — the authors' own working directory, complete
with sources.

## What this is

An original interactive fiction game written at Digital's Spitbrook Road
facility in Nashua, NH, by **William Lees** and **Edmund Sullivan**, announced
29-AUG-1985. It is set inside Spitbrook itself. From their announcement
(`src_original/GAMES.TXT`):

> Announcing ZK, the first interactive fiction game written exclusively for the
> VAX! ZK is an sophisticated adventure game which takes place at Digital's
> Spitbrook software engineering facility. Meet famous Spitbrook personalities!

The disk holds the whole project, not just the game: **ZK's complete VAX Pascal
source**, and **IFC**, the authors' own interactive-fiction compiler
(`IFC$ROOM.PAS`, `IFC$OBJECT.PAS`, `IFC$LEX.PAS`, `IFC$RTL_SCREEN.PAS` …) with
the game written in its `.ROOM` / `.OBJECT` / `.MESSAGE` source languages.
As far as I can tell this is the only surviving copy of either.

## How to play

    port\play.cmd

ZK is screen-based, so run it in a console that understands ANSI/VT100 —
Windows Terminal is fine. It paints a status line with Score and Moves, a room
banner, and a scrolling text region, exactly as it did on a VT100 in 1985.

The keypad shortcuts from `ZK$KEY_DEF.COM` (PF1 gold key, KP7/KP8/KP9 for
NW/N/NE, and so on) are loaded but depend on your terminal sending real VT100
keypad sequences; typing the words always works.

```
                                       Score:         Moves:
Helicopter Pad
ZK: SPITBROOK Interactive Fiction
Copyright (c) 1985 by Digital Equipment Corporation.  All rights reserved.
You are playing version V1.0-823, which was built on 29-AUG-1985 at 01:00.

You really shouldn't be playing during prime time, but we'll make an
exception just this once...

It's a late Wednesday night. The sky is gray and overcast. You find
yourself in a helicopter preparing to land...
```

## How it works

Not a rewrite. `port\vaxvms.exe` is a VAX user-mode emulator with an OpenVMS
shim: it loads the original `ZK.EXE` together with the real VMS `PASRTL`,
`LIBRTL`, `SMGSHR` and `MTHRTL` shareable images, applies the image activator's
fixups, and interprets VAX machine code. Only the system-service boundary is
native — RMS, `$QIO`, `$CRMPSC`, `$GETDVI`, `$FAO`, `$ASCTIM` and friends are
implemented against Windows. SMG (VMS screen management) runs as real VAX code
and drives your console through the same escape sequences it sent to a VT100.

`TERMTABLE.EXE` is the VMS terminal capability table; SMG memory-maps it via
`$CRMPSC`, so it has to be here.

Verified against a transcript captured from the game running under simh on the
original disk image.

## Contents

| path | what |
|---|---|
| `src_original/` | the entire `DUA0:[SYSMGR]` directory: ZK sources, the IFC compiler, the game's `.ROOM`/`.OBJECT`/`.MESSAGE` data, build procedures, linker map |
| `port/play.cmd` | run the game |
| `port/build.cmd` | rebuild the emulator (needs gcc) |
| `port/engine/` | emulator source |
| `port/lib/` | the VMS shareable images ZK links against |
| `port/TERMTABLE.EXE` | VMS terminal capability table, needed by SMG |
| `port/ZK$KEY_DEF.COM` | keypad definitions, in raw RMS record format |

## Notes on the sources

`MAKE_ZK.COM` shows the build: nine Pascal modules (`ZK$MAIN`, `ZK$PARSE`,
`ZK$AST`, `ZK$OBJECT`, `ZK$ROUTINES`, `ZK$INIT`, `ZK$LEX`, `ZK$PARSE_OBJ`,
`ZK$ACTION`) plus four data modules compiled from the IFC languages, linked
against `IFC$RTL` and the V4.0 shareable images. The `[-.env]` environment
files it inherits (`TYPEDEF`, `RTLDEF`, `SMGDEF`, `SYSDEF`) are all present
here too, so the project is complete enough to study — and, with a VAX Pascal
compiler, to rebuild.

`ZK$VERSION_V.OPT` pins `ZK$T_VERSION` and the base level; the shipped image is
V1.0-823, built 29-AUG-1985 at 01:00. The game reads the weekday from the
current date, so the opening line follows the day you play it.
