# Dungeon (HP 3000)

The FORTRAN *Dungeon* - Bob Supnik's translation of MIT's *Zork* - as it was installed on an HP 3000
under MPE, keeping opening hours outside which the dungeon is closed (`run.bat` lifts them). The
source is lost: the original MPE program, its two data files and the FORTRAN/3000 run-time are built
into `Dungeon.exe`, which emulates an HP 3000 Series III and the parts of MPE the program uses. It
waits about a second at the start: the game checks that time is passing.

## Command line

`Dungeon.exe [options]` (`run.bat` adds `-u` and passes its parameters on)

| Option | Effect |
| --- | --- |
| `-u` | ignore the dungeon's opening hours |
| `--about` | say what the program is |
| `--trace` | write an instruction trace to standard error |
| `-h`, `--help` | list the options |

## Recommended start

`run.bat`
