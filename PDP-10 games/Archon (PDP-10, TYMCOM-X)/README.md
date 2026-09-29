# The Vale of ARCHON

*The Vale of ARCHON*, a large text adventure in FORTRAN-10 for the DECsystem-10, played on Tymshare's
TYMCOM-X: 504 locations on five parallel planes, 129 objects, a 509-word vocabulary, combat and some
24,000 words of prose. You start at the crossing of two roads among decayed mud buildings, with five
gold pieces and a man with the face of a weird hamster. `INFO`, `HELP`, `HITS` and `SCORE` explain the
rest. No source survives; the port runs the original program on a built-in PDP-10 emulator with
enough of the TOPS-10 monitor underneath it.

## Command line

`archon.exe [options]` (`run.bat` adds `-u -m` and passes its parameters on)

| Option | Effect |
| --- | --- |
| `-u`, `--unlimited` | lift ARCHON's hours, the turn limit ("THIS EXPLORATION HAS LASTED TOO LONG") and the 90-minute wait before a suspended game may be resumed |
| `-m`, `--fix-map` | stop wandering creatures straying between the Vale's five planes through two slips in the map, which rings the author's own "LEAKAGE" alarm. Your own travel is not affected |
| `-c`, `--continue` | resume the game saved by `SUSPEND` |
| `-f FILE` | save to and resume from FILE instead of `archon.core` |
| `-t HH:MM` | tell the game it is HH:MM |
| `-q`, `--no-delay` | skip the pauses the game asks for during combat |
| `--tables` | enter at the world reset, so the program prints its own table-space report first |
| `-h`, `--help` | list the options |
| `-v`, `-vv`, `-vvv` | report monitor calls, in increasing detail (diagnostics) |
| `-T` | trace every instruction (diagnostics; very slow) |
| `-w ADDR` | report every change to that octal core address (diagnostics) |

The game has no save file: type `SUSPEND`, answer yes, and the port writes the core image; `-c` picks
it up.

## Recommended start

`run.bat` for a new game, `run.bat -c` to continue one.
