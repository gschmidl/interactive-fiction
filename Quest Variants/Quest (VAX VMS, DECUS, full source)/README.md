# QUEST (Ball State University, 1984-85)

QUEST, a dungeon crawler in VAX FORTRAN by Chris K. Kelley at Ball State University, with Owen G.
Anthony and W. T. Konopa, from the DECUS Spring 1985 tape (version 3.36 of 27 April 1985): six
dungeons of eight levels each, 119 monsters, 91 magic items, a magic shop, a temple, four character
classes and permanent death. Keys are single keystrokes (`H` is help everywhere). The VAX FORTRAN
source is compiled for the Windows console, with the game's data files, including 55 characters
recovered from 1985. The access file as shipped keeps the game open at all hours.

## Command line

`quest.exe [options]`

| Option | Effect |
| --- | --- |
| `-f`, `--fast` | skip the timed pauses the VAX used for pacing |
| `-s N`, `--seed N` | start the dice from N instead of the clock |
| `--freeze "YYYY-MM-DD hh:mm:ss"` | pin the clock (with `--seed`, a session repeats) |
| `-h`, `--help` | list the options |

Environment variables: `QUEST_DATA` (the data folder), `QUEST_SEED`, `QUEST_FREEZE` and `QUEST_FAST`
(the same as the options), and `QUEST_USERNAME` and `QUEST_UIC`, the account the game sees (by
default your Windows user name with UIC 000100). `QUEST_USERNAME=00CKKELLEY` with `QUEST_UIC=065244`,
the author's account, adds `O` to the main menu, the editor for characters and dungeons, and lets you
run anyone's character.

## Recommended start

`quest.exe` (add `--fast` to skip the pacing pauses)
