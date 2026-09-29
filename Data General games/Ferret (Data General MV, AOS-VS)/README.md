# Ferret (Data General AOS/VS, rev. 4.10)

"AOS/VS Ferret Rev 4.10": a text adventure written in PL/I in 1982 by programmers at Data General's
UK Systems Division, who stayed anonymous and kept extending it for decades. You wake in a small dark
room with no exits. Saves are `SAVE "name"` and `RESTORE "name"`, quotes included. The original
program runs unmodified on a built-in ECLIPSE MV emulator with an AOS/VS layer (`aosvs32.exe`).

## Command line

`aosvs32.exe [options] data\FERRET.PR`

| Option | Effect |
| --- | --- |
| `-s DIR` | where the game saves and restores (default: the current directory) |
| `-d DIR` | where the program's data files are (default: the program's own folder) |
| `-Z SECONDS` | freeze the clock at that many seconds since 1970, for repeatable sessions |
| `-v` | trace system calls and file access |
| `-h` | list the options |

Emulator diagnostics: `-t` instruction trace, `-F` floating-point trace, `-e HEX` entry point,
`-n COUNT` stop after COUNT instructions, `-D LO HI` dump that word range at the end (with `-v`),
`-W LO HI` report writes to that word range, `-P PC ADDR VAL` set a word each time the program
reaches PC (up to eight).

## Recommended start

`aosvs32.exe data\FERRET.PR`
