# Mystery Mansion (HP 3000) -- Windows port

`Mansion.exe` is the HP 3000 program running natively on Windows. It needs
nothing beside it; a saved game is written to the directory you run it from.

## What it actually is

This is not a rewrite. `MANSION.PUB.GAMES` -- the FORTRAN/3000 program as it was prepared on the
machine -- is compiled into the executable, together with the FORTRAN/3000
runtime taken out of `SL.PUB.SYS`. The .exe is an emulator of the HP 3000
Series III/58 that loads the program the way MPE's loader does, executes it, and
provides the MPE intrinsics the runtime calls natively.

So the program you are playing is the original, and so is the library that
formats its output: only the operating system underneath it is new.

## Fidelity

The opening text and the responses match `doc/transcript_mansion_probe.txt`
word for word. The one thing that differs is the mystery number on the banner
line, which the game draws from the clock: given the machine's own clock
(9 September, 13:00) the port picks the same mystery the machine picked, #2.

## Files

    Mansion.exe              the game
    ../src_original/         the MPE program file, as extracted
    ../doc/                  the reference transcript
