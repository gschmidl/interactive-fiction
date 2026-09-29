# QUEST version 1 (Philips P7000, multiplayer)

QUEST version 1, a multi-player cave game in Pascal, as a Danish site ran it under MFE on a Philips
P7000 - a Four-Phase Systems IV/90. Almost everything about the cave, the quest included, is drawn at
random when the game starts, actions take real time, and you type sentences; `QHELP.txt` is the
game's own manual and `HELP` gives the short version. `quest.exe` boots the site's own disc pack on an
emulated Four-Phase IV/90, starts MFE and QUEST as the operator did, and gives each player a 7200
terminal in a window of their own - up to six. The game closes itself on weekdays 9-12 and 13-17
unless the operator gives the site's password.

## Command line

`quest.exe [options]` (`run.bat` adds `-u` and passes its parameters on)

| Option | Effect |
| --- | --- |
| `-u`, `--unlimited` | open the cave at any time: the operator gives the site's password when QUEST asks. Without it the cave is closed on weekdays 9-12 and 13-17 |
| `-p N`, `--players=N` | the number of players, 1 to 6 (default 1); start the game in further windows and they join it |
| `--easy` | play with the 'EASY' library that QUEST came with (the site had installed the 'HARD' one) |
| `--lan` | let players on other computers join |
| `--join=HOST[:PORT]` | be a player in the game started on computer HOST |
| `--port=N` | the TCP port for the players' windows (default 7000) |
| `--transcript` | follow terminal 0 as a scrolling log and read its lines from standard input (the default when either is not a console) |
| `--fixed-clock` | the 60 Hz clock counts instructions and MFE gets a fixed time and date, so a run repeats exactly |
| `--trace=FILE` | write an instruction trace to FILE (debugging) |
| `-h`, `--help` | list the options |

## Recommended start

`run.bat`, or `run.bat --players=N` for N players, then `run.bat` in N-1 more windows.
