# Dungeon (Jonathan H. Reed, 1983)

Jonathan H. Reed's DUNGEON, June 1983, in Prime PL/I subset G: a dragon crawl through a 10 x 10 x 10
dungeon with typed commands (move, attack, search, take, hide, checktrap, untrap and more; `help`
lists them). The game asks for "a wierd positive number": that is the seed, and the same number gives
the same dungeon as it did on the Prime. The PL/I is carried across to C statement by statement and
built for the Windows console.

## Command line

`dungeon.exe [options]` (`run.bat` passes its parameters on)

| Option | Effect |
| --- | --- |
| `--no-fixes` | the program as it ran on the Prime: at "press <return> twice" the Returns alone do not go on, you have to type a word |
| `-h`, `--help` | list the options |

## Recommended start

`run.bat`
