# Hobbits (Michael Jackson, PLATO, 1980)

Michael Jackson's class work for course CIS4932 at Florida State University, written in TUTOR for
PLATO in 1980: an index of assignments, the third of which is the riddle game - Gollum asks Bilbo
five of Tolkien's riddles from *The Hobbit*, with five hints each. The others are a sheet of drawing
commands and a random walk of boxes. `Hobbits.exe` runs the original lesson on a built-in TUTOR
interpreter and PLATO terminal.

## Command line

`Hobbits.exe [options]`

| Option | Effect |
| --- | --- |
| `--name NAME` | the PLATO name to sign on as (default: your Windows user name) |
| `--course COURSE` | the PLATO course to sign on in (default `home`). The lesson turns away students of course `cis4932` with a "Restricted File" screen |
| `--saves DIR` | where the saved state is kept (default: a `saves` folder beside the program) |
| `--seed N` | fixed random numbers (default: from the clock) |
| `--textlog` | print a transcript of the text on the screen on standard output |
| `--script FILE` | run without a window, taking the keys from FILE (for testing) |

## Recommended start

`Hobbits.exe`
