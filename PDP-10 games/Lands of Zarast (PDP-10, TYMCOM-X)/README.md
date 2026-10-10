# Lands of Zarast (Tymshare, 1984-88)

A homebrew Dungeons & Dragons-style adventure in Tymshare BASIC for a PDP-10 under TYMCOM-X, signed
"Sauron the Feared" (Stafford): roll up characters, take a party into the dungeon, fight, find
treasure and rise in rank. Two versions, whose texts are identical and whose characters and world
files work in either:

| Program | Version |
| --- | --- |
| `zarast.exe` | the December 1984 game, with the older stand-alone adventure *the Land of Fred* and two character generators |
| `zarast87.exe` | the later game (September 1988 build, and the May 1987 one) |

The original compiled programs run on a built-in DECsystem-10 emulator. The first run builds the
world file and rolls up a character; everything the game keeps (world, characters, the game in
progress) lives in one folder, the current directory by default.

## Command line

`zarast.exe [options] [PROGRAM]`

| Program | Runs |
| --- | --- |
| `dungeon` (default) | the dungeon crawl, the game |
| `overworld` | the older stand-alone adventure, the Land of Fred |
| `newchar` | character creation, quick |
| `character` | character creation, the long form |
| `filer` | rebuild the world file |

`zarast87.exe [options] [PROGRAM]` takes `dungeon` (default, the 1988 build), `older` (the May 1987
build) and `newchar`.

| Option | Effect |
| --- | --- |
| `--fix` | repair the bugs players ran into in the Land of Fred (1984 only): SAVE and RESTORE, lamps and torches, the store, negative damage, a titan that will not stay dead, the game stopping at the 85th kill; winning ends the game |
| `--debug` | commands at any prompt: `#GOD` (invulnerability), `#STATS` (best abilities, full hit points and mana), `#GOLD` (9,999,999 gold), `#HELP` |
| `-d DIR` | keep characters and the world in DIR (default: the current directory) |
| `-q` | do not pause where the game pauses |
| `-e` | echo what you type, as the Tymshare monitor did |
| `--lower` | pass lower case through instead of folding it to capitals (the game then misreads Y and N) |
| `-t MIN` | freeze the clock at MIN minutes past midnight |
| `--no-setup` | skip the first-run set-up (world file and first character) |
| `-v` | report monitor calls the port does not implement (repeat for more) |
| `-T` | trace every instruction |
| `-h`, `--help` | list the programs and options |

## Recommended start

`zarast.exe`, started in its own folder; `zarast.exe --fix overworld` for the Land of Fred.
