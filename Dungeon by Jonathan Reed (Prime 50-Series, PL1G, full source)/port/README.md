# DUNGEON (Jonathan H. Reed, Prime PL/I subset G, June 1983) - Windows console port

`dungeon.exe` plays Reed's 10x10x10 dragon crawl. Run `play.bat`. The game asks for "a wierd positive number": that
is the seed, and the same number gives the same cave as it did on the Prime.

The port is a statement-for-statement transliteration of `..\src_original\PL1G_GAMES\DUNGEON.PL1G` (`src\dungeon.c`),
plus the piece of PL/I it needs: list-directed input and output (`src\pl1io.c`). **Three recorded sessions of the
original running on real PRIMOS come out byte-identical** (`tests\runall.sh`).

## How the original was measured

The SBD003 tape in `..\archive_original` was restored under PRIMOS 23.4.Y2K.R1 on p50em
(disk pack copied to a scratch directory first): `ASSIGN MT0`,
`MAGRST`, tape unit 0, logical tape 1, and **`YES`** at "Ready to Restore:". That restored `PL1G_GAMES>DUNGEON.PL1G`
*and* `SEG_GAMES>DUNGEON.SEG`, the compiled original, which `SEG DUNGEON` runs. `tests\primesh.py` drives the system
over telnet and `tests\syncplay.py` plays the game, waiting for its prompts. (PL1G itself is not installed on this
pack, so the source cannot be recompiled there; the comparison is against the tape's own DUNGEON.SEG.)

What that settled - none of it guessable from the source:

1. **List-directed output layout.** Items go into 7-column fields starting at columns 1, 8, 15, 22, ...; an item is
   placed at the first boundary at or after the current column; arithmetic items are right justified in the six
   columns of their field (FIXED(4): precision + 2), character items are written as they stand, and one blank follows
   every item. `PUT LIST` without SKIP continues the current line, which is how the ESP map is drawn. A `PICTURE '9'`
   item is written as its one character - the gold in TAKE prints as `5      gold pieces` while the FIXED(4) total in
   ITEMS prints as `     5 gold pieces.`
2. **List-directed input.** `GET SKIP LIST` discards the rest of the line the previous item came from and skips blank
   lines entirely until it finds a token: a blank line at "Command?" is *not* an empty command (the game just waits),
   and "status extra words here" runs `status` and throws the rest away. Typing two words ("move north") therefore
   does not work: the direction prompt discards "north" and reads the next line.
3. **FLOAT rounds toward zero.** The 50-Series truncates, and the cave depends on it: `RND = MOD(SEED,100)*.01` and
   `FLOOR(RND*20)` fall on different integers under round-to-nearest, so every draw count in INIT shifts and the
   whole dungeon changes. The port sets `FE_TOWARDZERO` and computes the generator in single precision. With
   round-to-nearest (single or double) seed 42 starts the player at 1 1 1 or 6 3 1; truncating, it starts at 5 7 10,
   which is what the Prime prints, and the ESP map then matches cell for cell.
   (`tests\model.py` is the small Python model used to find this.)

## PL/I details the transliteration keeps

- A `DO ... TO expr` limit is evaluated **once**; the loops in INIT and THCORD call the generator in their bodies, so
  a C `for` re-evaluating the limit silently changes the cave.
- CHARACTER comparison pads the shorter side with blanks; `INDEX('north', item) = 1` is how every command and
  direction abbreviation works ("no" is an abbreviation of "north", which is why it answers the direction prompt).
- Assignment to CHARACTER(n) VARYING truncates: COMMAND is CHARACTER(8), which is why the source tests for
  `'checktra'`; GETNUM's NUM is CHARACTER(4), so a seed of "12345" is read as 1234.
- The global `i, u, v, x, y, z` are shared by the procedures, and GETNUM, TAKE, WAND, POTION, ATTACK, MOVE and
  RETRIEVE declare locals that shadow them - reproduced as written.
- A dangling `ELSE` belongs to the nearest `IF`: in FORGET, item number 0 or less prints nothing at all.

## What the port changes

**Fix 1** (2026-09-21, from a user's bug report). The instructions pause three times with "*** press <return> twice
to continue ***", and read the pause with `GET SKIP LIST`. On the Prime that skips blank lines until it finds a
word (point 2 above), so the Returns alone never went on; you had to type something. The port lets two blank lines
go on, as the text says, and a word still does.

    dungeon [--no-fixes] [-h]

`--no-fixes` is the program as it ran on the Prime. Unknown options are refused.

Whatever the options (refine pass, 2026-09-21): REMEMBER takes any three numbers, and the one place they are used as
subscripts is DTURN's dragon, which "knocks you in the head" and moves a remembered thing to a random empty room -
reading and clearing `m(x,y,z)` at the remembered coordinates. The PL/I had no subscript checking, so a remembered
place outside the dungeon (say 0, 99, -5) read and cleared whatever lay at that address. In the port such a place
holds nothing and nothing is cleared. The wand and potion numbers were already checked by Reed (`w > 0`, `w <
pos_sub`).

## Checked so far (first pass, 2026-09-20)

- `tests\runall.sh`: three PRIMOS sessions replay byte-identically - a 140-line session (status, ESP map, remember /
  memory / forget, hide, untrap, retrieve, move, search, take, quit), a 36-line input-semantics session (blank line,
  several words on one line), and a 63-line session that walks two rooms north to the gold the model predicted and
  takes, hides and retrieves it.
- Builds clean with gcc 15 (`-Wall -Wextra`).
- `tests\returns.py` (fix 1): two Returns at each pause reach the seed prompt, one does not, a word does too, and
  with `--no-fixes` Returns alone never do.

- Refine pass (2026-09-21): 150 random sessions of 400 lines (the game's words, numbers from -50 to 999, blank
  lines; every other one with `--no-fixes`) end cleanly; `tests\runall.sh` still identical.

## Still to do (refine pass)

- Deferred (user, 2026-09-21: bugs and fuzzing only): a dragon fight and a win through a true portal compared against
  the Prime (`drag` is FIXED(5), so "You vanquished ... dragons" uses a 7-column field - unverified), and the
  instructions text captured with a prompt-synchronised run.
