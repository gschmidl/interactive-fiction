# Adventure, PL/1 Version 4.0 (Greg Price, 382 points)

Greg Price's *Adventure - PL/1 Version 4.0* (1981-84), the only known Australian variant, from an
IBM mainframe program on the SHARE/CBT tapes. It descends from Gary Palter's portable version of the
Crowther and Woods game and adds 32 points of science fiction: a spaceship with anti-intruder
defences, a great red ruby, a teleport bracelet and Orac, the computer from *Blake's 7*. The PL/I is
carried across to C statement by statement and built for the Windows console. The mainframe login
gate is skipped (the player is always authorized), so the turn and time limits do not apply; a
suspended game could be restored only an hour after it was saved, which `run.bat` lifts.

## Command line

`adventure.exe [options] [PARM]` (`run.bat` adds `-u` and passes its parameters on). Anything that is
not an option becomes the program's mainframe PARM string, and none of the PARM settings does anything
here. It reads `OBJECT`, the game database, from the current directory and keeps saved games
(`SUSPEND name`, `RESTORE name`) in `STORAGE` there, so start it in its own folder (`run.bat` does).

| Option | Effect |
| --- | --- |
| `-u`, `--unlimited` | a suspended game can be restored at once, not an hour later |
| `-h`, `--help` | list the options |

## Recommended start

`run.bat`
