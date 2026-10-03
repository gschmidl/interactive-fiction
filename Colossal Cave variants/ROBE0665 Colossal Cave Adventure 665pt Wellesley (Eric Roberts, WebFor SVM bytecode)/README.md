# Wellesley Adventure V6.4.2 (Eric Roberts, 665 points)

The large *Colossal Cave* made at Wellesley College by Mark Edwards, Mark Sylvester, Eric Roberts
and Kristin Powers, as Roberts put it on the web: his FORTRAN program (database of 7 June 2021),
compiled by his WebFor compiler for his SVM stack machine. No source of this revision is known; the
earlier 655-point source is ROBE0655. `wellesley.exe` is a C implementation of the SVM and the
WebFor run-time with the compiled program built in; it runs the program unchanged.

## Command line

`wellesley.exe [options]` (`run.bat` passes its parameters on and runs the game in a
`saves\wellesley` folder beside it, where `SAVE` writes and `RESTORE` reads `newadv.txt`)

| Option | Effect |
| --- | --- |
| `--seed N` | fixed random numbers (the web page used its own random numbers) |
| `--no-fixes` | keep the web edition's bug: a command starting with a word such as `at`, `with`, `off` or `all` stops the game with a run-time error |
| `--image FILE.js` | run another compiled image |
| `--echo`, `--no-echo` | repeat input lines in the output, or do not (by default they are repeated when input is not a console) |
| `-T` | trace every instruction (diagnostics) |
| `-W` | report ordered string comparisons (diagnostics) |
| `-h`, `--help` | list the options |

## Recommended start

`run.bat`
