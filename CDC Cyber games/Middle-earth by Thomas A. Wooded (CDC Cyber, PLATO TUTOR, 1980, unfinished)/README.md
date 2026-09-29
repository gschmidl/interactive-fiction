# Middle-earth (Thomas A. Wooded, PLATO, 1980, unfinished)

*A game of travel and adventure based on the works of J.R.R. Tolkien*, begun by Thomas A. Wooded in
TUTOR for PLATO at Florida State University in 1980, together with a second game, *Space Invasion*.
Neither was finished: of the characters only the wizard was written. `MiddleEarth.exe` runs the
original lesson, with its two character sets, on a built-in TUTOR interpreter and PLATO terminal. On
the title page, NEXT (Enter) chooses a character, LAB (F3) shows the map of Middle-earth and
Shift+F3 starts Space Invasion.

## Command line

`MiddleEarth.exe [options]`

| Option | Effect |
| --- | --- |
| `--name NAME` | the PLATO name to sign on as (default: your Windows user name) |
| `--course COURSE` | the PLATO course to sign on in (default `home`) |
| `--saves DIR` | where the saved state is kept (default: a `saves` folder beside the program) |
| `--seed N` | fixed random numbers (default: from the clock) |
| `--textlog` | print a transcript of the text on the screen on standard output |
| `--script FILE` | run without a window, taking the keys from FILE (for testing) |

## Recommended start

`MiddleEarth.exe`
