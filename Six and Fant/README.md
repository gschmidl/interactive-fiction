# six.py – an interpreter for SIX/FANT worlds (`*.6`)

Runs SIX source files directly, following the grammar in *Doc.txt*
("The SIX/FANT System"). It does not emulate the FANT machine and cannot read
compiled `.f` world files.

Needs Python 3.8+ (standard library only). Tested on Linux; written to be
portable (CRLF source and input files, Windows console, Ctrl+C).

**`six.exe`** is the same interpreter as a native Windows program (66 KB,
no Python needed; `six.c` is its source, a statement-for-statement
translation of six.py). Same options and the same output byte for byte,
including the numbers `?` gives with `--seed`. Build it with

    gcc -O2 -s -o six.exe six.c -Wl,--stack,268435456

In a folder with both, `six adventure.6` runs `six.exe`. The one known
difference: six.py's own stack gives out between 7000 and 8000 nested SIX
calls (and it crashes outright near 20000), while six.exe goes on to the
`--max-depth` limit, 20000 by default.

## Usage

    six adventure.6                 (Windows: six.bat)
    python six.py this.6 --fix-typos
    python six.py world.6 < commands.txt --echo
    python six.py world.6 --check   (parse only, list problems)

| Option | Meaning |
|---|---|
| `--width N` | wrap `output` at N columns (default 79, 0 = never) |
| `--seed N` | seed for `?` (random 0..9999) |
| `--echo` | echo input lines (handy when input is piped) |
| `--prompt TEXT` | print TEXT before every `input` |
| `--input FILE` | read `input` lines from FILE |
| `--check` | only parse; report warnings |
| `--fix-typos` | resolve an undeclared word to the one declared name it is a near-miss of |
| `--strict` | undeclared words and questionable operations are errors |
| `--warn` | report tolerated problems on stderr |
| `--quiet` | suppress the startup note about tolerated problems |

## What is implemented

Everything in the grammar: `cons`, `var`, `thing`, `proc` (proper and
`result` procedures, forward declarations), `verb`/`noun`, `start`;
`for`/`while`/`if` (statement and expression, with block expressions),
`:=`, `<+ <++ <-`, `--`, `.` / `..`, `$`, `#`, `length`, substrings, `is`/`isnt`,
`in`, `and`/`or` (short-circuit), `?`, `input`, `csid/project/time/date`,
nested comments, the string-break rule, 24-bit integers, and `output`
word-wrapping. `dict` holds every thing/verb name.

Lists are real linked lists: `for w in L do L <- w od` (deleting the element
being visited) works as it did on the original machine.

## Judgement calls (the documentation doesn't settle these)

* `output` wraps at 79 columns; blank runs at a break are dropped, leading
  blanks are kept, trailing blanks are trimmed.
* `absent` counts as false; `<+` does not remove duplicates.
* `mts` (run an MTS command) is reported on stderr and ignored.
* Output of nil/absent/non-printable values prints nothing (error with `--strict`).
* `in` on a non-list gives "not a member" (error with `--strict`).
* Undeclared words become new unique properties (as the grammar says for
  unused table indices).

## Problems in the sample worlds

* Unmatched `fi` (adventure.6 line 3081, this.6 line 1580): ignored.
* Doubled comma in an `output` list (this.6 line 6195): ignored.
* `this.6` uses three undeclared words that are typos of declared names:
  `_badegeon` (line 1995), `_dugout` (4908), `weight` (7000, should be
  `_weight`). Without `--fix-typos`, the `weight` typo makes `get` fail
  for every object, so use `--fix-typos` to play `this.6`.
* `this.6` tests `submarine in _hung` on a boolean.
* In adventure.6, `throw dwarves` puts an object without a `LONG` entry in the
  room.

## Tests

    python tests/test_six.py
