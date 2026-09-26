# Dungeons — Data General AOS

"Rev. 4 of the Dungeon game", March 1980: a one-key dungeon crawl for the
Eclipse under AOS.  The original `DG.PR` and `DG.OL` from the *AOS Games 1
(1976–1983)* tape (identical copies are on *AOS Games 2*) run unmodified on
the Eclipse emulator in `src/`.

    dungeons          shows the four levels and asks which (what DG.CLI did)
    dungeons 3        straight into a level 3 dungeon

Levels: 1 Hacker, 2 Experienced, 3 Pro, 4 Suicidal.

## Playing

Commands are single keys, taken as you press them — no Enter.  The program
types the rest of the word itself ("A" → "You decide to attack").

| key | | key | |
|---|---|---|---|
| **H** | the command list | **T** | take the treasure here |
| **C** | your status and possessions | **G** *n* | give up object *n* |
| **L** | look around again | **B** | bargain with the creature |
| **R** | read a map (if one is on the wall, and you are bright enough) | **P** | panic — run somewhere |
| **M** *N/S/E/W* | move | **A** | attack |
| **Q** | quit | | |

Find the great Power Ring somewhere in the 10×15 maze and carry it out
through the dungeon door, alive.  Everything — the maze, your race, the
creatures and treasures, the random events — is new each game.  There is no
saving.  El Explorador de RPG has a thorough description of the rules:
<https://exploradorrpg.wordpress.com/juegos/dungeons/>.

Keys may be typed in either case.  Ctrl-C leaves.

## What it took

Dungeons is the program that needed a SIMH Eclipse set to C/150 instead of
S/140.  The reason is not the floating point unit but the **Commercial
Instruction Set** of the C-series machines: its FORTRAN runtime converts
between floating point and packed decimal with `LDI`, `STI` and the
commercial `FINT`, and its random number generator is built on them.  The
emulator implements those, plus the rest of what this program was the first
to use: Eclipse stack-overflow faults (the runtime initialises its memory
from the first one), `ELDB`/`ESTB`/`DSPA`, the character instructions
`CMV`/`CMP`/`CTR`/`CMT`, `DLSH`, and `FPSH`/`FPOP`.  The details and the
evidence for each are in `NOTES.md`.

## Files

- `dungeons.bat` — the launcher; its texts are DG.CLI's.
- `aosvs16.exe` — the emulator: `aosvs16 -d data data\DG.PR 3`.  `-Z` freezes
  the clock (the dungeon then comes out the same every time), `-v` logs system
  calls.
- `data/` — `DG.PR` and `DG.OL` exactly as on the tape, only ever read.
  `../src_original` also has `DG.CLI`.
- `src/` — emulator source (`make`); `make test` or `sh tests/run.sh` replays
  the recorded sessions in `tests/`.
- `tools/ovlimg.py` builds per-overlay images for `dis.exe`;
  `tools/drive.py` plays through a pipe and records the keys it used.
