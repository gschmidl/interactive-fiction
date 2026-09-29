# The unnamed volcano adventure (North Star BASIC)

An untitled adventure in North Star BASIC, the file DBACK on a NorthStar Horizon work disk: escape
the headhunters down a volcano into a lava tube, find a lantern and a gold brick, cross a maze and
reach a room with a trap door. Moves are single letters - `U`, `D`, `B` (back), `R`, `F`, `T` (take) -
and `INVENTORY`. The game is unfinished by design: every way on ends it, in the pit ("DUNGEON UNDER
CONSTRUCTION"), in the orcs' jaws or in their prison. `volcano.exe` runs the program on the North
Star BASIC from the same disk on an emulated Z80. The author's own warps, typed at `MOVE?`, are
`MAZE1`, `TRAPDOOR`, `PIT` and `SHAFT`.

## Command line

`volcano.exe [options]` (`run.bat` passes its parameters on)

| Option | Effect |
| --- | --- |
| `--seed MS` | count the time you take to type as MS milliseconds, so the maze's random way out repeats |
| `--no-fixes` | the program exactly as on the disk. Otherwise one line is changed so that the capture by the orcs is shown instead of being skipped |
| `--basic` | stay in BASIC (READY) when the game ends |
| `--direct` | North Star BASIC alone, without the game |
| `--check` | compare the program as BASIC tokenises it with the file on the disk |
| `-u`, `--unlimited` | accepted; the game has no limits to lift |
| `--trace`, `--sample` | debugging |
| `-h`, `--help` | list the options |

## Recommended start

`run.bat`
