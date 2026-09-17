# ADVENTURE/3000 — playable port

`adventure3000.exe` plays ADVENTURE/3000 3.2 (Benjamin Moser, *Creative
Computing*, November 1979) from the transcribed listing itself: the BASIC
source in `basic/` is the program as printed in the magazine, and the
interpreter in `src/hp3kbasic.c` is an HP 3000 BASIC (BASIC/3000) interpreter.
Nothing is translated or rewritten.

```
adventure3000              play
adventure3000 --no-fixes   play the listing exactly as printed, bugs included
adventure3000 --rebuild    rebuild the data files from the text records
adventure3000 --help       interpreter options (it will run any BASIC/3000 program)
```

Build with `make`, or `build.cmd` on Windows; the only dependency is a C89/C99
compiler.

## What is in here

| path | what it is |
| --- | --- |
| `basic/fixed/ADV3000.txt` | the program, with the four corrections listed in `../transcription/fixes.txt` |
| `basic/print/ADV3000.txt` | the program exactly as the magazine printed it (`--no-fixes`) |
| `data/fixed/`, `data/print/` | the four data files as text, one line per record; the interpreter turns them into BASIC data files on first run |
| `src/hp3kbasic.c` | the interpreter |
| `tests/` | the two walkthroughs recorded on a real HP 3000, and the script that replays them |

The first run builds `AITEMS`, `ADESCRIP`, `AMESSAGE` and `AMOVING` next to the
text records, plus an empty `BUG` file for the game's gripe command. Saved
games are written there too, as the game's own `CREATE`/`ASSIGN` do.

## How closely it follows the original

The game was first entered into the real thing: a Series 58 running MPE V/E
under SIMH, BASIC/3000 HP32101B.00.26. All 1,003 statements were accepted
without a single syntax error, the four data files were built there and
checked record by record, and the game was played.

Two walkthroughs recorded on that machine are in `tests/`, replayed by
`tests/run_tests.sh` against the same program here. Both come out **identical,
line for line** — 100 and 138 lines of game text, including the inventory's
column layout, the score line's number formatting, and a game saved to a file:

```
walkthrough b: matches the HP 3000 (100 lines)
walkthrough c: matches the HP 3000 (138 lines)
```

For the comparison both sides run a copy of the program with `RND(0)` replaced
by `.5` (`tests/ADV3000T.txt`), so the two take the same branches; the shipped
game uses ordinary randomness.

Details taken from the machine rather than guessed: numbers print in a field
padded to a multiple of three columns (at least six, twelve for a fraction,
fifteen in exponent form); a PRINT item that does not fit in the 73-column
line starts a new one; `ASSIGN` returns 3 for a missing file and `CREATE` 1
for one that exists; `INPUT` prompts with `?` and no blank, or with the text
given to it; `DAT$` reads `WED, SEP  9, 2026,  3:37 PM`.

## The fixes

`--no-fixes` plays the listing as printed. By default four things are
corrected, and every one of them is written down in
`../transcription/fixes.txt` with the reason:

* three of the author's own bugs — drinking the water cleared the oil instead,
  eating the food emptied your bottle, and smashing the ming vase removed the
  velvet pillow from the game rather than the vase;
* one data record where the magazine's print of `ADESCRIP` room 71 reads
  `You'reein the giant room.`

The author's misspellings are left alone in both variants: they are his
(`overlloking`, `litle`, `fierece`, `tweleve`, `dessend`, `stiflingg`,
`imbedded`, `"fee fie foe foo" [sic].`).
