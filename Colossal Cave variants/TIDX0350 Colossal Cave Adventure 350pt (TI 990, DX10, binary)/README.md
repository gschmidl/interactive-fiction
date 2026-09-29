# Adventure thru a Cave (350 points, TI 990)

"ADVENTURE - Wandering adventure thru a Cave" from a Texas Instruments TI 990 DX10 games tape: Don
Woods' 350-point game in TI 990 FORTRAN, in mixed case, with his wizard machinery - opening hours, a
short game in the off hours, magic mode and a 90-minute wait before a suspended game may be resumed.
Only the linked program survives; `adventure.exe` runs it on an emulated TI 990/10 and answers the
DX10 supervisor calls it makes.

At the start, "Will/did you save your game?": Enter or `NO` plays without a save file; `YES` asks
for a file, which `SAVE` suspends the game into and `RESTORE` (after the instructions question)
brings back.

## Command line

`adventure.exe [options]` (`run.bat` adds `-u`, runs the game in a `saves` folder beside it and
passes its parameters on)

| Option | Effect |
| --- | --- |
| `-u`, `--unlimited` | the cave never closes and a suspended game resumes at once |
| `--no-fixes` | the original exactly: the first random event in the first minute of an hour then freezes the game for up to a minute |
| `--clock=TIME` | a clock that starts at `YYYY-MM-DD HH:MM:SS` (or `HH:MM:SS` today) and advances a second per reading; the dice are seeded from it, so a session repeats |
| `--trace` | each DX10 supervisor call on standard error |
| `-h`, `--help` | list the options |

## Recommended start

`run.bat`
