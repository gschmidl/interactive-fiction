# Caves (HP 3000) -- Windows port

`Caves1.exe` is the HP 3000 program running natively on Windows. Answer its
questions with 1 or 0 and steer by cavern number.

## What it actually is

This is not a rewrite. `CAVES1.PUB.GAMES` -- the BASIC/3000 program as it was prepared on the
machine -- is compiled into the executable, together with the BASIC/3000
runtime taken out of `SL.PUB.SYS`. The .exe is an emulator of the HP 3000
Series III/58 that loads the program the way MPE's loader does, executes it, and
provides the MPE intrinsics the runtime calls natively.

So the program you are playing is the original, and so is the library that
formats its output: only the operating system underneath it is new.

## Fidelity

The transcript matches `doc/transcript_caves1_probe.txt` exactly, including the
blank lines and the six-character numeric columns BASIC/3000 prints.

BASIC/3000's own runtime is the one part that is reimplemented rather than
loaded from `SL.PUB.SYS`: the library version stops during its own
initialisation, while the native one reproduces the game's output exactly.

## Files

    Caves1.exe               the game
    ../src_original/         the MPE program file, as extracted
    ../doc/                  the reference transcript
