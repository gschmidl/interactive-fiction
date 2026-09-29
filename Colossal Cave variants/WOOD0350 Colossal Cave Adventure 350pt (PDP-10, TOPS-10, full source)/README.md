# Colossal Cave Adventure, 350 points (Don Woods, TOPS-10)

Don Woods' complete 350-point *Colossal Cave Adventure* in DEC FORTRAN-10, in the DECUS conversion
Paul T. Robinson made at Wesleyan University in June 1980, with Woods' wizard, cave hours and
SUSPEND. The original TOPS-10 source compiled for the Windows console, keeping the PDP-10's packing
of five characters to a word; no line of game logic is changed.

## Command line

`advent350.exe [options] [SAVED-GAME]` (`run.bat` adds `-u` and passes its parameters on)

| Option | Effect |
| --- | --- |
| `SAVED-GAME` | reload a game left by `SUSPEND`, or a version saved by `MAGIC MODE`, as running a saved core image did on TOPS-10 |
| `-u` | lift the cave hours (on weekdays 08:00-17:59 the cave is closed to all but wizards, who may offer a 30-turn demonstration game), the demonstration game's turn limit and the wait before a suspended game may be resumed. It also answers `MAGIC MODE` for you, printing the magic word and the reply to the challenge. Scoring and the dice are untouched |
| `-s FILE` | where `SUSPEND` writes (default `ADVENT.SAV`) |
| `-d DD-MMM-YYYY` | freeze the date |
| `-t HHMM` | freeze the time |
| `-v` | show the database initialisation report |
| `-h`, `--help` | list the options |

`ADVENT_DATE`, `ADVENT_TIME` and `ADVENT_SAVE` in the environment do the same as `-d`, `-t` and `-s`;
`ADVENT_ECHO=0` or `1` turns the echo of typed lines off or on (by default they are echoed only when
input is not a terminal). `ADVENT.DAT` is read from the current directory or from beside the program.

## Recommended start

`run.bat`, and `run.bat ADVENT.SAV` to continue a suspended game.
