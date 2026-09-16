# THE LABYRINTH (Michael Wei, PLATO / CYBIS, 1980–1996)

*"The LABYRINTH is a constantly evolving dungeon game."*

A multi-player dungeon game written in TUTOR by Michael Wei ("©1980 by Randy
Harmelink & Michael Wei" on the title page). It grew over fifteen years and many
maintainers:
- Dave Small's two-lesson "MinnA version", 1981
- "Labyrinth has been recovered (obviously)", Jack Guinnip, 4/23/93
- Bob Hubel's edits, as late as April 1996

The game has a castle with shops and a casino, and characters of eleven races. Below
it is a dungeon of 14×14 modules, drawn as a line-drawn 3D corridor view. You meet
monsters, fight them or charm them, find treasure, potions and scrolls, and cast
spells.

This folder holds **two playable versions**, both from the NOS 2.8.7 CYBIS pack
`DQ23_PUB0`:

| Program | Lessons | Common (world and players) |
| --- | --- | --- |
| `Labyrinth.exe`: the version as last configured | `labyrinth` (entry) → `exp` (castle) → `khazaddum` ("Khazad-dûm, the Mansion of the Dwarves", the dungeon). Also `operate`, the use file with the define sets and shared routines, and `helpless`, the help lesson | `operate,krozaircom`, 7,600 words |
| `old\LabyrinthOld.exe`: "(OLD VERSION)" | `olabyrinth` → `odungeon`, with `fenhollen` (use file), `hyborea` (data file: casino, character set, lineset) and `helpless` | `hyborea,dungeoncom`, 6,400 words |

The routing between lessons is the game's own. `operate`'s defines name `exp` and
`khazaddum` as the drivers; `fenhollen`'s name `odungeon`.

Both programs contain the original lesson files, word for word, and run them on the
TUTOR interpreter shared with the other PLATO ports. The TUTOR source is not
translated into another language.

## Playing

Start `Labyrinth.exe` (or `old\LabyrinthOld.exe`) and press **NEXT** (Enter) at the
title.
- **Options:** `c` creates a character (race, sex, alignment, rolled stats, a
  pseudonym and a password), `r` runs a saved one, and `h` opens the help lesson,
  which documents every key.
- **Castle:**
  - `a`–`g` are the hostel, casino (blackjack), weapons store, magic store, monster
    shop, skills shop and supply shop.
  - In a shop, LAB lists the goods, typing a name and NEXT buys, and DATA sells.
  - `-` goes down to the dungeon.
- **Dungeon:**
  - `a`/`d` turn left/right and `x` turns around.
  - `w` moves ahead and **`W` (Shift+w) moves ahead through a door**.
  - `+`/`-` use the stairs.
  - `c` casts, `e` equips, `u` uses, `t` throws away, `r` rests, `i` gives info, `o`
    changes name or password, and `K` kicks out a charmee.
- **Meeting monsters:** the options are Shift-letters, with `M` to maul.
- **Saving:** shift-BACK (Shift+F9) saves your character and leaves.

| PLATO key | PC key |
| --- | --- |
| NEXT / shift-NEXT | Enter / Shift+Enter |
| BACK / shift-BACK | F9 / Shift+F9 |
| HELP / LAB / DATA | F1 / F3 / F2 (Shift for the shifted keys) |
| TERM | F4 |
| STOP / shift-STOP | F10 / Shift+F10 |
| ERASE | Backspace |

Characters, the world and the counters are kept in `saves\operate.common` (or
`old\saves\hyborea.common`) next to the program.

## What was found in the game

- **Characters:** the common still holds the characters of the game's last CYBIS
  system. The main version has Dog, bilbo, Chris, Smarty, b, Firehawk and Surtur;
  the old version has Firehawk. Their passwords are unknown, so they can't be run,
  but they are part of the world and its user lists.
- **Visitor counts:** the title page shows 1332 total users for the main version and
  27 for the old one, as the game counted them.
- **Messages:** the message of the day from Jack Guinnip (4/23/93) is shown as he
  left it.

## Differences from the original

- **The notesfile** (`n` on the options page) was a PLATO group notes file
  (`tavern`/`otavern`), not a lesson. It isn't available, so `n` does nothing.
- **The editor** (`e`, in `operate`/`fenhollen`) is offered only to the game's
  directors, named in the lesson ("jack guinn" of izcoserv), so it stays hidden.
- **You are always alone.** The multi-player parts, such as messages and user lists,
  work, but nobody else is in the game.
- **Condense errors in the original are kept.** A few statements in the source
  contain errors that CYBIS's condensor rejected, for example `calcs args = 0, arg6
  « 1,,` in `uplev`, where `args` should be `arg6`. Such a statement never ran on
  CYBIS and doesn't run here. `plato_dev --trace` lists them.
- **The © sign** on the title is blank, as in the other ports.

## Making it run

The Labyrinth needed a large part of CYBIS TUTOR in the interpreter:
- `use` blocks from the lesson's use file (header word 39), with one `define`
  spanning a dozen included blocks
- `commonx`, and `comload` windows onto a 7,600-word common
- `sort common`, `find` and `from`
- colour (`color display;…` and «c,…»)
- names with capitals, and `[ ]`/`{ }` as parentheses
- automatic and typed unit locals
- `jumpout (expression)` and `jumpout lesson,unit`
- `pack`/`packc` with embeds
- two-row pictures built with access-shift-sub/super
- linesets

**Linesets.** The line-drawn characters for sized alternate-font text are not
documented anywhere. Their layout was reverse-engineered from `dunglines`. Words
11 onward hold 15-bit start offsets (the data begins one word earlier than the
entry). The characters are 15-bit groups, either points (7-bit signed x,y) or
commands (end, pen up, advance). Decoded this way they draw the blackletter
"Labyrinth" title and the corridor wall pieces of the 3D view.

## Source and build

`..\src_original\` holds the nine lesson files as `.words` (every word, read by
following the NOS track chain) and decoded `.txt` listings. The engine is shared
with the other PLATO ports, in `D:\tools\IFBackup\_PLATO_work\engine`. `build.bat`
rebuilds both programs.
