# Adventure (ENB, ND-100, 1983)

ADVENTURE-ENB, a Swedish princess quest in ND BASIC by someone who signed their files ENB, from a
Swedish computer club's floppy of 1983. The land is 85 by 85 squares and new every game - fields,
forests, towns, castles, dragons, thirty roaming monsters - and somewhere a cave maze where the
princess waits behind two locked doors; bring her to a castle. Buy arms in a town first, and rest
(`V`) often. Keys are single letters at `ORDER:`. `advenb.exe` is an ND-100 with enough of SINTRAN
III to run the compiled program; the port's fixes (the lost instruction files that hung the game, a
princess no one could win, crashes at the start) are built into the program.

## Command line

`advenb.exe [options]`

| Option | Effect |
| --- | --- |
| `-d DIR`, `--data DIR` | where the program is (default: `data` beside the program) |
| `--swedish` | show the 7-bit national characters as Ä Ö Å ä ö å, and @ \` as É é (default) |
| `--norwegian` | show them as Æ Ø Å æ ø å |
| `--ascii` | show them as the 7-bit codes `[ \ ] { \| }` |
| `--uptime UNITS` | start the machine's uptime at UNITS (1/50 s) and run it with the instructions alone: one land for each value, the same every time |
| `-Z SECONDS`, `--clock SECONDS` | fix the clock at SECONDS since 1970; the uptime stays 0, so every game is the same land |
| `--no-hold` | do not wait where the program pauses |
| `--vdu` | tell the program the terminal is a screen (VT100) |
| `--terminal N` | the terminal's logical device number (default 1) |
| `--raw` | pass the terminal bytes through untranslated |
| `--debug` | debug commands on a line starting with `#`: `#peek ADDR [COUNT]`, `#poke ADDR VALUE...`, `#find VALUE... [in FROM TO]`, `#dump FILE` (octal; decimal with a trailing `.`) |
| `--prog FILE` | run another one-bank program file instead |
| `-v`, `--verbose` | log monitor calls on standard error |
| `-T`, `--trace` | trace every instruction on standard error |
| `-h`, `--help` | list the options |
| `--version` | show the version |

## Recommended start

`advenb.exe`
