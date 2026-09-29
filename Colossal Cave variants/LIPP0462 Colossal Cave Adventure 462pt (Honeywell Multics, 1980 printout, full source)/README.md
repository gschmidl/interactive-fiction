# Adventure 1.2 (462 points, Honeywell Multics, 1980)

The *Colossal Cave* that ran on Honeywell's Phoenix Multics system in 1980: Don Woods' game in Gary
Palter's Multics FORTRAN version, with early rooms and objects of Dave Platt's 550-point game, named
SUSPEND/RESTORE and a maximum of 462 points. Jim Lippard kept it for his Explorer post; this is his
2026 transcription of the listings, corrected and completed, built for the Windows console. The
business-hours lockout and the wait before restoring a suspended game are off.

## Command line

`adv462.exe` takes no parameters (`run.bat` starts it). `SUSPEND name` writes `saves\name.adv462`
beside the program and `RESTORE name` reads it back.

Environment variables:

| Variable | Effect |
| --- | --- |
| `ADV462_CLOCK="DAY MINUTE"` | set the clock (days since 1 January 1977, minutes since midnight). It seeds the dice, so a game repeats exactly |
| `ADV462_DATA` | the database file |
| `ADV462_DIR` | the folder of `adventure.newgame`, the prepared new game |
| `ADV462_SAVEDIR` | the folder of saved games |

`MAGIC MODE` as the first command lets a wizard change the hours and other settings; the magic word is
`dwarf`, and the answer to "do you know what I thought it was?" is `no`, then `dwarf` again.

## Recommended start

`run.bat`
