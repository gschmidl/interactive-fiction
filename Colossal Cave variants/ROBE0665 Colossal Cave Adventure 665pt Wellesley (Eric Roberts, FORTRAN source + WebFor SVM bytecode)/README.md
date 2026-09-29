# Wellesley Adventure (Eric Roberts, 665 points)

The large *Colossal Cave* made at Wellesley College by Mark Edwards, Mark Sylvester, Eric Roberts
and Kristin Powers, with a pantry, a honeycomb and a beehive, an overgrown roadway and much more cave.
Two revisions, built for the Windows console:

| Launcher | Program | Revision |
| --- | --- | --- |
| `play-wellesley.bat` | `wellesley.exe`: the web edition's compiled program (database of 7 June 2021), run by a C implementation of Roberts' SVM stack machine | V6.4.2, 665 points |
| `run.bat` | `newadv.exe`: Roberts' FORTRAN source of 3 March 2010, compiled | V6.2, 655 points |

Each launcher runs its game in its own folder under `saves`, where `SAVE` and `RESTORE` keep the
saved game.

## Command line

`wellesley.exe [options]`

| Option | Effect |
| --- | --- |
| `--seed N` | fixed random numbers (the web page used its own random numbers) |
| `--no-fixes` | keep the web edition's bug: a command starting with a word such as `at`, `with`, `off` or `all` stops the game with a run-time error |
| `--image FILE.js` | run another compiled image |
| `--echo`, `--no-echo` | repeat input lines in the output, or do not (by default they are repeated when input is not a console) |
| `-T` | trace every instruction (diagnostics) |
| `-W` | report ordered string comparisons (diagnostics) |
| `-h`, `--help` | list the options |

`newadv.exe [options]`

| Option | Effect |
| --- | --- |
| `--no-fixes` | keep the original's bug: `SAVE` forgets what is in the containers, so a restored bottle is full of water again |
| `-h`, `--help` | list the options |

The FORTRAN program never seeds its random numbers, so every game of `newadv.exe` rolls the same dice.

The launchers pass their parameters on.

## Recommended start

`play-wellesley.bat`
