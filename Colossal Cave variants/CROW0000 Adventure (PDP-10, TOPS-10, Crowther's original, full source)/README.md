# ADV (Will Crowther's original Adventure)

The first *Adventure*: Will Crowther's FORTRAN program for the PDP-10, abandoned unfinished in early
1976 before Don Woods found it and made it the 350-point *Colossal Cave*. No score, no treasures to
bank, no save, no ending: a cave, a lamp, a bird, a snake and three dwarves with knives. Native
Windows build of the original TOPS-10 source; no line of game logic is changed.

Type commands in upper case: the program does not fold case, so `no` is not `NO`. There is no `QUIT`
(Ctrl-C leaves), and a death ends in FORTRAN's `PAUSE` dialogue, where `G` carries on.

## Command line

`adv.exe [options]`

| Option | Effect |
| --- | --- |
| `-2`, `--double-space` | print exactly what TOPS-10 printed: the blank lines the database's carriage control asks for and the trailing blanks of each word. By default the text is single-spaced |
| `-i`, `--init-pause` | read the database, then stop at `PAUSE INIT DONE` and wait for `G`, as `RUN ADV` did on the PDP-10 |
| `--echo`, `--no-echo` | echo typed lines, or do not. By default they are echoed only when input is not a terminal |
| `-V`, `--version` | say what this is |
| `-h`, `--help` | list the options |

`ADV_VERBATIM=1` in the environment does the same as `-2`; `ADV_ECHO=0` or `1` the same as the echo
options. `ADV.DAT` is read from the current directory or from beside the program.

## Recommended start

`adv.exe`
