# Adventure in C: Jaeger/Pohl (1984) and Daimler (1990), 350 points

Two early C versions of the 350-point *Adventure*, each built for the Windows console from its own
source:

| Launcher | Program |
| --- | --- |
| `play-pohl.bat` | J. R. Jaeger's BDS C conversion of the FORTRAN, standardised for Unix by Jerry D. Pohl in 1984 (Pohl's revision of March 1990). The dwarves are re-placed at random every turn instead of walking. Starts at the end of the road |
| `play-daimler.bat` | the same program after Martin Heller's OS/2 conversion (1988) and Daimler's Turbo C 2.0 conversion (1990), with Pohl's 1984 texts. Starts in the building |

The launchers keep each game's saved games apart, in `saves\pohl` and `saves\daimler`.

## Command line

`advent.exe [options]` (both programs; the launchers pass their parameters on)

| Option | Effect |
| --- | --- |
| `-r`, `--restore` | start from a saved game (the game asks for its name). `SUSPEND`, `SAVE` or `PAUSE` writes one |
| `-d` | the authors' debug output: give it up to three times for Pohl's program, twice for Daimler's |
| `--no-fixes` | the programs as they were. Otherwise their bugs are fixed: a dwarf can block your way (a precedence slip made that impossible), and in Daimler's program the pirate keeps out of the rooms it should, messages numbered above 127 print the right text, `LOG` with an object no longer ends the game, and `BACK` after a forced move no longer reads past the tables |
| `-h`, `--help` | list the options |

## Recommended start

`play-pohl.bat` or `play-daimler.bat`
