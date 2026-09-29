# Moria (PLATO / CYBIS, 1978-1993)

*Moria* by Kevet Duncombe and Jim Battin, the great PLATO dungeon game: underground rooms and
corridors drawn as a first-person 3D view, a city on top, four wilderness terrains, sixty levels,
guilds, spells, monsters and parties of players. Written in TUTOR at the University of Illinois;
this is the CYBIS version of 30 November 1993. `Moria.exe` runs the original lessons and dataset on a
built-in TUTOR interpreter and PLATO terminal. You play alone; your character belongs to your PLATO
name.

## Command line

`Moria.exe [options]`

| Option | Effect |
| --- | --- |
| `--name NAME` | the PLATO name to sign on as, which owns the character (default: your Windows user name) |
| `--course COURSE` | the PLATO course to sign on in (default `home`) |
| `--saves DIR` | where the saved state is kept (default: a `saves` folder beside the program) |
| `--seed N` | fixed random numbers (default: from the clock) |
| `--textlog` | print a transcript of the text on the screen on standard output |
| `--script FILE` | run without a window, taking the keys from FILE (for testing) |

## Recommended start

`Moria.exe`
