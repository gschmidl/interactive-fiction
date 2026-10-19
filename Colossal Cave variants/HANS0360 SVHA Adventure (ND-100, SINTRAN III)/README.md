# SVHA Adventure (360 points)

*Colossal Cave* as it grew at NTH in Trondheim: Crowther and Woods' game by way of Bob Supnik's and
Kent Blackett's RT-11 version, expanded by Nils-Morten Nilssen and Svein Hansen and the "Studio 54
Hobbies Group", with parts from Greg Hassett's *Creative Computing* article; 360 points, dated 1984
and later sold by Norsk Data. `svha.exe` is an ND-100 with enough of SINTRAN III to run the original
NORD FORTRAN program. The game had no working save; the port adds `SAVE` (or `SUSPEND`, `PAUSE`) and
`RESTORE`.

## Command line

`svha.exe [options] [SAVED-GAME]`

| Option | Effect |
| --- | --- |
| `SAVED-GAME` | start a game saved with `SAVE` |
| `-s DIR`, `--save-dir DIR` | where saved games are written and looked for (default: the current directory) |
| `-d DIR`, `--data DIR` | where the game's files are (default: `data` beside the program) |
| `--no-hold` | do not wait where the program pauses (a 30-second wait at the start, most of a minute in one scene) |
| `--no-fixes` | play with the bugs the port fixes: `OPEN COFFIN` fails with the magic ring on, the soup cannot be poured in the ice pit (both make the game impossible to finish), `OPEN VAULT` shuts the open vault, and a command that takes no turn leaves its object behind for the next one |
| `--vdu` | tell the program the terminal is a screen (backspace-space-backspace deletion) |
| `--ascii` | show the 7-bit national characters as `[ \ ] { \| }` (default) |
| `--norwegian` | show them as Æ Ø Å æ ø å |
| `--swedish` | show them as Ä Ö Å ä ö å, and @ \` as É é |
| `--raw` | pass the terminal bytes through untranslated |
| `--terminal N` | the terminal's logical device number (default 1) |
| `-Z SECONDS`, `--clock SECONDS` | fix the clock at SECONDS since 1970, for repeatable sessions |
| `--debug` | debug commands on a line starting with `#`: `#peek ADDR [COUNT]`, `#poke ADDR VALUE...`, `#find VALUE... [in FROM TO]`, `#dump FILE` (octal; decimal with a trailing `.`) |
| `--prog FILE` | run another one-bank program file instead |
| `-v`, `--verbose` | log monitor calls on standard error |
| `-T`, `--trace` | trace every instruction on standard error |
| `-h`, `--help` | list the options |
| `--version` | show the version |

## Recommended start

`svha.exe --no-hold`
