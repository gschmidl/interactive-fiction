# Caves 2 (HP 3000) -- Windows port

`Caves2.exe` is the HP 3000 program running natively on Windows. You build the
caves first, then explore them.

## What it actually is

This is not a rewrite. `CAVES2.PUB.GAMES` -- the BASIC/3000 program as it was prepared on the
machine -- is compiled into the executable, together with the BASIC/3000
runtime taken out of `SL.PUB.SYS`. The .exe is an emulator of the HP 3000
Series III/58 that loads the program the way MPE's loader does, executes it, and
provides the MPE intrinsics the runtime calls natively.

So the program you are playing is the original, and so is the library that
formats its output: only the operating system underneath it is new.

## Fidelity

The transcript matches `doc/transcript_caves2_probe.txt` exactly.

BASIC/3000's own runtime is the one part that is reimplemented rather than
loaded from `SL.PUB.SYS`: the library version stops during its own
initialisation, while the native one reproduces the game's output exactly.

## Files

    Caves2.exe               the game
    ../src_original/         the MPE program file, as extracted
    ../doc/                  the reference transcript
