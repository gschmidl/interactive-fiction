# Faemar (1982)

*Faemar*, a 1982 text adventure written in a MacLisp-family Lisp: a generic engine and parser plus
the world of Faemar, with a fountain, a stone idol with precious eyes and a mace to wield. Only its
source survives. The two programs are a small Lisp interpreter for that dialect with the archived
source built in, so the original text runs as written:

| Program | Build |
| --- | --- |
| `faemar-fixed.exe` | two changes to the game: "move mace on hand" now wields the mace (the original loops forever there; "to hand" and "in hand" always worked), and taking the idol's eyes ends the game with a win message |
| `faemar.exe` | the archived source exactly, bugs included |

## Command line

`faemar-fixed.exe [-debug]` (the same for `faemar.exe`)

| Option | Effect |
| --- | --- |
| `-debug` | report the loading and, after a runaway recursion, the last calls on standard error |

## Recommended start

`faemar-fixed.exe`
