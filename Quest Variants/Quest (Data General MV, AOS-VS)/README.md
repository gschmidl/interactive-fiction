# Quest (Data General AOS/VS, NADGUG tape)

*Quest*, a multiplayer role-playing game for Data General's ECLIPSE MV computers under AOS/VS, written
in DG FORTRAN 77 around 1983-84 and distributed on the NADGUG users' group library tape: a map of
forests, cities and castles seen from above on a Dasher D200 terminal, with sieges, spells, dragons
and up to ten players in one world. Any name and password creates a character; commands are single
keys (`H` for help) and ESC leaves, saving the character. The original world server and player
program run unmodified on a built-in ECLIPSE MV emulator (`aosvs32.exe`). Start the game in a second
window and that window joins the same world as another player.

## Command line

`quest.bat [options]` runs `aosvs32.exe --port PORT -d data -s save [options] data\QUEST.PR` and
passes its parameters on. Deleting the `save` folder resets the world.

PORT is the first free port from 8000 to 9000. Ports that other programs hold and ports Windows
has reserved are skipped (the port finder from eXo's `launch_if.bat`, extended to skip reserved
ports). If this game's world is already running on a port in that range, the new window joins it.
With `--port` or `--join` no search is made.

| Option | Effect |
| --- | --- |
| `--lan` | let players on other computers join too (they run `quest.bat --join HOST:PORT`, or connect any telnet client to PORT; the window's title shows both) |
| `--join HOST[:PORT]` | play in the world running on computer HOST (PORT defaults to 8000) |
| `--server` | run the world with nobody playing at this window |
| `--exit-when-empty` | with `--server`: stop once the last player has left |
| `--god` | highest strength, intelligence, experience, vision, perception and wealth, and never die; with `--server`, for everyone who joins |
| `--title FILE`, `-c FILE` | type this file on each player's screen first |
| `--no-title` | leave the title out |
| `--port N` | the port the world uses (the launcher finds one from 8000 on) |
| `-d DIR` | where the programs and their data are |
| `-s DIR` | where the world and the characters are saved |
| `-T MODE` | the player's terminal: `ansi` (default at a console), `snap`, `raw` or `off` |
| `-h`, `--help` | list the options |

Testing and debugging: `-p KEYS SCREENS` a scripted extra player (up to 14), `-S FILE` another server
program, `-q` no server, `-e HEX` entry point, `-v` trace system calls and file access, `-t`
instruction trace, `-F` floating-point trace, `-D LO HI` dump a word range at the end, `-W LO HI`
report writes to a word range, `-P PC ADDR VAL` set a word each time the program reaches PC, `-n
COUNT` stop after COUNT instructions.

## Recommended start

`quest.bat`, and `quest.bat` again in another window for each further player.
