# Crystal Cave

John Kopf's *Crystal Cave* (Tymshare, 1979): a 500-point game grown out of Crowther and Woods'
*Adventure*, written in DEC FORTRAN for the DECsystem-10 and played on Tymshare's TYMCOM-X. You start
at a barn at the end of a road above Crystal Cave park. No source survives; the port runs the original
program on a built-in PDP-10 emulator with enough of the TOPS-10 monitor underneath it.

Three builds from Tymshare's tapes, with the same text:

| Program | Build |
| --- | --- |
| `cave.exe` | 4-17 October 1979, the earliest, and the only one rebuilt exactly from its damaged tape copies |
| `cave-1983.exe` | 29 March 1983 |
| `cave-1984.exe` | 27 December 1984 |

## Command line

`cave.exe [options]` (the same for all three; `run.bat` starts `cave.exe` with `-u` and passes its
parameters on)

| Option | Effect |
| --- | --- |
| `-u`, `--unlimited` | lift the cave hours, the 30-turn limit of a short expedition and the 90-minute wait before a suspended game may be resumed |
| `-c`, `--continue` | resume the game saved by `SUSPEND` |
| `-f FILE` | save to and resume from FILE instead of `cave.core` |
| `-t HH:MM` | tell the game it is HH:MM |
| `-q`, `--no-delay` | skip the pauses the game asks for |
| `-e`, `--echo` | keep the program's own echo of what you type |
| `-h`, `--help` | list the options |
| `-v`, `-vv`, `-vvv` | report monitor calls, in increasing detail (diagnostics) |
| `-T` | trace every instruction (diagnostics; very slow) |
| `-w ADDR` | report every change to that octal core address (diagnostics) |

The game has no save file: type `SUSPEND` and the port writes the core image; `-c` picks it up.

## Recommended start

`run.bat` for a new game, `run.bat -c` to continue one.
