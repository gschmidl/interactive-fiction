# The King's Mission Game (Kent A. Wendler, PLATO / CYBIS)

Kent A. Wendler's game (1977, University of Illinois; later in Control Data's CYBIS Authors
Library): the King sends you to slay every goblin, zombie, werewolf or demon in one of his enchanted
realms - fields of trees, treasure chests, living pyramids and monsters - and the missions grow
harder as your rating rises. `KingsMission.exe` runs the original lessons and dataset (last edited
September 1985) on a built-in TUTOR interpreter and PLATO terminal.

## Command line

`KingsMission.exe [options]`

| Option | Effect |
| --- | --- |
| `--name NAME` | the PLATO name to sign on as (default: your Windows user name) |
| `--course COURSE` | the PLATO course to sign on in (default `home`) |
| `--saves DIR` | where the saved state is kept (default: a `saves` folder beside the program) |
| `--seed N` | fixed random numbers (default: from the clock) |
| `--textlog` | print a transcript of the text on the screen on standard output |
| `--script FILE` | run without a window, taking the keys from FILE (for testing) |

## Recommended start

`KingsMission.exe`
