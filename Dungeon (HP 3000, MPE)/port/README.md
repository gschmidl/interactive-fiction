# Dungeon (HP 3000) -- Windows port

`Dungeon.exe` is the HP 3000 program running natively on Windows; it needs
nothing beside it.

    Dungeon.exe            play
    Dungeon.exe --about    what it is
    Dungeon.exe -u         ignore the dungeon's opening hours

It waits about a second at start-up: the game reads the clock, calls MPE's
PAUSE, and reads the clock again to check that time is passing, so the wait is
the program's own.

## What it actually is

This is not a rewrite. `DUNGEON.PUB.GAMES` -- the FORTRAN/3000 program as it was prepared on the
machine -- is compiled into the executable together with its two data files `DINDX` and `DTEXT`, together with the FORTRAN/3000
runtime taken out of `SL.PUB.SYS`. The .exe is an emulator of the HP 3000
Series III/58 that loads the program the way MPE's loader does, executes it, and
provides the MPE intrinsics the runtime calls natively.

So the program you are playing is the original, and so is the library that
formats its output: only the operating system underneath it is new.

## Fidelity

The transcript matches `doc/transcript_dungeon_ref.txt` byte for byte, the
score line included. The emulated instruction stream was also compared against
a trace of the real machine (SIMH's HP 3000 simulator running MPE V/E G.40.00)
taken while typing the same command: 28,900 instructions of the program's and
the library's own code match, apart from the random number generator, which is
seeded from the clock and so starts from a different number in every session.

## Files

    Dungeon.exe              the game
    ../src_original/         the MPE program file, as extracted
    ../doc/                  the reference transcript
