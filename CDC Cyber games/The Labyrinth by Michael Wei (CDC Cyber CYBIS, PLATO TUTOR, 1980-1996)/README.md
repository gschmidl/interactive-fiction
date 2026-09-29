# The Labyrinth (Michael Wei, PLATO / CYBIS, 1980-1996)

"A constantly evolving dungeon game" by Michael Wei (with Randy Harmelink), written in TUTOR in 1980
and maintained by many hands until 1996: a castle with shops and a casino above a dungeon drawn as a
line-drawn 3D corridor view, characters of eleven races, monsters to fight or charm, treasure,
potions, scrolls and spells. Two versions from the CYBIS pack, each running the original lessons on a
built-in TUTOR interpreter and PLATO terminal:

| Program | Version |
| --- | --- |
| `Labyrinth.exe` | the version as last configured, with the dungeon "Khazad-dûm" |
| `old\LabyrinthOld.exe` | the "(OLD VERSION)" |

## Command line

`Labyrinth.exe [options]` (the same for both)

| Option | Effect |
| --- | --- |
| `--name NAME` | the PLATO name to sign on as (default: your Windows user name) |
| `--course COURSE` | the PLATO course to sign on in (default `home`) |
| `--saves DIR` | where the saved state is kept (default: a `saves` folder beside the program) |
| `--seed N` | fixed random numbers (default: from the clock) |
| `--textlog` | print a transcript of the text on the screen on standard output |
| `--script FILE` | run without a window, taking the keys from FILE (for testing) |

## Recommended start

`Labyrinth.exe`
