# Mordor: Land of evil (ND-100, 1985)

*Mordor: Land of evil*, v7.52 of 11 February 1985, Mikael Johansson's ND-Pascal game for a Swedish
computer club: it is 12 April 3019 of the Third Age, you have the Ring, and with your heroes of the
West you must carry it through a Middle Earth of 128 by 128 squares to Mount Doom, past orcs, trolls,
Balrogs, the Nazgul and Gollum, while the citadels fall one by one. The source was recovered from the
free pages of a floppy and compiled with the club's own compiler; `mordor.exe` is an ND-100 with
enough of SINTRAN III to run it. The port's fixes (names that had to be typed shifted, a fight that
could go on for ever) are built in. The game refused to start between 08 and 16; `run.bat` starts
it at any hour.

## Command line

`mordor.exe [options]` (`run.bat` adds `--unlimited` and passes its parameters on)

| Option | Effect |
| --- | --- |
| `-u`, `--unlimited` | play at any hour: types DEL ahead of the program, as players did between 08 and 16 when it refused to start |
| `--new-map` | start with an empty map file, so the game builds a new Middle Earth (it keeps the one it made the first time) |
| `--no-rubout` | leave the delete keys to the program instead of rubbing characters out (the game cannot rub out itself) |
| `-d DIR`, `--data DIR` | the game's files (default: `data` beside the program) |
| `--swedish` | show the 7-bit national characters as Ä Ö Å ä ö å, and @ \` as É é (default) |
| `--norwegian` | show them as Æ Ø Å æ ø å |
| `--ascii` | show them as the 7-bit codes `[ \ ] { \| }` |
| `-Z SECONDS`, `--clock SECONDS` | fix the clock at SECONDS since 1970; the random numbers no longer stir in the time, so the same typing plays the same game |
| `--no-hold` | do not wait where the program pauses |
| `--terminal N` | the terminal's logical device number (default 1) |
| `--raw` | pass the terminal bytes through untranslated |
| `--debug` | debug commands on a line starting with `#`: `#peek ADDR [COUNT]`, `#poke ADDR VALUE...`, `#find VALUE... [in FROM TO]`, `#dump FILE` (octal; decimal with a trailing `.`) |
| `--prog FILE` | run another one-bank program file instead |
| `-v`, `--verbose` | log monitor calls on standard error |
| `-T`, `--trace` | trace every instruction on standard error |
| `-h`, `--help` | list the options |
| `--version` | show the version |

## Recommended start

`run.bat`
