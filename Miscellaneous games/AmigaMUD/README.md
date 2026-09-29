# AmigaMUD: the "uncle" world

The small demonstration world that came with Chris Gray's AmigaMUD (1991), a multi-user dungeon
system for the Amiga with its own scripting language: a house with an encyclopedia to read, potion
ingredients to put in a cauldron and spells to brew and cast. `amigamud.exe` is an interpreter for the
AmigaMUD language that loads the world's scripts (`go` and the five `.m` files) and plays it
single-user.

## Command line

`amigamud.exe [options] [FILE...]`

| Option | Effect |
| --- | --- |
| `FILE...` | the boot scripts to load (default: `go` in the current directory) |
| `--assign NAME=DIR` | map an Amiga volume name used by the scripts (such as `uncle:`) to a folder (default: the folder of the first script) |
| `--width WIDTH` | wrap the text at WIDTH columns (default 78) |
| `--strict` | refuse a stray `.` where the language wants `$`, instead of warning and going on |
| `--lint` | report undefined symbols in the loaded scripts |
| `--load-only` | load the scripts and stop |
| `--verbose` | report the files loaded and the login on standard error |
| `--echo`, `--no-echo` | echo input lines, or do not (by default they are echoed when input is not a terminal) |
| `-h`, `--help` | list the options |

## Recommended start

`amigamud.exe`, started in its own folder (double-clicking it does that).
