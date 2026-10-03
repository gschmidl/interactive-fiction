# Wellesley Adventure V6.2 (Eric Roberts, 655 points)

The large *Colossal Cave* made at Wellesley College by Mark Edwards, Mark Sylvester and Eric Roberts,
with a pantry, a honeycomb and a beehive, a road west to a stone spire and much more cave: Roberts'
FORTRAN source of 3 March 2010, compiled for the Windows console. The later 665-point revision
(V6.4.2) is ROBE0665.

## Command line

`newadv.exe [options]` (`run.bat` passes its parameters on and runs the game in a `saves` folder
beside it, where `SAVE` and `RESTORE` keep `newadv.sav`)

| Option | Effect |
| --- | --- |
| `--no-fixes` | keep the original's bug: `SAVE` forgets what is in the containers, so a restored bottle is full of water again |
| `-h`, `--help` | list the options |

The FORTRAN program never seeds its random numbers, so every game rolls the same dice.

## Recommended start

`run.bat`
