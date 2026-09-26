# Sessions recorded on the reference system

These were recorded 2026-09-21 on:
- Dave Pitts' sim990 3.3.0 (Windows) running DX10 3.7;
- the GAMES tape restored with `RD MT01 .GAMES`;
- `ADVENTUR` run on the system console (telnet 2099), a teleprinter; input is upper case (the console is attached
  with `/u`).

`..\..\..\..\..\_work\_TI990_work\tools\ref990.py` typed the answers in `NAME.in` one prompt at a time and kept the console's
raw bytes in `NAME.raw`. The recording starts at the `ADVENTUR` command and ends at SCI's next prompt.

| Session | Clock (IDT) | What it covers |
|---|---|---|
| ref1 | Saturday 2026-09-19 20:05 | No save file. Into the cave: lamp, grate, cage, bird, gold, then back and forth until two dwarves appear. Knives miss, then one hits; the reincarnation question gets wrong answers, then N; QUIT's question gets wrong answers, then YES. |
| ref2 | Saturday 2026-09-19 01:05 | YES to the save question, a new file `.GAMES.MYSAVE`, into the building, GET LAMP, SAVE, YES. |
| ref3 | the same, 2 minutes later | YES, the same file: "The file exists, do you want to use it?" Y. The game starts fresh (RESTORE was not typed), LOOK, QUIT. |
| fuzz1 | Saturday 2026-09-19 11:16 | Recorded 2026-09-22 by `..\reffuzz.py`, like fuzz2-5. The cave is closed (the program counts that Saturday as a weekday): the wizard question gets NO, the short-exploration question gets the walk's commands, then N. |
| fuzz2-5 | 2026-09-19 19:24, 20:31, 21:37, 22:44 | The walk into the cave (lamp, keys, grate, cage), 40 random commands (moves, verbs and objects, yes/no answers), QUIT, YES. |

`clocks.txt` gives the port's `--clock` for each. The game seeds RANDOM from the minute and second of its first
reading of the clock. The port's clock ticks once a reading, so one start second replays each session;
`crossref.py --find` searched for it.
