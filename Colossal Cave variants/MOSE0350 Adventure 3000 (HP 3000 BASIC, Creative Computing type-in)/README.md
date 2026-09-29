# ADVENTURE/3000 (Benjamin Moser, 1979)

ADVENTURE/3000 version 3.2 (27 February 1979) by Benjamin Moser, a student at James Madison High
School in Vienna, Virginia: a 350-point *Colossal Cave* in HP 3000 BASIC, printed as a type-in in
*Creative Computing*, November 1979, with dumps of its four data files. The listing and data were
rebuilt from the magazine scans; `adventure3000.exe` is an HP 3000 BASIC interpreter that runs the
listing itself and builds the data files on first run. Several commands can go on one line,
separated by full stops (`take lamp. w. s.`).

## Command line

`adventure3000.exe [options]`

| Option | Effect |
| --- | --- |
| `-r` | seed the dice from the clock. Without it every game rolls the same dice |
| `--no-fixes` | play the listing exactly as printed. Otherwise the corrections are applied: place words (grate, bridge, building) no longer block verbs such as `unlock grate` or `cross bridge`, `take rock` no longer asks about the dwarf, drinking water no longer clears the oil, eating no longer empties the bottle, smashing the vase no longer removes the pillow, and four damaged text records are repaired |
| `-v VARIANT` | `fixed` (default) or `print`, the same as `--no-fixes` |
| `--bell` | ring the bell at every prompt, as the original did on an HP terminal |
| `--rebuild` | rebuild the data files from the text records before playing |
| `--build-data` | build the data files and stop |
| `-p DIR` | where the BASIC program is |
| `-d DIR` | where the data files are |
| `-u ID` | the account the program sees (default `B500`) |
| `-e` | echo input lines when input is not a terminal |
| `-q` | do not print `DONE` when a program stops |
| `PROG...` | run these BASIC programs instead of the game (a general BASIC/3000 interpreter) |
| `-h`, `--help` | list the options |

`HP3K_ANSI=1` or `0` in the environment forces the HP terminal's display enhancements on or off as
ANSI sequences (by default they are on at a console).

## Recommended start

`adventure3000.exe -r`
