# XYZZY Adventure (Colossal Cave Adventure, 350 points, Data General AOS/VS)

The stock 350-point Crowther and Woods game as Data General shipped it for AOS/VS, calling itself
"XYZZY Adventure", taken from an MV/8000 disk dump. The original program is cold-started from its
own `.PR` file on a built-in 16-bit Data General Eclipse emulator that answers its AOS/VS system
calls; nothing is patched.

## Command line

`adventure.exe [options]`

| Option | Effect |
| --- | --- |
| `-s DIR` | where the game saves and restores (default: the current directory) |
| `-d DIR` | where `ADVENTURE.PR` and its files are (default: `data` beside the program) |
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

`adventure.exe`
