# Dungeon V2.0 (Burroughs B7700, 1980)

The DECUS FORTRAN *Dungeon* - Bob Supnik's translation of MIT's *Zork* - as converted for a Burroughs
B7700: Chris Wilson's V1.2c code with Tom Fota's international V2.0 text of 1 December 1980. The
game's own newspaper calls it "a first, trial version of Dungeon on the B7700": the main game only,
500 points, no endgame, a simple parser. The B7700 source is compiled for the Windows console. Nothing
keeps a player out; the debugger opens with `GUARDIAN` and the password `CHRIS,QUETZAL,27`.

## Command line

`dungeon.exe [options]` (`run.bat` passes its parameters on)

| Option | Effect |
| --- | --- |
| `--saves=DIR` | where `SAVE` keeps the game (default: a `saves` folder beside the program) |
| `--fixed-clock` | the clock counts the lines typed, so a game repeats exactly (the dice are drawn from the clock) |
| `-h`, `--help` | list the options |

## Recommended start

`run.bat`
