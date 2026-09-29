# Skattejakt

Crowther and Woods' *Adventure* in Norwegian, expanded to 500 points, for Norsk Data's ND-100 under
SINTRAN III; author and date unknown. The 350-point cave is there in translation, with a princess, a
throne, a sword, a harp and a suit of armour besides. `skattejakt.exe` is an ND-100 with enough of
SINTRAN III to run the original NORD FORTRAN program unchanged. The cave is open at all hours.

Useful words: `HJELP`, `INFO`, `N S Ø V OPP NED INN UT`, `TA`, `SLIPP`, `TENN`, `INNHOLD`, `POENG`,
`SLUTT`. `SPAR` (or `UTSETT`, `PAUSE`) suspends the game and saves it as a program file; the game
itself makes you wait 90 minutes before continuing it, unless you are the wizard (magic word
`LURVEN`), and as it counts the minutes in 16 bits it can refuse a game suspended more than 22 days
before. `run.bat` lifts both.

## Command line

`skattejakt.exe [options] [SAVED-GAME]` (`run.bat` adds `-u` and passes its parameters on)

| Option | Effect |
| --- | --- |
| `SAVED-GAME` | continue a game suspended with `SPAR` |
| `-u`, `--unlimited` | a suspended game goes on at once, however long ago it was saved |
| `-s DIR`, `--save-dir DIR` | where suspended games are written and looked for (default: the current directory) |
| `-d DIR`, `--data DIR` | where the game's program file is (default: `data` beside the program) |
| `--norwegian` | show the 7-bit national characters as Æ Ø Å æ ø å (default) |
| `--swedish` | show them as Ä Ö Å ä ö å, and @ \` as É é |
| `--ascii` | show them as the 7-bit codes `[ \ ] { \| }` |
| `--raw` | pass the terminal bytes through untranslated |
| `--no-hold` | do not wait where the program pauses |
| `--vdu` | tell the program the terminal is a screen (VT100) |
| `--terminal N` | the terminal's logical device number (default 1) |
| `-Z SECONDS`, `--clock SECONDS` | fix the clock at SECONDS since 1970, for repeatable sessions |
| `--debug` | debug commands on a line starting with `#`: `#peek ADDR [COUNT]`, `#poke ADDR VALUE...`, `#find VALUE... [in FROM TO]`, `#dump FILE` (octal; decimal with a trailing `.`) |
| `--prog FILE` | run another one-bank program file instead |
| `-v`, `--verbose` | log monitor calls on standard error |
| `-T`, `--trace` | trace every instruction on standard error |
| `-h`, `--help` | list the options |
| `--version` | show the version |

## Recommended start

`run.bat`, and `run.bat NAME` to continue the game saved as `NAME`.
