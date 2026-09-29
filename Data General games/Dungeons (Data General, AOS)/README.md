# Dungeons (Data General AOS, 1980)

"Rev. 4 of the Dungeon game" (March 1980), a one-key dungeon crawl for the Data General Eclipse under
AOS: find the great Power Ring somewhere in a 10 x 15 maze and carry it out through the dungeon door
alive. The maze, your race, the creatures, the treasures and the events are new every game, and there
is no saving. Commands are single keys, taken as you press them (`H` lists them). The original
program runs unmodified on a built-in Eclipse emulator (`aosvs16.exe`).

## Command line

`dungeons.bat [LEVEL]`: the difficulty, 1 Hacker, 2 Experienced, 3 Pro or 4 Suicidal. Without it the
launcher lists the levels and asks, as the original start-up did. It runs
`aosvs16.exe -d data data\DG.PR LEVEL`.

The emulator's options:

`aosvs16.exe [options] PROGRAM.PR [ARGUMENTS]`

| Option | Effect |
| --- | --- |
| `-d DIR` | where the program and its data files are |
| `-s DIR` | where the program saves and restores (default: the current directory) |
| `-Z` | freeze the clock, so the dungeon comes out the same every time |
| `-L` | pass lower-case input through (by default it is folded to capitals) |
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

`dungeons.bat`
