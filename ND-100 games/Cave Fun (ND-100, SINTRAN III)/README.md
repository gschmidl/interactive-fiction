# Cave Fun (ND-100, 1981)

CAVE-FUN, Mikael Johansson's adventure of February 1981 for a Swedish computer club's ND-100, played
by his table-driven interpreter ADVENTURE V4.2 in ND BASIC: a house in a forest, a road to a
mountain and, behind a locked iron door, three levels of an abandoned fortress - guard rooms, a maze,
a mine, a troll's bridge and a bear - with 27 treasures to bring back to the house. At the first
question type `CAVE`. Commands are a verb and a noun (`GET LAMP`, `ON LAMP`); `GET I` shows what you
carry and `GET S` the score. `SAVE` keeps a game beside the game file. `cavefun.exe` is an ND-100
with enough of SINTRAN III to run the compiled interpreter, which reads the game file as it did then;
the port's fixes to the interpreter and to two rules of the game are built in.

## Command line

`cavefun.exe [options]`

| Option | Effect |
| --- | --- |
| `-d DIR`, `--data DIR` | the game's files and saved games (default: `data` beside the program) |
| `--editor` | run Johansson's adventure editor instead of the game |
| `--sintran-files` | SINTRAN's rule for file names: a new file named in quotes, an old one without (the port takes either) |
| `--ascii` | show the 7-bit national characters as `[ \ ] { \| }` (default) |
| `--norwegian` | show them as Æ Ø Å æ ø å |
| `--swedish` | show them as Ä Ö Å ä ö å, and @ \` as É é |
| `-Z SECONDS`, `--clock SECONDS` | fix the clock at SECONDS since 1970, so a game repeats |
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

`cavefun.exe`, then `CAVE` at the first question.
