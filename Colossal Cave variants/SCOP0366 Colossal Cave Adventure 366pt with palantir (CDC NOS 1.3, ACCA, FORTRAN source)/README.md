# Adventure, 366 points, with the palantir (CDC NOS 1.3)

Kent Blackett's and Bob Supnik's FORTRAN *Adventure* as Bill Hein and Shelley Hobson brought it up
under NOS 1.3 on a CDC Cyber at ACCA. They added an overgrown path to a dell and a gazebo with elvish
runes, and a sixteenth treasure, the palantir, which you PEER into for hints; the cave's hours call it
Mirkwood. Native Windows console port of the CDC FORTRAN source.

## Command line

`advent.exe [options]` (`run.bat` adds `-u` and passes its parameters on)

| Option | Effect |
| --- | --- |
| `-u`, `--unlimited` | the cave's hours do not apply. Without it the game refuses to run 06.00-11.30 and 13.30-15.30 unless you are the wizard (password `WORMTONGUE`) |
| `--seed N` | the 48-bit seed to start the dice at, instead of one from the clock |
| `--time HHMM` | hold the clock still |
| `--no-fixes` | accepted; the port has no fixes to leave out |
| `-h`, `--help` | list the options |

## Recommended start

`run.bat`
