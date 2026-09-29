# Zork (Data General AOS/VS, 1984)

"An AOS/VS rev of ZORK as of 7/16/84 by PZ": the *Dungeon* version of *Zork*, endgame included, for
Data General's ECLIPSE MV computers under AOS/VS. Saves are `SAVE "name"` and `RESTORE "name"`,
quotes included. The original program runs unmodified on a built-in ECLIPSE MV emulator with an
AOS/VS layer (`aosvs32.exe`).

## Command line

`aosvs32.exe [options] data\ZORK.PR`

| Option | Effect |
| --- | --- |
| `-s DIR` | where the game saves and restores (default: the current directory) |
| `-d DIR` | where the program's data files are (default: the program's own folder) |
| `-Z SECONDS` | freeze the clock at that many seconds since 1970; the dice are seeded from it, so a session repeats |
| `-v` | trace system calls and file access |
| `-h` | list the options |

Emulator diagnostics: `-t` instruction trace, `-F` floating-point trace, `-e HEX` entry point,
`-n COUNT` stop after COUNT instructions, `-D LO HI` dump that word range at the end (with `-v`),
`-W LO HI` report writes to that word range, `-P PC ADDR VAL` set a word each time the program
reaches PC (up to eight).

## Recommended start

`aosvs32.exe data\ZORK.PR`
