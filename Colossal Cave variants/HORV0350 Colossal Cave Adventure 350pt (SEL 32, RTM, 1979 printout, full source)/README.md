# Adventure, 350 points (SEL 32 / RTM, 1979)

The 350-point Crowther and Woods game as Ned Horvath ported it to the SEL 32 minicomputer under RTM
in 1978 and C. Norwood tidied it, by way of Gary Palter's portable FORTRAN version. Recovered from a
line-printer listing of 21 March 1979 (Arthur O'Dwyer's transcription). The cave is the standard one;
the vocabulary adds `G` and `T` for take, `SLAY` and `Q`. Native Windows console port of the FORTRAN.

## Command line

`advent.exe [options]` (`run.bat` adds `-u` and passes its parameters on)

| Option | Effect |
| --- | --- |
| `-u`, `--unlimited` | no prime time (the site shut the cave to all but wizards 8:00-18:00 on weekdays, offering a 30-turn demonstration game) and no 90-minute wait before a suspended game may be restored |
| `--day N` | pretend N days have passed since Saturday 1 July 1978 |
| `--time HHMM` | pretend the time is HH:MM. With `--day`, this freezes the clock that seeds the dice |
| `--auto` | pass the wizard's test without being asked (used to set the game up) |
| `--no-fixes` | leave out the two switchable repairs of the transcription (the lower-case alphabet, blank on the printout, and the input routine's empty-line check); the game then understands nothing |
| `-h`, `--help` | list the options |

## Recommended start

`run.bat`
