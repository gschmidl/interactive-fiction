# Starter Adventure (Eric Roberts, 240 points)

The 240-point "Starter Adventure, reminiscent of the early releases" that Eric Roberts put on the web:
a FORTRAN *Colossal Cave* (version 2.1), compiled by his WebFor
compiler for his SVM stack machine. No source is known. `starter.exe` is a C implementation of the
SVM and the WebFor run-time with the compiled program built in; it runs the program unchanged.

## Command line

`starter.exe [options]` (`run.bat` passes its parameters on and runs the game in a `saves` folder
beside it, where `SAVE` writes and `RESTORE` reads `newadv.txt`)

| Option | Effect |
| --- | --- |
| `--seed N` | fixed random numbers (the browser used its own random numbers) |
| `--no-fixes` | keep the browser edition's bug: a command starting with a word such as `at`, `with`, `off` or `all` stops the game with a run-time error |
| `--image FILE.js` | run another compiled SVM image instead |
| `--echo`, `--no-echo` | repeat input lines in the output, or do not (by default they are repeated when input is not a console) |
| `-T` | trace every instruction (diagnostics) |
| `-W` | report ordered string comparisons (diagnostics) |
| `-h`, `--help` | list the options |

## Recommended start

`run.bat`
