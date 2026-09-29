# Adventures in Pascal (Colossal Cave Adventure, 350 points)

Barry C. Breen's 1980-82 rewrite of the 350-point Crowther and Woods game in OMSI Pascal for RSX-11M on
a PDP-11/23, adapted from Kent Blackett's FORTRAN version and published on a DECUS tape in 1982. It
adds a wizard mode kept in a file, three saved-game slots, and a second text for the VT100 that
prints signs, voices and magic words in double-size letters. Native Windows console port.

## Command line

`adventure.exe [options]` (`run.bat` adds `-u`, passes its parameters on and waits for a key at the end)

| Option | Effect |
| --- | --- |
| `-u`, `--unlimited` | ignore the cave hours (the author's file shuts the cave to non-wizards for most of the working day) and the 45-minute wait before a suspended game may be resumed, and answer the wizard test for you (`MAGIC MODE` as the first command). Nothing on disk is changed |
| `-d DATE`, `--date=DD-MMM-YYYY` | pretend it is this date |
| `-t TIME`, `--time=HHMM[SS]` | pretend it is this time. The dice are seeded from the time of day, so a frozen clock repeats the whole game |
| `--no-fixes` | leave the original's three bugs in: fatal falls that do not kill, a bare `TAKE` beside the far end of a two-place object, and a lower-case new magic word that locks the wizards out |
| `--echo` | show each input line (for transcripts of redirected input) |
| `--data=DIR` | where the game database is (default `data` beside the program) |
| `--save=DIR` | where `ADVWIZ.DTA`, the wizard file with the saved games, is kept (default `save` beside the program) |
| `--debug` | test bench: at any prompt `#goto N`, `#take N`, `#put N LOC`, `#prop N V`, `#where N`, `#dwarf I LOC`, `#set NAME V`, `#show` |
| `-h`, `--help` | list the options |

The environment variables `ADVPAS_DATA`, `ADVPAS_SAVE`, `ADVPAS_DATE` and `ADVPAS_TIME` do the same as
`--data`, `--save`, `--date` and `--time`.

The game first asks "Are you using a VT100?": Windows Terminal is one, so answer `yes` for the
double-size lettering.

## Recommended start

`run.bat`
