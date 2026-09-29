# Quest (Gary Stollman, PLATO, 1980, unfinished)

A student's *Adventure*-style game, written by Gary Stollman in TUTOR for PLATO at Florida State
University in 1980, in the class work space of course CIS4932. He never finished it. `Quest.exe` runs
the original lesson on a built-in TUTOR interpreter and PLATO terminal. Every command is judged "no",
so press Enter once to clear it before typing the next, as the lesson was written.

## Command line

`Quest.exe [options]`

| Option | Effect |
| --- | --- |
| `--name NAME` | the PLATO name to sign on as (default: your Windows user name) |
| `--course COURSE` | the PLATO course to sign on in (default `home`) |
| `--saves DIR` | where the saved state is kept (default: a `saves` folder beside the program) |
| `--seed N` | fixed random numbers (default: from the clock) |
| `--textlog` | print a transcript of the text on the screen on standard output |
| `--script FILE` | run without a window, taking the keys from FILE (for testing) |

## Recommended start

`Quest.exe`
