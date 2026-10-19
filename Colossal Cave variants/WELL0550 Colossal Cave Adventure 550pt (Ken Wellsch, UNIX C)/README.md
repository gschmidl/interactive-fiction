# Adventure, 550 points (Ken Wellsch, UNIX C)

David Platt's 550-point *Adventure* as Ken Wellsch rewrote it in C for BSD 4.2 at the University of
Waterloo (1984-85): a small kernel that runs the game logic written in Platt's A-code, compiled
by `munge` into a key file and a record file. Built from David Welbourn's 2015 transcription of
1986 printouts. Native Windows console port.

## Command line

`adv.exe [-u]` (reads `adv.key` and `adv.rec` from the current folder; `SAVE` writes `freezer`
there)

| Option | Effect |
| --- | --- |
| `-u` | start as a wizard, which lifts the 30-minute wait the game imposes after `SAVE` and the end of the game that follows a save |

## Recommended start

`run.bat`
