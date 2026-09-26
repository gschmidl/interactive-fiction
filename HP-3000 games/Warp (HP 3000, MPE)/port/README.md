# Warp (HP 3000) -- Windows port

`Warp.exe` is the HP 3000 program running natively on Windows; it needs
nothing beside it.

    Warp.exe               play
    Warp.exe --about       what it is

It asks for your name, offers instructions, draws its welcome screen and then
assembles the world -- which takes a moment, because it really is reading the
game's data files while it prints the dots.

## What it actually is

This is not a rewrite. `WARP.PUB.GAMES` -- the Pascal/3000 program as it was
prepared on the machine -- is compiled into the executable together with its
data files `WARPADAT` and `WARPBDAT`, and with the Pascal/3000 runtime taken
out of `SL.PUB.SYS`. The .exe is an emulator of the HP 3000 Series III/58 that
loads the program the way MPE's loader does, executes it, and provides the MPE
intrinsics the runtime calls natively.

So the game you are playing is the original, and so is the library that reads
its files and formats its output: only the operating system underneath it is
new. The welcome text, incidentally, is stored enciphered in `WARPADAT` with a
different key per record; the program deciphers it as it reads.

## Fidelity

The transcript matches `doc/transcript_warp_probe.txt`, from the enciphered
welcome screen through the first room description and its object list.

One difference is known: the machine clears the screen (escape H, escape J)
before the second and third pages of the opening -- the creation date and the
welcome box -- and the port does not, so those pages scroll instead of
replacing what is above them. The first clear, at the name prompt, is there.
Nothing else in the text differs.

The reason is in the game rather than in the port. Warp decides whether to
clear by testing a byte of a local variable it never assigns, so what it reads
is whatever the last procedure left on the stack there; on the machine that
leftover happens to be even and here it is odd. What makes the leftover differ
is one step earlier still: the Pascal runtime opens `WARPADAT` for reading, the
machine refuses that open and the runtime opens it again for update instead,
while the port's file system grants the first one. Both paths read the same
file and print the same text.

## Files

    Warp.exe                 the game
    ../src_original/         the MPE program file and its data, as extracted
    ../doc/                  the reference transcript and the game's own docs
