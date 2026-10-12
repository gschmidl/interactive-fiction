# Colossal Cave Adventure, 500 points (Data General AOS)

Crowther and Woods' *Adventure* with 150 more points, from Data General's *AOS Games 2* tape
(1977-84): the whole 350-point cave plus a monstrous spider and its web, a coral passage, a "South
Seas" beach, the "Marquis De Sade Memorial Maze", a torture chamber and eight new treasures. The
original FORTRAN 5 program runs unmodified on a built-in Data General Eclipse emulator that answers
its AOS system calls. The cave hours are already off in the data ("open all day, every day").

## Command line

`adventure.exe [options] [NAME]`

| Option | Effect |
| --- | --- |
| `NAME` | resume the game suspended as NAME (`save`, `suspend` or `pause` asks for the name). Resuming deletes the file, as on the DG |
| `-s DIR` | where suspended games are written and looked for (default: the current directory) |
| `-d DIR` | where `ADVENTURE.PR` and its files are (default: `data` beside the program) |
| `-L` | pass lower-case input through (by default it is folded to capitals, which is all the game understands) |
| `-Z` | freeze the clock, and with it the dice, for repeatable transcripts |
| `-v` | log every system call, overlay load and file transfer |
| `-g` | enable the `#` debug commands (`#help` lists them) |
| `-h` | list the options |

Emulator diagnostics: `-t` instruction trace, `-F` floating-point trace, `-b HEX` breakpoint (up to
eight), `-B` stop at a breakpoint, `-w LO HI` report writes to a hex address range, `-n COUNT` stop
after COUNT instructions, `-W FILE` write the memory image at the end, `-k` carry on past an
unimplemented system call, `-S` treat `JSR @15` as a system call, `-e HEX` entry point, `-f FILE`
the program file, `-D FILE` start from a memory dump (`-z HEX` its page-zero copy), `-V FILE FWORD
BASE WORDS` preload an overlay, `-O N` force overlay N, `-a AC0 AC1 AC2 AC3` initial accumulators,
`-c` start with Carry set.

## Recommended start

`adventure.exe` for a new game, `adventure.exe NAME` to resume one.
