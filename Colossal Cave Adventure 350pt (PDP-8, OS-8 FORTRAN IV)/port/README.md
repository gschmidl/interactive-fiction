# Adventure (350 points) — PDP-8 / OS-8 FORTRAN IV — native Windows port

`adventure.exe` is a single self-contained program. Double-click it, or run it
from a terminal, and you are playing.

Nothing about the game has been rewritten. The executable is a PDP-8/E: it
boots the original OS/8 pack that came with the distribution, the OS/8 monitor
starts the original FRTS FORTRAN run-time system, and FRTS loads and runs the
original `ADVENT.LD`. Every message, every parser decision and every scoring
rule is 1978 code executing as it always did.

## The game

This is the OS/8 version, recoded from Bob Supnik's RT-11 PDP-11 port of the
DEC-10 original. The game says who did it, in its own credits:

> This version, for the PDP-8, was done by Dick Murphy. It is
> based on a version for RT-11 done by Bob Supnik.

`ADVENT.FT` still carries the header of the RT-11 sources Murphy started from
— Kent Blackett, July 1977, then Bob Supnik through to 12-NOV-78. Its own
documentation (`ADVENT.DC`, in `../src_original/adventure-source/`) records
what the recoding cost:

> Because it is based on the RT-11 version of ADVENTURE, the following
> features of the FORTRAN-10 version are not supported:
> 1) MAGIC mode was removed
> 2) The SUSPEND and HOURS commands were deleted

So there is no wizard mode and no cave-hours restriction: the cave is always
open. Everything else is the full 350-point game.

The game logic is not FORTRAN. Only the outer skeleton is — `ADVENT.FT`,
`INITAD.FT`, `GETIN.FT` and a handful of small routines. The main program is
`AMAIN.RA`, 6,482 lines of RALF, the relocatable assembler for the FPP-8
floating-point processor, hand-translated from the FORTRAN "to fit into 32K"
and to run faster. There is no FPP-8 in this machine, so FRTS interprets that
code in software, exactly as it would on a PDP-8 without the option.

## Usage

```
adventure.exe [-s FILE] [-t FILE] [-n] [-r] [-C] [-D N]
```

| option | meaning |
| --- | --- |
| `-s FILE` | save file to use (default: `adventure.sav` beside the program) |
| `-t FILE` | also write a transcript of the session to `FILE` |
| `-n` | do not read or write a save file |
| `-r` | show the OS/8 boot dialogue instead of hiding it |
| `-C` | trace console traffic to stderr |
| `-D N` | trace the first N instructions to stderr |

`SAVE` and `RESTORE` work. The game writes its saved position into a file
called `ASAVE.DA` inside the OS/8 file system, so what actually changes is the
disk pack; the port remembers the blocks that were written and replays them
over the embedded image next time, which is what `adventure.sav` holds.

Ctrl-C quits. Typing `QUIT` and confirming ends the game and the program with
it.

## Text case

The reference emulator prints Adventure in block capitals. That is the
terminal, not the game. SIMH configures the PDP-8 console as a KSR-33
Teletype, which had no lower case, so it folds everything on the way out.

The message database has real capitalisation in it, encoded with shift
markers — `Y]OU ARE STANDING` in `ADVENT.TX` means `You are standing`. The
authors wrote it that way on purpose, for the terminals of 1978 that could
show it. This port prints what the program emits:

```
You are standing at the end of a road before a small brick building.
Around you is a forest.  A small stream flows out of the building and
down a gully.
```

Keyboard input is still folded up, because a KSR-33 keyboard could only send
upper case and the parser was written for that.

## Randomness

Adventure seeds its generator from a counter that FRTS spins in its idle loop
while waiting for the player to type — the "blinky lights" routine in
`SEED.RA`. The seed therefore depends on how long you took to answer the first
question, and two sessions on the real machine never matched either. The port
reproduces the mechanism rather than replacing it, so dwarf behaviour still
varies from game to game.

## Building

Needs a C compiler and zlib. On Windows the Strawberry Perl mingw-w64
toolchain has both.

```
./build.sh          # or build.bat
```

`tools/mkimage.py` produces `src/rk05image.h`: it takes the distribution pack,
zeroes every block that is not the OS/8 system area or one of the five files
Adventure opens (`FRTS.SV`, `ADVENT.LD`, `AINDX.DA`, `ATEXT.DA`, `ADVENT.IN`),
and deflates the result. 3.3 MB of RK05 becomes 91 KB of C.

## Testing

```
python tests/regress.py
```

Plays each `tests/*.txt` script twice — once on the reference SIMH PDP-8
booting the untouched distribution pack, once on `adventure.exe` — and diffs
the transcripts. Case is folded, for the reason above; nothing else is.

## What the emulator is

A PDP-8/E with 32K words, a KL8E console and one RK8E/RK05 drive. That is the
whole machine, and it is deliberately no larger. The reference configuration
was stripped one device at a time — EAE, line clock, paper tape, line printer,
DECtape, magtape, the extra terminal multiplexer — and the game played
identically each time, so none of them are here. An EAE in particular would
not have been more faithful: FRTS probes for one and would take a different
path through its floating-point interpreter than the reference takes.

The console is where the emulation has to think. OS/8's interrupt handler
reads the receiver on every interrupt to clear the flag, and has nowhere to
put the character until a program has a read pending, so characters offered
too early are simply gone — the reference machine loses fast type-ahead in
exactly the same way. The port therefore waits for a gap in the machine's own
printing before handing over the first character of each line, and then lets
the rest of the line follow at full speed. See the comments in `src/pdp8.c`.
