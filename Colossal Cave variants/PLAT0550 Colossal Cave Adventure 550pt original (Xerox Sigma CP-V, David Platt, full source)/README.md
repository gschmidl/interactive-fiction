# CP-V Adventure (David Platt, 550 points, 1979)

David Platt's original of the 550-point *Adventure*, "almost twice as big as before", written for
the Xerox Sigma under CP-V at Honeywell's Los Angeles Development Center and released on its tapes in
July 1979 with the patches of December 1979. It is Platt's ANS FORTRAN interpreter running the
database his translator compiles from his cave source, built for the Windows console.

## Command line

`adv.exe [options]` (`run.bat` adds `-u` and passes its parameters on)

| Option | Effect |
| --- | --- |
| `-u`, `--unlimited` | open outside the posted hours (on weekdays the cave is closed 9:00-11:30 and 13:30-17:00, offering a 30-move demonstration), no 600-move limit, and no 30-minute wait before `RESTORE` |
| `--time HH:MM[:SS[.mmm]]` | hold the clock at this time. The dice are drawn from the clock, so this makes a game repeat |
| `--date YYYY-MM-DD` | run as on this date |
| `--sense-switch N` | set CP-V sense switch N (1-6). The `WIZARD` command asks for switch 1, and a magic word |
| `--no-fixes` | accepted; the port changes nothing in the game |
| `-h`, `--help` | list the options |

`SAVE` writes to a `saves` folder beside the program; only the first four letters of the name count,
as on CP-V.

## Recommended start

`run.bat`
