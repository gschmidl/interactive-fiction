# My World (ND-100, 1983)

MY_WORLD, an adventure in ND-Pascal by Mikael Johansson of a Swedish computer club (program of
September 1983, world of April 1983): a forest, a cottage on a hill, and an iron gate into the
mountain, beyond it a hall of mists, a snake, a fissure, a museum of stuffed animals, a troll's
bridge and a maze of twisty little tunnels where a pirate lives. Treasures count double in the
cottage; there is no end, the score says how far you got, and you have three lives. Commands are a
verb and an object (`GET LAMP`, `LIGHT LAMP`); several go on one line separated by commas. `myworld.exe`
is an ND-100 with enough of SINTRAN III to run the compiled program, which reads its world from the
file it read then; the port's fixes (every command stopped the game with an overflow, long words
crashed it) are built in.

## Command line

`myworld.exe [options]`

| Option | Effect |
| --- | --- |
| `-d DIR`, `--data DIR` | the program and its world (default: `data` beside the program) |
| `--no-rubout` | leave the delete keys to the program instead of rubbing characters out (the program cannot rub out itself) |
| `--uptime UNITS` | start the machine's uptime at UNITS (1/50 s) and run it with the instructions alone: one game for each value |
| `-Z SECONDS`, `--clock SECONDS` | fix the clock at SECONDS since 1970; the uptime stays 0, so the same commands meet the same dice |
| `--ascii` | show the 7-bit national characters as `[ \ ] { \| }` (default) |
| `--norwegian` | show them as Æ Ø Å æ ø å |
| `--swedish` | show them as Ä Ö Å ä ö å, and @ \` as É é |
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

`myworld.exe`
