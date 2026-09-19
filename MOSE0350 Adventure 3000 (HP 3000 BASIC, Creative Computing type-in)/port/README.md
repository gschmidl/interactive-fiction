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
adventure3000 --bell       ring the bell at every prompt, as the original did
adventure3000 --help       interpreter options (it will run any BASIC/3000 program)
```

Build with `make`, or `build.cmd` on Windows; the only dependency is a C99
compiler.

The parser is the author's two-word one: it looks for keywords anywhere in
what you type and ignores the rest, so `unlock`, `unlock grate` and
`unlock it` are the same command, and several commands can be typed at once,
separated by full stops (`take lamp. w. s.`).

## What is in here

| path | what it is |
| --- | --- |
| `basic/fixed/ADV3000.txt` | the program, with the corrections listed in `../transcription/fixes.txt` |
| `basic/print/ADV3000.txt` | the program exactly as the magazine printed it (`--no-fixes`) |
| `data/fixed/`, `data/print/` | the four data files as text, one line per record; the interpreter turns them into BASIC data files on first run |
| `src/hp3kbasic.c` | the interpreter |
| `tests/` | four walkthroughs recorded on a real HP 3000, and the script that replays them |

The first run builds `AITEMS`, `ADESCRIP`, `AMESSAGE` and `AMOVING` next to the
text records, plus an empty `BUG` file for the game's gripe command. Saved
games are written there too, as the game's own `CREATE`/`ASSIGN` do.

## How closely it follows the original

The game was first entered into the real thing: a Series 58 running MPE V/E
under SIMH, BASIC/3000 HP32101B.00.26. All 1,003 statements were accepted
without a single syntax error, the four data files were built there and
checked record by record, and the game was played.

Four walkthroughs recorded on that machine are in `tests/`, replayed by
`tests/run_tests.sh` against the same programs here. All four come out
**identical, line for line**, including the inventory's column layout, the
score line's number formatting, `take all`/`drop all`, and a game saved to a
file:

```
walkthrough b (ADV3000T): matches the HP 3000 (100 lines)
walkthrough c (ADV3000T): matches the HP 3000 (138 lines)
walkthrough d (ADV3000T): matches the HP 3000 (91 lines)
walkthrough e (ADV3000V): matches the HP 3000 (84 lines)
```

For the comparison both sides run copies of the program with `RND(0)` replaced
by `.5` (`tests/ADV3000T.txt`, `tests/ADV3000V.txt`), so the two take the same
branches; the game itself uses ordinary randomness.

Details taken from the machine rather than guessed: numbers print in a field
padded to a multiple of three columns (at least six, twelve for a fraction,
fifteen in exponent form); a PRINT item that does not fit in the 73-column
line starts a new one; `ASSIGN` returns 3 for a missing file and `CREATE` 1
for one that exists; `INPUT` prompts with `?` and no blank, or with the text
given to it; `DAT$` reads `WED, SEP  9, 2026,  3:37 PM`; and a `FOR` on a
variable whose loop was left open replaces only that loop, leaving loops
opened since untouched (the game's `take all` depends on it).

The game highlights its `>` prompt with the HP terminals' display enhancements
(`ESC &dB` … `ESC &d@`, inverse video) and rings the bell before it. On a
console the enhancements are shown as their ANSI equivalents and the bell is
left out (`--bell` puts it back); piped output keeps the bytes the HP 3000
sent. `HP3K_ANSI=1` or `=0` overrides the choice.

## The fixes

`--no-fixes` plays the listing as printed. By default the corrections in
`../transcription/fixes.txt` are applied, each with the reason for it:

* **the parser.** It looks for place words (rock, stairs, house, grate,
  stream, bridge, pit, road …) before it looks for a verb, and answers *What
  do you want to do with the grate?* to any sentence containing one, so
  `unlock grate`, `open grate`, `cross bridge` or `enter building` never work
  and only the bare verb does. The real HP 3000 does exactly the same. One
  added line lets the verbs that act on a place win (cross, climb, jump, look,
  enter, leave, lock, unlock, open, close); other verbs still get the
  question, since their routines only deal with things you can carry.
* **the name of a place word.** The lookup reads one name behind and the
  name list has no *steps*, so `take rock` asked *What do you want to do with
  the dwarf?* and `stairs` came back as *rock*.
* **three of the author's own bugs:** drinking the water cleared the oil
  instead, eating the food emptied your bottle, and smashing the ming vase
  removed the velvet pillow from the game rather than the vase.
* **four damaged data records:** room 71's `You'reein the giant room.`,
  `small  it.` and `soft  oom.` where the print lost a letter, and `the pii.`

The author's misspellings are left alone in both variants: they are his
(`overlloking`, `litle`, `fierece`, `tweleve`, `dessend`, `stiflingg`,
`imbedded`, `"fee fie foe foo" [sic].`).
