# adv462

**Adventure 1.2 (462 points)**, the version of Colossal Cave Adventure that ran on Honeywell's Phoenix
Multics system in 1980.

This repository preserves the surviving listings of that game as transcribed artifacts, and makes the game
playable again on Multics and on Unix (with an OpenBSD port), following the pattern of
[adv550](https://github.com/lippard661/adv550).

## What this version is

- **Base:** Don Woods' 350-point Fortran Adventure (1977), after Will Crowther's original. Woods' data and
  code are recognizable throughout.
- **Engine:** Gary Palter's MIT-Multics Fortran port of Woods. Palter's hallmarks are all present:
  - the common blocks (`/msccom/`, `/ioscom/`, `/mtdcom/`);
  - five-character words packed with `code1`/`code2`;
  - the `scrmbl` vocabulary hash;
  - common-block save and restore through `svcomn`/`ldcomn`;
  - Palter named as a developer in the game's opening message.
- **Additions:** Dave Platt material related to his 550-point game: rooms 141–217, objects 65–98, messages
  202–270 and new vocabulary. The code implements it inside Palter's engine; it is not Platt's own code.
  - **Dating:** Platt's 550-point Adventure dates from 1979. He wrote it at Honeywell's Los Angeles Development
    Center for CP-V, then CP-6.
  - **"Platt 1984":** the only surviving source is his A-code database dated 18 September 1984 (preserved by
    Mike Arnautov and in the IF Archive). "Platt 1984" in these notes means that file, not a release date.
  - **What this copy shows:** the rooms here are often word for word the 1984 text, but differ from it in
    ways that look like an earlier stage of Platt's work:
    - the ice tunnels are "under construction";
    - there is no safe puzzle;
    - the wheat-stone bridge is permanent;
    - the fog rooms are separate numbered rooms;
    - there are rooms (the marble corridor, 143–147) that Platt 1984 doesn't have.
  - **Provenance:** how the material reached Multics is unknown. It may have come through contacts between
    Honeywell's Multics, CP-6 and GCOS people, and it may derive from Platt material earlier than his 1979
    release. So it can't be taken as a picture of Platt's game as it stood in 1980.
  - **The marble corridor and crystal medallion** are not in the 1984 source. They look more like earlier Platt
    material that was later cut than like Multics additions, though that isn't certain:
    - The corridor's rooms (143–147) and motion words (78 STAIRS, 79 GARDEN) are numbered inside the Platt
      block, before rooms and words that are in the 1984 source, rather than appended after it.
    - The medallion lies in the corridor's garden.
    - The corridor is entered by going north from Woods' Y2 (room 33), whose description was not changed to
      mention a north passage.
  - **The bulletin board** is the one Platt-range item that may be local. It displays the May 1980 news, and
    Platt 1984 has a NEWS command instead.
- **New verbs**, all implemented here in Palter's engine:
  - **From Platt:** FAST and FULL, STOP (as QUIT), and the magic words PHUGGG and MELENKURION. All are in the
    1984 source with the same meanings, so they probably came with the Platt material. The May 1980 news still
    announces FAST and FULL as new commands, since they were new to players of this game.
  - **From Palter:** SLAY (as KILL) is already in Palter's port as preserved in the SEL-32 copy (HORV0350), and
    it is also in Platt 1984.
  - **"." for the version** follows a Multics convention. The "." request of Multics subsystems (most notably
    those built with `ssu_`, the subsystem utility written by Gary Palter) identifies the subsystem and its
    version. It is not in Palter's portable Adventure as preserved in HORV0350, or in Platt 1984, so it was
    probably added on Multics in that tradition.
  - **Not in Platt 1984, possibly local:** LISTEN and TURNS.
- **Local changes:**
  - named SUSPEND/RESTORE;
  - a news message dated May 1980;
  - a maximum score of 462.
  Maintained by Jim Lippard (`jjl.sct`, `Lippard.Scouting@PCO-Multics`) as part of the Scouting project
  (Explorer Post 414) on the Phoenix Multics system. It ran from `>udd>MED>Kaiser>Lippard`, with storage
  quota courtesy of Garry Kaiser of Honeywell.
- **Source file name:** `adventure_.fortran`. On Multics, a trailing underscore marks a subroutine rather
  than a command. The Multics Fortran compiler of 1980 names a main program's entry point `main_`, so
  `adventure_` could not be typed as a command at all. It must have been run by a small `adventure` command
  calling `adventure_$main_`, probably written in PL/I along with the routines missing from the listing
  (`addr`, `size`, `ldcomn`, `svcomn`, `getime`). None of these survive; [`Multics/`](Multics/) has new ones, with the command named `adv462`.

No other copy of this version is known to exist.

## Status

| Step | State |
|---|---|
| 1. Artifacts: transcriptions and notes | done, see [`artifacts/`](artifacts/) |
| 2. Working Multics version: bugs fixed, TAKE/DROP ALL finished, mixed-case text supported and the database converted to mixed case, PL/I wrapper and missing routines supplied | done, see [`Multics/`](Multics/); compiles, binds and runs on MR12.8 (DPS8M) |
| 3. Unix / gfortran port and OpenBSD package, reading the same data file | done, see [`unix/`](unix/) and [`openbsd/`](openbsd/); play-tested on Linux, packaged on OpenBSD 7.9 (amd64) |

A signed OpenBSD package, [`openbsd/adv462-1.0.tgz`](openbsd/adv462-1.0.tgz), is built for OpenBSD 7.9 on amd64.
It can be verified with the signify public key
https://www.discord.org/lippard/software/discord.org-2026-pkg.pub

[`Multics/CHANGES.md`](Multics/CHANGES.md) lists every change from the 1980 listings. Among them, the working copy
finishes both of Jim's 1980 experiments on the listing: TAKE/DROP ALL, and packing text in base 90 instead of 64,
which lets the database be in mixed case.

### Porting to Unix

The port was feasible with little change, so it is a generated copy rather than a fork:

- `unix/mkunix.py` makes `unix/adv462.f` from the Multics source by replacing a few machine-dependent routines;
- a small C file supplies the save and restore routines;
- gfortran compiles the 1970s Fortran with `-std=legacy` and 64-bit integers, since the program packs five
  characters into each 36-bit word. They are now packed in base 90 rather than six bits apiece, which is what
  makes mixed-case text possible (see [`Multics/CHANGES.md`](Multics/CHANGES.md)).

Every fix is made once, in the Multics source, and both versions read the same `adventure.data`.

## Directory structure

```text
adv462/
├── README.md                 # This file
├── LICENSE                   # BSD license for Jim Lippard's contributions
├── artifacts/                # Transcriptions of the listings (1980; GCOS 1983), uncorrected
│   ├── README.md             # What each file is, and transcription conventions
│   ├── Multics/              # This game: adventure_.fortran, adventure.data, notes
│   │   └── transcription-source/   # Tab-separated source and render script for adventure.data
│   └── gcos/                 # Related artifact: the separate GCOS port of Woods 350
├── Multics/                  # The working game for Multics (the master copy)
│   ├── README.md             # How to build and run it
│   ├── CHANGES.md            # Every change from the listings
│   ├── adventure_.fortran    # The game, corrected
│   ├── adventure.data        # The database, corrected
│   ├── adv462.pl1            # The "adv462" command
│   ├── adv462_io_.pl1        # Save/restore and database attachment
│   ├── addr.pl1, size.pl1, getime.pl1   # Other site routines
│   ├── bound_adv462_.bind    # Bindfile
│   └── build.ec              # Builds and binds bound_adv462_
├── unix/                     # gfortran port
│   ├── mkunix.py             # Generates adv462.f from ../Multics/adventure_.fortran
│   ├── adv462.f              # Generated source
│   ├── adv462_util.c         # Site routines in C
│   ├── Makefile, adv462.6, README.md
└── openbsd/                  # OpenBSD port (Makefile, distinfo, pkg/DESCR, pkg/PLIST)
    └── adv462-1.0.tgz        # Signed package for OpenBSD 7.9 amd64
```

## Licensing and credits

The BSD license covers Jim Lippard's contributions: the transcriptions, notes, fixes and porting work.
Will Crowther and Don Woods retain whatever rights they hold in Adventure, Gary Palter in his Multics port,
and Dave Platt in his game content.

Transcription, analysis, fixes and porting by Jim Lippard, with assistance from Claude (Anthropic), September 2026.
