# Adventure 6.1/3 (Colossal Cave Adventure, 751 points)

*ADVENTURE < 6.1/ 3>* of 14 January 1982: the 751-point *Colossal Cave* that grew out of Don Woods'
350 points by way of David Long's 501-point version, with a safe behind the poster, matches, a cloth
bag and a castle. This copy was set up for Stanford's LOTS, where the opening hours were disabled.
No source survives: the original FORTRAN-10 program of 1984, its run-time and its encrypted databases
run on a built-in DECsystem-10 emulator with the TOPS-20 calls answered natively.

## Command line

`adv751.exe [options]`

| Option | Effect |
| --- | --- |
| `-p NAME`, `--player NAME` | play as NAME, for the greeting and the scoreboard (default: your Windows user name) |
| `--no-delays` | do not pause where the game pauses |
| `-e`, `--echo`, `--no-echo` | echo typed lines, or do not. By default they are echoed when input is a file and not at a console |
| `--time HH:MM` | pretend it is this time of day (it also seeds the dice) |
| `-v`, `--verbose` | trace operating-system calls to standard error (repeat for more) |
| `-t`, `--trace` | trace every instruction (diagnostics) |
| `-h`, `--help` | list the options |

`SUSPEND` writes a saved game to the current directory (`MYGAME.DAT` for the name MYGAME) and `RESUME`
reads it back.

## Recommended start

`adv751.exe`
