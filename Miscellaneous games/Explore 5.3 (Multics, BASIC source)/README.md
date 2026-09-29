# Explore 5.3 (Jim Lippard, Multics, 1980)

*Explore*, a cave adventure in the manner of *Colossal Cave* that Jim Lippard wrote at 14 on Multics
in Phoenix in 1979-80; version 5.3 of 6 June 1980, as he reconstructed it in 2026 from his printouts.
You start at a shack in a forest clearing and go down into a cave of 58 rooms to collect treasures,
deal with a troll, a dragon, a cyclops, a scylla and knife-throwing dwarves, and learn its magic
words; 500 points makes you a Grand Master Explorer. `explore.exe` runs the original Multics BASIC
on Lippard's own interpreter, MBasic, with a Perl of its own. The cave is always open; the sorcerer's
word is "hello". Saved games and the winners' tablet are kept in a `saves` folder beside the
program.

## Command line

`explore.exe [options] [game arguments]` (`run.bat` passes its parameters on)

| Option | Effect |
| --- | --- |
| `--share DIR` | the read-only game files (default: `share` beside the program) |
| `--var DIR` | the writable data folder (default: `saves` beside the program) |
| `--home DIR` | the player's home folder, for saved games, abbreviations and `start_up.explore` (default: the `--var` folder) |
| `--basic FILE` | the main BASIC program |
| `--helpers DIR` | the BASIC helper programs |
| `--root DIR` | where Multics paths that are not mapped end up |
| `--prefix M=U` | map a Multics path prefix M to a Windows path U (repeatable) |
| `-h`, `--help` | list the options |

The game's own Multics control arguments of 1980, passed to it unchanged:

| Argument | Effect |
| --- | --- |
| `-brief`, `-bf` | no welcome message and no news lines |
| `-modes STR` | set modes from a comma-separated list (`brief`, `full`, `fast`, `quit`, `^quit`) |
| `-version` | print the long version line |
| `-no_version` | print no version line |
| `-abbrev PATH`, `-ab PATH` | use PATH as the abbreviations file |
| `-pathname PATH`, `-pn PATH` | read the game database from PATH |
| `-no_startup`, `-ns` | do not run `start_up.explore` |
| `-table_space`, `-ts` | report the table space used while loading |

`EXPLORE_SHAREDIR`, `EXPLORE_VARDIR`, `EXPLORE_HOME`, `EXPLORE_BASIC`, `EXPLORE_HELPERS`,
`EXPLORE_ROOT` and `MBASIC_LIB` in the environment set the same locations.

## Recommended start

`run.bat`
