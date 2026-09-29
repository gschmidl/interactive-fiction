# Exploration

Michael Stimac's *Exploration* (Tymshare, Cupertino): a large 900-point game built on Crowther and
Woods' *Adventure* and John Kopf's *Crystal Cave*, written in DEC FORTRAN for the DECsystem-10 and
played on Tymshare's TYMCOM-X. You start in the small town of Plovertown and work your way to Mystic
Cave. No source survives; the port runs the original program on a built-in PDP-10 emulator with
enough of the TOPS-10 monitor underneath it.

## Command line

`explor.exe [options]` (`run.bat` adds `-u` and passes its parameters on)

| Option | Effect |
| --- | --- |
| `-u`, `--unlimited` | lift the cave hours (shut 9-12 and 13-17 on weekdays), the 900-turn limit and the 45-minute wait before a suspended game may be resumed |
| `-c`, `--continue` | resume the game saved by `SUSPEND` |
| `-f FILE` | save to and resume from FILE instead of `explor.core` |
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
