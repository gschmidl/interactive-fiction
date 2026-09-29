# ADVENTURE - FORTRAN FROM MSU (350 points, IBM MVS)

Don Woods' 350-point game by way of Gary Palter's portable FORTRAN version, as Doug Moore set it up
in IBM FORTRAN for MVS, from the CBT tape collection. The site added a shack and an outhouse. Native
Windows console port of the FORTRAN, with the game's initialization file prepared with no prime time
(MSU closed the cave to all but wizards from 07:00 to 17:00 on weekdays).

## Command line

`advent.exe [options]` (`run.bat` passes its parameters on)

| Option | Effect |
| --- | --- |
| `--date YYDDD` | pretend today is Julian date YYDDD (two-digit year, as the original's clock had) |
| `--time HHMM` | pretend the time is HH:MM. The clock seeds the dice, so `--date` with `--time` makes a game repeatable |
| `--no-fixes` | leave out the port's fix to the message-of-the-day chain, which otherwise hangs a game set up without a message |
| `-h`, `--help` | list the options |

## Recommended start

`run.bat`
