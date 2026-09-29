# Legend v10.0 (ND-100, 1984-87)

LEGEND v10.0, Magnus Lundin's multi-player fantasy game in ND BASIC for a Swedish computer club,
"(C) A Hedström & M Lundin 84-87": two brotherhoods at war, the noble Orden and the bloodthirsty
Klanen. Create fighters, apprentices, magicians or priests, wander a village of a hundred rooms,
fight what you meet and rise through the ranks to Konung, in nine separate worlds still peopled by
the club's players of 1986-87. The game is in Swedish. `legend.exe` is an ND-100 with enough of
SINTRAN III to run the compiled program; the port's fixes (worlds that could not be loaded again,
two-word commands that never worked, and more) are built in. The worlds, players and letters are kept
in `data` beside the program.

## Command line

`legend.exe [options]`

| Option | Effect |
| --- | --- |
| `-d DIR`, `--data DIR` | the game's files (default: `data` beside the program) |
| `--swedish` | show the 7-bit national characters as Ä Ö Å ä ö å, and @ \` as É é (default) |
| `--norwegian` | show them as Æ Ø Å æ ø å |
| `--ascii` | show them as the 7-bit codes `[ \ ] { \| }` |
| `-Z SECONDS`, `--clock SECONDS` | fix the clock at SECONDS since 1970; the random numbers no longer stir in the time, so the same typing plays the same game |
| `--no-hold` | do not wait where the program pauses |
| `--vdu` | tell the program the terminal is a screen (VT100) |
| `--terminal N` | the terminal number the game shows (default 1) |
| `--raw` | pass the terminal bytes through untranslated |
| `--debug` | debug commands on a line starting with `#`: `#peek ADDR [COUNT]`, `#poke ADDR VALUE...`, `#find VALUE... [in FROM TO]`, `#dump FILE` (octal; decimal with a trailing `.`) |
| `--prog FILE` | run another one-bank program file instead |
| `-v`, `--verbose` | log monitor calls on standard error |
| `-T`, `--trace` | trace every instruction on standard error |
| `-h`, `--help` | list the options |
| `--version` | show the version |

## Recommended start

`legend.exe`
