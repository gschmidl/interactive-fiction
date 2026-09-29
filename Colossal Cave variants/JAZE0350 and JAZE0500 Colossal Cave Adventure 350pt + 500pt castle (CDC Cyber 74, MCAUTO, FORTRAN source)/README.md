# Adventure (MCAUTO Cyber 74): BEG, 350 points, and ADV, 500 points

Kent Blackett's FORTRAN *Adventure*, with Don Woods' wizard and prime-time machinery, as Tony
Jarrett and Paul Zemlin put it on McDonnell Douglas Automation's CDC Cyber 74 in December 1978. The
Black Wizard of the High East Tower asks which cave you want: BEG, the standard 350-point cave, or
ADV, 500 points, with an Egyptian room, a castle north-east of the forest and the Black Wizard in its
dungeon. Native Windows console port of the CDC FORTRAN source.

## Command line

`advent.exe [options]` (`run.bat` adds `-u` and passes its parameters on)

| Option | Effect |
| --- | --- |
| `-u`, `--unlimited` | never prime time: a full game at any hour (weekdays 08.00-17.59 were reserved for wizards, with a short game for everyone else) and no 90-minute wait before resuming a suspended game |
| `--day N` | hold the day of the year still (1-366) |
| `--time HHMM` | hold the time of day still. The dice are seeded from the clock, so `--day` with `--time` makes a game repeatable |
| `--no-fixes` | accepted, but there is nothing to turn off: none of the port's changes alters the game |
| `-h`, `--help` | list the options |

The magic word for wizard mode is `DWARF`.

## Recommended start

`run.bat`
