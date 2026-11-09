# Adventure, 366 points (HP 9825, HPL)

"The Adventure Game -- Colossal CAVE 1/14/80": the Crowther and Woods game ported to the HP 9825
desktop computer and extended in 1979-80 by Eric Nelson and Craig Robinson of Hewlett-Packard's
Desktop Computer Division in Fort Collins, Colorado. It has 366 points: a few rooms reached by
digging with a shovel, an island in the reservoir reached by a boatman's boat, and one more
treasure, a golden ring set in a cave wall. The game is 26 HPL programs and their data files on
one floppy disk.

The Windows console port is the game's own HPL programs, translated to C, with the disk's
database built in. It is one 32-bit exe that also runs on Windows 7, and it matches the original
running in MAME line for line. The original gives a restored game the wrong exits until it moves to
another part of the cave; the port corrects that unless you pass `--no-fixes`. The original also
joins the lines of its text and refills them to the printer's width, which suited the 9825's
16-column strip printer. The port prints them as they were laid out, unless you pass `--reflow`.

## Command line

`advent.exe [options]` (`run.bat` passes its parameters on)

| Option | Effect |
| --- | --- |
| `--no-fixes` | play exactly as the 9825 program does: its restore bug, and the text refilled to the width |
| `-w WIDTH` | the line width, the game's "printer's width" (default 79) |
| `--reflow` | join the lines of the text and refill them to the width, as the 9825 does (always below width 71) |
| `--seed N` | seed the random numbers (default: the clock) |
| `--saves FILE` | where SUSPEND keeps the game (default `saves\ADVSAV` next to the exe) |
| `--echo`, `--no-echo` | print each command after reading it, as the 9825 does on its printer (the default when input is not a console) |
| `--no-pause` | do not wait for Enter when the game ends |

## Recommended start

`run.bat`

## What is here

- `game.zip`: `advent.exe`, `run.bat` and an empty `saves` folder.
- `reconstruction/`: the HPL listings, the decoded database, the disk image and the tools that
  read it, and the port with its MAME tests (`reconstruction/README.md`,
  `reconstruction/port/README.md`).
