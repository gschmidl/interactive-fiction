# ADVENT (Don Woods and Donald E. Knuth, 1998)

Donald E. Knuth's CWEB rewrite (1998, revised 1999) of Don Woods' 350-point *Adventure*: a literate
C program faithful to Woods' FORTRAN down to the messages. Built for the Windows console from Knuth's
untouched source through a change file that adds his later errata.

## Command line

`advent.exe [options]` (`run.bat` passes its parameters on)

| Option | Effect |
| --- | --- |
| `--seed N` | replay a game: the dice start from N instead of the clock |
| `--no-fixes` | the program as Knuth's text has it. Otherwise three bugs are fixed: the ranks are one class too high (349 points already made you Grandmaster), a bottle of water carried when the cave closes lets you carry eight things in the endgame, and the rest of an over-long input line becomes the next command |
| `--debug` | test commands, on a line starting with `#`: `#show`, `#loc N`, `#where OBJ`, `#put OBJ PLACE`, `#prop OBJ N`, `#set VAR N` |
| `-h`, `--help` | list the options |

## Recommended start

`run.bat`
