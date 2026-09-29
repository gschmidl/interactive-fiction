# Dave's Dungeon (DAVESCAVE, 1980)

"Dungeons and Dragons, written by Dave Parker", distributed by the MITRE Corporation, version 1.0 of
September 1980: a dungeon crawl behind a magic elven gate in the Misty Mountains, played with
two-letter commands (`LI` lists them). Your character, your map and your stocked dungeon are kept
between expeditions under your three initials, in a `saves` folder beside the program. The VAX
FORTRAN source is compiled for the Windows console.

## Command line

`davescave.exe [options]` (`run.bat` passes its parameters on)

| Option | Effect |
| --- | --- |
| `--time HHMMSS` | hold the clock still. The dice are seeded from it at the start and at every expedition, so a game repeats |
| `--no-fixes` | accepted; it changes nothing in the game |
| `-h`, `--help` | list the options |

## Recommended start

`run.bat`
