# Colossal Cave Adventure (HP 3000) — Windows port

`Adventure.exe` is the HP 3000 program running natively on Windows. Double-click
it or run it from a command prompt; it needs nothing beside it.

    Adventure.exe            play
    Adventure.exe --about    what it is
    Adventure.exe -u         ignore the cave's opening hours

## What it actually is

This is not a rewrite. `ADVENT.PUB.GAMES` — the FORTRAN/3000 program as it was
prepared on the machine, 378 records of MPE program file — is compiled into the
executable, together with its data file `ADVCOM` and the FORTRAN/3000 runtime
taken out of `SL.PUB.SYS`. The .exe is an emulator of the HP 3000 Series III/58
that loads the program the way MPE's loader does, executes it, and provides the
MPE intrinsics the runtime calls (`FOPEN`, `FREAD`, `FWRITE`, `FGETINFO`,
`CLOCK`, and so on) natively.

So the FORTRAN you are playing is the original, and so is the library that
formats its output: only the operating system underneath it is new.

## Where it comes from

`../doc/ADVENT_DOC_MAINLIB.txt` is the users' library abstract, dated 20
December 1981, for MPE II-A on the Series II. It names Don Woods as the
author, and "Willie Crowthers, PARC-MAXC and Gary Palter, MIT-MULTICS" as
the others, so this is Woods' game by way of Palter's portable FORTRAN
version. Like Palter's version it keeps the cave closed during business
hours (`-u` lifts that) and lets a game be suspended. Who ported it to the
HP 3000 is not known. The library also listed a source file, but the system
it was taken from does not have it.

It is not Benjamin Moser's *ADVENTURE/3000*, the other HP 3000 Adventure,
which is in BASIC
(`../../MOSE0350 Adventure 3000 (HP 3000 BASIC, Creative Computing type-in)/`).
For a few hours in September 2026 this folder carried Moser's ID, MOSE0350.

## Fidelity

The emulated instruction stream was compared against a trace of the real machine
(SIMH's HP 3000 simulator running MPE V/E G.40.00) taken while playing the same
game: 81,000 instructions of the program's own code match exactly, and the
transcript matches `doc/transcript_advent_ref.txt` line for line, trailing
blanks included.

## Files

    Adventure.exe            the game
    ../src_original/         the MPE program file and its data, as extracted
    ../doc/                  the reference transcript and the library's own docs
