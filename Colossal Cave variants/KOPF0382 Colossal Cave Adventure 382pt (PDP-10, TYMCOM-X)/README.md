# Adventure, 382 points (Tymshare)

Don Woods' *Colossal Cave Adventure* as Tymshare ran it on its DECsystem-10s under TYMCOM-X, extended
in-house - probably by John Kopf - from 350 to 382 points with a 50-foot rope (and the verbs TIE,
UNTIE and CUT), a mithril mail coat and a ring of adamant. No source survives; the port runs the
original FORTRAN program on a built-in PDP-10 emulator with enough of the TOPS-10 monitor underneath
it. Three builds from Tymshare's tapes, which play identically:

| Program | Build |
| --- | --- |
| `advent-1979.exe` | the reference compilation (October 1979) |
| `advent-1981.exe` | the same program with two instructions patched by hand (February 1981) |
| `advent-1978.exe` | an earlier compilation (November 1978) |

## Command line

`advent-1979.exe [options]` (the same for all three; `run.bat` starts `advent-1979.exe` with `-u` and
passes its parameters on)

| Option | Effect |
| --- | --- |
| `-u`, `--unlimited` | lift the cave hours, the short expedition's turn limit and the 90-minute wait before a suspended game may be resumed |
| `-c`, `--continue` | resume the game saved by `SUSPEND` |
| `-f FILE` | save to and resume from FILE instead of `advent.core` |
| `-t HH:MM` | tell the game it is HH:MM |
| `-D YYYY-MM-DD`, `--date YYYY-MM-DD` | tell the game it is that day. With `-t`, the dice repeat |
| `-q`, `--no-delay` | skip the pauses the game asks for |
| `-e`, `--echo` | keep the program's own echo of what you type |
| `-h`, `--help` | list the options |
| `-v`, `-vv`, `-vvv` | report monitor calls, in increasing detail (diagnostics) |
| `-T` | trace every instruction (diagnostics; very slow) |
| `-w ADDR` | report every change to that octal core address (diagnostics) |

The game has no save file: type `SUSPEND` and the port writes the core image; `-c` picks it up.

## Recommended start

`run.bat` for a new game, `run.bat -c` to continue one.
