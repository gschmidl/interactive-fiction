# Adventure, 350 points (Prime 50-Series, PRIMOS)

Gary Palter's portable FORTRAN version of the 350-point Crowther and Woods game, as it was installed
on a Prime 50-Series under PRIMOS, from a 1985 games tape - wizard machinery, mixed-case text,
left-over debugging output and all. Native Windows console port; it starts from the Prime's own saved
game state, with its text, vocabulary, hours and magic word.

The installed game keeps hours: on weekdays from 09:00 to 17:00 the cave is closed to all but wizards,
and a suspended game may be restored only a minute later. `run.bat` lifts both. The wizard's password
is the magic word `DWARF` with its second and fourth letters replaced by the two-letter code of the day
(`SA`, `SU`, `MO`, ...): on a Saturday, `DSAAF`.

## Command line

`advent.exe [options]` (`run.bat` adds `-u` and passes its parameters on)

| Option | Effect |
| --- | --- |
| `-u`, `--unlimited` | no prime time (so no demonstration game either) and no wait before a suspended game is restored |
| `--time HHMM` | hold the game's clock at HH:MM |
| `--day N` | hold the game's date at N days after Saturday 1 January 1977 |
| `--seed N` | start the dice at N instead of from the clock, so a game repeats |
| `--auto` | accepted; does nothing in this build |
| `--no-fixes` | accepted; the port has no fixes to leave out |
| `-h`, `--help` | list the options |

## Recommended start

`run.bat`
