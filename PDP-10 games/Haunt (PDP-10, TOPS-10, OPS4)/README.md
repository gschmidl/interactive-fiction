# HAUNT 4.6 (John E. Laird, 1982)

*HAUNT*, version 4.6 of 21 June 1982, by John E. Laird at Carnegie-Mellon University: a 440-point
haunted-house adventure written in OPS4, a rule-based production-system language, for the
DECsystem-10 (not the later, unfinished OPS5 rewrite). You start at a bus stop with a fistful of
tokens; waiting is the opening move. No source survives; the port runs the original program on a
built-in PDP-10 emulator with enough of the TOPS-10 monitor underneath it.

Haunt never had a save command. The port adds one outside the game: `#save [FILE]` at the prompt
writes a snapshot of the machine, `-c` resumes it, and `#help` lists these commands.

## Command line

`haunt.exe [options]`

| Option | Effect |
| --- | --- |
| `-c`, `--continue` | resume the snapshot written by `#save` |
| `-f FILE` | use FILE instead of `haunt.core` |
| `-t HH:MM` | tell the game it is HH:MM |
| `-q`, `--no-delay` | skip the pauses the game asks for |
| `-e`, `--echo` | keep the program's own echo of what you type |
| `-h`, `--help` | list the options |
| `-v`, `-vv`, `-vvv` | report monitor calls, in increasing detail (diagnostics) |
| `-T` | trace every instruction (diagnostics; very slow) |
| `-w ADDR` | report every change to that octal core address (diagnostics) |

## Recommended start

`haunt.exe`, and `haunt.exe -c` to continue from a `#save`.
