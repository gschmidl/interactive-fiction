# Adventure ]I[ (1400 points)

A 1400-point descendant of *Colossal Cave* written around 1978-79 in HP 2000 Access Time-Shared
BASIC, attributed to Alex Guma, then a high-school student in Fairfax County, Virginia. It is the
cave with a sense of humour: the Frobozz Magic Sno-Disc Company, a zarka that only eats pizza, a ski
resort, a subway and a nuclear reactor that melts down unless you stop it. `advent.exe` is an HP 2000
BASIC interpreter that runs the archived listings directly and builds the data files on first run.

Two versions of the program survive:

| Variant | Source |
| --- | --- |
| `recon` (default) | Guma's own 2023 reconstruction |
| `orig` | Rick Hammerstone's 1981-82 line-printer listing |

Each has one line repaired from the other's reading.

## Command line

`advent.exe [options]`

| Option | Effect |
| --- | --- |
| `-v VARIANT` | play `recon` (default) or `orig` |
| `-r` | seed the dice from the clock. Without it every game rolls the same numbers, as on the HP, and the dark room's xeener bugs always kill you on the first try |
| `--rebuild` | rebuild the data files before playing |
| `-u ID` | the account the game sees (default `B500`, the game's own account, which is on the wizard list). `-u S999` plays as an ordinary user; `-u S3xx` brings back the school's "Advent is down from 8:01 to 1:20" lockout |
| `-p DIR` | where the BASIC programs are |
| `-d DIR` | where the data files are |
| `-e` | echo input lines when input is not a terminal |
| `-q` | do not print `DONE` when a program stops |
| `PROG...` | run these BASIC programs instead of the game (a general HP 2000 BASIC interpreter) |
| `-h`, `--help` | list the options |

## Recommended start

`advent.exe -r`
