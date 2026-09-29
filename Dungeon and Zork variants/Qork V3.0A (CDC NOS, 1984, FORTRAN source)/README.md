# Qork V3.0A (CDC NOS, 1984)

S. O. Lidie's *Qork*, the DECUS *Dungeon* (Bob Supnik's FORTRAN *Zork*) for a Control Data Cyber
under NOS: "WELCOME TO QORK. THIS VERSION CREATED 84/05/30." It has Lidie's own ending instead of the
Dungeon endgame, 600 points. The original FTN5 program is compiled for the Windows console with its
own database. `SAVE` and `RESTORE` keep one game in a `saves` folder beside the program. The
implementers' debugger opens with `GUARDIAN` and the password `EXPLURIBUSONION TKMG`.

## Command line

`qork.exe [options]` (`run.bat` passes its parameters on)

| Option | Effect |
| --- | --- |
| `--seed N` | other dice. Without it every game rolls the same dice, as every game on the Cyber did |
| `--build` | set the database up again from its text file, `qork.txt`, as the site did (that file is not part of this package) |
| `--no-fixes` | accepted; it changes nothing in the game |
| `-h`, `--help` | list the options |

## Recommended start

`run.bat --seed N`, with a different number N for each game.
