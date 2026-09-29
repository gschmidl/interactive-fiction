# ADVENTURE4 (Colossal Cave Adventure, 660 points, Glaxo version 4.3)

Mike Arnautov's 660-point extension of *Adventure* as it ran on a Prime 50-Series under PRIMOS at
Glaxo in July 1984. The game is written in Arnautov's own adventure language, A-code, and only its
compiled database survives; the port runs that database byte for byte under his A-code executive
(rev. 19.2), converted from its FORTRAN 77 source. Native Windows console port.

## Command line

`adventure4.exe [options]` (`run.bat` adds `-u` and passes its parameters on)

| Option | Effect |
| --- | --- |
| `-u`, `--unlimited` | a restored game counts as saved long ago: restoring within 30 minutes is allowed ("only a wizard can restart a game in less than 30 minutes") and costs no points |
| `--seed N` | start the dice from N. Without it every game rolls the same dice, as every game on the Prime did |
| `--echo` | write each line read back, as the Prime's terminal echoed it (for comparing transcripts) |
| `-h`, `--help` | list the options |

## Recommended start

`run.bat`
