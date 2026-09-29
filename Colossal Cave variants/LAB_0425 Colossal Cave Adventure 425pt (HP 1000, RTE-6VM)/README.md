# Adventure II 2.2 (425 points, HP 1000)

"HP 1000 Adventure" version 2.2, the 425-point *Colossal Cave* that LABtec, the HP Technical Computer
User's Group of Los Angeles, distributed in 1987 for HP 1000 minicomputers running RTE-6/VM and RTE-A.
Version 2 added `SAVE`/`RESTORE`, a `MAGIC` maintenance mode,
mixed-case text and a larger cave. Native Windows console port of the FORTRAN; the shipped database
keeps the cave open at all hours.

## Command line

`adventure.exe [FILE]`

| Parameter | Effect |
| --- | --- |
| `FILE` | start from FILE instead of `ADVENTURE.DAT`: a game written by `SAVE` is a complete image in the same format, so `adventure.exe mygame.dat` resumes it |

Files are read from and written to the current directory, so start it in its own folder.

## Recommended start

`adventure.exe`, started in its own folder (double-clicking it does that).
