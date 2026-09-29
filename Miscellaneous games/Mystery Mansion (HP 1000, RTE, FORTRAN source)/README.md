# Mystery Mansion (HP 1000, revision 16)

Bill Wolpert's *Mystery Mansion* for the HP 1000, "MYSTERY 2. REVISION 16 - 23 JUL 81", from the
HP 1000 users' contributed library. A murder has been done in the mansion - scene, murderer and
weapon are drawn from the clock - and the day runs out: night falls at turn 300, the mansion goes up
at 450 and the game gives up on you at 550; every 30 seconds spent thinking is a turn too. Answer
questions with YES or NO in full. The FTN4 source is compiled for the Windows console. Wolpert's
debug menu: `DISPLAY`, then to "nn ARE YOU THE CREATOR?" answer `YES` and 99 minus nn.

## Command line

`mmm.exe [options]` (`run.bat` passes its parameters on)

| Option | Effect |
| --- | --- |
| `--site` | run the game as on Wolpert's own machine: playing hours (7:00-7:30, 11:30-12:00 and 16:00-17:30) with a security code asked of anyone else, the player's name and messages, a count of runs, a log of games and a request for comments |
| `-u`, `--unlimited` | give the security code of a player allowed to play at any hour (15815); only matters with `--site` |
| `--code N` | give another security code (27740, 24500 and 31313 are also valid) |
| `--time HH:MM[:SS[.mmm]]`, `--date YYYY-MM-DD` | hold the clock. The mystery and every roll of the dice come from it, so a game repeats, and thinking time no longer costs turns |
| `--no-fixes` | keep the original's bug: `RECORD` onto the player's own terminal prints the same message for ever |
| `-h`, `--help` | list the options |

## Recommended start

`run.bat`
