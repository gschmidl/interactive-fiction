# Dungeon V3.0a (HP 1000, 1982)

Dungeon V3.0a, "Initial version for HP 1000 by Tom Hutchinson" (Dome Petroleum, Calgary, 18 November
1982): the DECUS FORTRAN *Dungeon* - Bob Supnik's translation of MIT's *Zork* - by way of the
Burroughs version, adapted to RTE-IVB for the HP 1000 users' contributed library. The main game only,
500 points; when you have them all, a sign promises an endgame "soon to be constructed on this site".
The FTN4X source is compiled for the Windows console. The implementers' debugger opens with
`GUARDIAN` and the answer `CHRIS,QUETZAL,27`.

## Command line

`dungeon.exe [options]` (`run.bat` passes its parameters on)

| Option | Effect |
| --- | --- |
| `--time HH:MM[:SS[.mmm]]` | hold the clock at this time. The dice are seeded from the time of day, so this makes a game repeat |
| `--date YYYY-MM-DD` | run as on this date |
| `-u`, `--unlimited`, `--no-fixes` | accepted, and change nothing: the game has no hours, no move limit and the port no fixes |
| `-h`, `--help` | list the options |

`SAVE` asks for a file name and writes it to a `saves` folder beside the program; `RESTORE` reads it.

## Recommended start

`run.bat`
