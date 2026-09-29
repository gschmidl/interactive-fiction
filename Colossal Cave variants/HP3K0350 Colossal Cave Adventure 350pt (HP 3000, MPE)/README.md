# Colossal Cave Adventure, 350 points (HP 3000)

Don Woods' 350-point game by way of Gary Palter's portable FORTRAN version, as the HP 3000 users'
library distributed it for MPE in 1981; who ported it is not known. Like Palter's version it keeps
the cave closed during business hours (`run.bat` lifts that) and lets a game be suspended. The
source is lost: the original MPE program, its data file and the FORTRAN/3000 run-time are built into
`Adventure.exe`, which emulates an HP 3000 Series III and the parts of MPE the program uses.

## Command line

`Adventure.exe [options]` (`run.bat` adds `-u` and passes its parameters on)

| Option | Effect |
| --- | --- |
| `-u` | ignore the cave's opening hours |
| `--about` | say what the program is |
| `--trace` | write an instruction trace to standard error |
| `-h`, `--help` | list the options |

## Recommended start

`run.bat`
