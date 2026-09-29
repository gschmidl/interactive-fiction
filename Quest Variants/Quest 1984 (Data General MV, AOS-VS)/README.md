# Quest (1984, Data General AOS/VS)

An earlier version of the multiplayer role-playing game *Quest* for Data General's ECLIPSE MV
computers under AOS/VS, from a tape dumped on 14 November 1984 from the game's development directory
- the game as it was then being written, with its authors' own characters still in the user file.
"To prove your heroism, your first quest is to slay a dragon." Any name and password creates a
character; commands are single keys (`H` for help). The original world server and player program run
unmodified on a built-in ECLIPSE MV emulator (`aosvs32.exe`), after the animated CASTLE title. Start
the game in a second window and that window joins the same world as another player.

## Command line

`quest.bat [options]` runs `aosvs32.exe --port 4084 --title data\CASTLE -d data -s save [options]
data\QUEST.PR` and passes its parameters on. Deleting the `save` folder resets the world.

| Option | Effect |
| --- | --- |
| `--no-title` | straight into the game, without the CASTLE animation |
| `--lan` | let players on other computers join too (they run `quest.bat --join HOST`, or connect any telnet client to port 4084) |
| `--join HOST[:PORT]` | play in the world running on computer HOST |
| `--server` | run the world with nobody playing at this window |
| `--exit-when-empty` | with `--server`: stop once the last player has left |
| `--god` | highest strength, intelligence, experience, vision, perception and wealth, and never die; with `--server`, for everyone who joins |
| `--title FILE`, `-c FILE` | type this file on each player's screen first |
| `--port N` | the port the world uses (the launcher sets 4084) |
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
