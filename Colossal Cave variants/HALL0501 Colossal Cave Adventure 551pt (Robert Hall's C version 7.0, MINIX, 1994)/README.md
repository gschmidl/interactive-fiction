# Generic Adventure 7.0 (Robert R. Hall, 1994)

Robert R. Hall's C *Adventure*, version 7.0 of July 1994, shipped with MINIX: David Long's and Doug
McDonald's 551-point game (McDonald's version 6.6, with an Infocom-style parser) put on top of Jerry
Pohl's C port of the 350-point original. Hall's scoring tops out at 501. Native Windows console
port.

## Command line

`advent.exe [options] [SAVEDGAME]` (`run.bat` passes its parameters on and runs the game in a
`saves` folder beside it, where `SAVE` writes `advent.sav`)

| Option | Effect |
| --- | --- |
| `SAVEDGAME` | start from a game saved with `SAVE` |
| `--no-fixes` | the 1994 program as it was. Otherwise four bugs are fixed: dwarves never block the way, `RESTORE` without a saved game ends the program, and `LEAVE` with nothing to leave or an object followed by `SAY` end the game with an internal error |
| `-h`, `--help` | list the options |

## Recommended start

`run.bat`
