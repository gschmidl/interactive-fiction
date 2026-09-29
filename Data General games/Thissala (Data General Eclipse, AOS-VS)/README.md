# Thissala (Data General AOS/VS, 1985)

*Thissala*, a 1985 text adventure by David Auerbach, Paul Chiasson and Peter Macaulay of Data
General's Corporate Consulting Group; this is revision 0.60, the AOS/VS version. The original
program and its overlays run unmodified on a built-in 16-bit Eclipse emulator that answers the
AOS/VS system calls. `HELP` prints the authors' notes; saves are `SAVE` and `RECALL`, and `RECALL`
with the name `DRAW` loads the authors' own game from the tape, 110 moves in.

The game's `HOOK` verb cannot be typed, so the passage behind the Library's curtains is unreachable
as shipped; with `-g`, typing `ENHOOK` in the Library hooks the curtains as the verb would.

## Command line

`thissala.exe [options]`

| Option | Effect |
| --- | --- |
| `-d DIR` | where the game's files are (default: `data` beside the program, or the current directory) |
| `-s DIR` | where `SAVE` writes and `RECALL` looks (default: the current directory) |
| `-g` | debug: `ENHOOK`, and emulator commands on a line starting with `#`: `#room`, `#goto N`, `#where OBJ`, `#move OBJ N`, `#bring OBJ`, `#state OBJ [N]`, `#peek ADDR [WORDS]`, `#poke ADDR VALUE`, `#regs`, `#dump FILE`, `#set room\|obj ADDR`, `#help`. It also opens the authors' own `ASSIST`/`EXPRESS`/`MTBL` verbs, which the current build refuses again |
| `-v` | log every system call, overlay load and file transfer |
| `-h` | list the options |

Emulator diagnostics: `-t` instruction trace, `-F` floating-point trace, `-b HEX` breakpoint (up to
eight), `-B` stop at a breakpoint, `-w LO HI` report writes to a hex address range, `-n COUNT` stop
after COUNT instructions, `-W FILE` write the memory image at the end, `-k` carry on past an
unimplemented system call, `-S` treat `JSR @15` as a system call, `-e HEX` entry point, `-f FILE`
the program file, `-D FILE` start from a memory dump (`-z HEX` its page-zero copy), `-V FILE FWORD
BASE WORDS` preload an overlay, `-O N` force overlay N, `-a AC0 AC1 AC2 AC3` initial accumulators,
`-c` start with Carry set.

## Recommended start

`thissala.exe -g`
