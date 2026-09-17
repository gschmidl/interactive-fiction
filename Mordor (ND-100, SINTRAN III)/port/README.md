# Mordor — Norsk Data ND-100, SINTRAN III — native Windows port

    mordor.exe --unlimited

`mordor.exe` is an ND-100 minicomputer with just enough SINTRAN III in it to
run one program: **Mordor: Land of evil, v7.52 850211**, Mikael Johansson's
ND-Pascal game for the Swedish computer club DNF.  The source was recovered
from the free pages of one of the club's floppies, where it had been deleted,
and compiled with the club's own ND-Pascal compiler.  What runs is that
compiled program, instruction for instruction, on an emulated CPU, with three
of the game's bugs fixed in its source: the names no one could type, a
fight that could go on for ever, and a hero screen that showed names no one
could reach (see *The port's fixes* below).

                        -  Mordor: Land of evil.  v7.52 850211  -

    Do you want instructions (Y/N) <N>:

## The game

It is 12 April 3019 of the Third Age and you have the Ring.  Choose heroes of
the West (the game suggests 4-12, but one will do), pick a Ringbearer and someone
to carry the Palantir, and get the Ring to Mount Doom through a Middle Earth
of 128 by 128 squares.  The map wraps round east to west; you start near its
southern edge, and Mount Doom and Barad-dûr lie far to the north.  Orcs, wolves
and wargs, trolls, Haradrim, four Balrogs and the nine Nazgul roam it.  Gollum
hunts the Ring, the Ring works on whoever carries it, and the citadels (Minas
Tirith, Dale, Rivendell, Lothlorien, Edoras) fall one by one while you
travel.  At the end the game scores you and gives its verdict, from "not fit
to shine Gollums shoes" to "The West is open to you!".

Every turn you get 5 movement points and a view of the land around you.  It
shrinks at night (every fourth turn) and in the mist, and grows while the Ring
is on.

| command | |
| --- | --- |
| `N` `S` `E` `W` `NE` `NW` `SE` `SW` | move; add a number of steps (`NE2`) or `+` for as far as you can (`W+`) |
| `0` | end the turn |
| `FOLLOW-ROAD` | follow a road north |
| `ON`, `OFF` | put the Ring on (stronger, sees further) or try to take it off |
| `THROW` | at Mount Doom, with the Ring off: throw it in |
| `USE` | the Palantir: how far to Orodruin, and where Gollum is if he has the Ring |
| `ASK`, `TIE`, `UNTIE`, `KILL` | Gollum, once caught |
| `MAP` | look again |
| `REPORT` | who is left, and how the citadels are doing |
| `HELP` | the commands and map symbols |
| `QUIT-GAME` | give up ("Chicken!") |

Commands can be shortened as long as they stay unambiguous.  In a fight the
game asks whether to fight together or individually and whether to fight or
run.

The map signs are `:` rough, `.` grass, `T` trees, `&` swamp (only with Gollum
untied), `M` mountain, `"` river, `-` road, `=` main road, `+` ford, `I` tower,
`D` Mount Doom and `B` Barad-dûr.  Next to them: `o` orcs, `w` wolves, `b`
Balrog, `n` Nazgul, `h` Haradrim, `t` trolls, `g` Gollum, `F` your fellowship,
`?` the traitor.

### Choosing heroes

The hero screen is driven with the **arrow keys**, and **Home** takes a hero
in or out (shown in reverse video).  Put the cursor on *Finished.* below the
list to go on: Down from Erkenbrand, the first name of the bottom row, or Right
from Denethor, the last.  The game goes on at once, so there is no coming
back; arrow keys struck after that are ignored.

### Names

When the game asks for a name (the Ringbearer, the Palantir's carrier, who
fights, who takes over) type it as it is spelt, in any case: `frodo`, `Sam`,
`ARAGORN`.  A beginning is enough when no other hero starts the same way
(`fro`, `ara`).  Éomer and Éowyn can be typed with É or a plain E, and
Theodréd (so spelt) as `theodred`.

### The instructions

Answering `Y` shows the instructions, twelve pages of them, with Return to
turn the page.  They are the ones written for the previous version, v7.1
(1982), which survive on another of the club's floppies; v7.52's own were
lost.  They fit the game well, with a few differences: they do not mention
`USE` (the Palantir), they place Imrahil at Dol Amroth, and v7.52 lets you take
more than 12 heroes.

## The port's fixes

v7.52 as the club compiled it had three bugs that the port fixes in the Pascal
source (`pascal\`), which was then compiled again with the club's compiler:

- **Names had to be typed shifted.**  The function that compares names
  subtracted 30 instead of 32 from a letter, so Frodo had to be typed `FTQ`
  and Sam `SC`.  Now names are typed as spelt, in any case; commands and
  answers can be typed in lower case too.
- **A fight could go on for ever**, with nothing on the screen.  If the
  Ring-bearer escaped from capture on his own and you then put the Ring on,
  the game counted him into the fellowship twice.  When the last man standing
  was captured, it kept looking for someone to fight.  The count is now
  kept right.
- **Names past *Finished.* on the hero screen.**  v7.52 added eight people
  who stay in the citadels (Hama, Ceorl, Orophin, Galadriel, Éowyn, Elladan,
  Ellrohir, Grimbeorn: the defenders the game counts when a citadel is
  attacked, with no strength of their own) and drew them on the hero screen
  too, one under *Finished.* and seven after it, where the cursor could never
  go.  Now the screen shows the 32 heroes one can choose, as v7.1 did.

`mordor --prog build\original\MORDOR-MJ.PROG` plays the program compiled
from the recovered source unchanged, bugs and all.

Two more, in the port rather than the game.  **Backspace rubs a character
out**: the game cannot do it, but SINTRAN's terminal driver could and did,
and the port had been passing the key into the command instead, where it
spoiled it out of sight (`--no-rubout` leaves it to the game).  And **Esc no
longer loses the game**: it still breaks to SINTRAN's `@`, as it did, but
CONTINUE there goes back into the game where it stood — the real machine
started it again from the title.  Both are in NOTES.md, with what the
reference machine does.

## Office hours

MORDOR will not start between 08:00 and 16:00; it says *You cannot play
MORDOR now!* and stops.  That was the club machine's working day.  Players
got round it by typing DEL just as the program started, and so does
**`-u`, `--unlimited`**.

## The map is kept

The first game builds Middle Earth and saves it in `data\MORDOR-MAP-MJ.DATA`.
Every later game plays on the same land, with the towers, Mount Doom,
Barad-dûr and the monsters placed afresh, as at the club.  `--new-map` makes
the next game build a new one.

## Keys

| key | effect |
| --- | --- |
| letters, digits | as typed; SINTRAN echoes them |
| Enter | ends the line |
| Backspace | rubs the last character out, as SINTRAN's delete keys (DEL and Ctrl-A) did; Ctrl-Q rubs out the whole line.  The game itself has no rubbing out: this is the terminal driver's, and `--no-rubout` leaves it to the game |
| arrows, Home | the hero screen (sent as the Facit terminal's `ESC A`-`D`, `ESC H`); anywhere else they are ignored, as their ESC would break the game |
| É é | the Swedish `@` and `` ` `` (Ä Ö Å ä ö å are `[ \ ] { \| }`) |
| Esc | SINTRAN *user break*: the program stops at once, as on the real machine, even in the middle of a turn (not on the hero screen, where the game switches it off).  It leaves you at SINTRAN's `@`, where **CONTINUE** goes back into the game and LOGOUT ends it |

## Options

| option | meaning |
| --- | --- |
| `-u`, `--unlimited` | play between 08 and 16 (types DEL ahead of the program) |
| `--new-map` | start with an empty map file: the game builds a new Middle Earth |
| `--no-rubout` | do not rub characters out while typing (the game cannot do it itself) |
| `-d`, `--data DIR` | the game's files (default: `data\` next to the program) |
| `--swedish` | show the national characters as Ä Ö Å ä ö å É é (default) |
| `--ascii`, `--norwegian` | show them as `[ \ ] { \| } @ \``, or as Æ Ø Å æ ø å |
| `--raw` | pass the terminal bytes through untranslated (the tests use this) |
| `-Z`, `--clock SECONDS` | fix the clock at SECONDS since 1970; the random numbers no longer stir in the time, so the same typing plays the same game |
| `--prog FILE` | run another one-bank `:PROG` file |
| `-v`, `--verbose` | log monitor calls on standard error |
| `-T`, `--trace` | trace every instruction on standard error |
| `-h`, `--help`, `--version` | |

Started from Explorer, the window stays open at the end with
`[press any key]`, so the final score does not vanish with it.

## Files

| | |
| --- | --- |
| `mordor.exe` | the port (`make`, gcc) |
| `data\MORDOR-MJ.PROG` | the program, as ND-Pascal J and the ND loader made it from `pascal\MORDOR-MJ.SYMB` |
| `data\MORDOR-RULES-MJ.DATA` | the instructions of v7.1 |
| `data\PASCAL-ERR.SYMB` | ND-Pascal's error messages, from the club's disk |
| `data\MORDOR-MAP-MJ.DATA` | your Middle Earth, made by the first game |
| `pascal\` | the source with the port's fixes, and `fixes.diff` |
| `..\src_original\MORDOR-MJ.SYMB` | the recovered source; `other-versions\` holds v7.1 and Magnus Domellöf's BASIC original |
| `build\` | the compiler's listing and object file; `build\original\` the same, and the program, for the source as recovered |
| `src\` | the emulator (shared with the Skattejakt and SVHA ports) |
| `tests\run.py` | replays games recorded on SINTRAN III, with both programs, and compares every byte |
| `tests\consoleplay.py` | plays through a real Windows console |
| `tools\` | the random player, the recording drivers for the reference machine, `portfuzz.py` |

How the source was recovered, how it was compiled, what the fixes change, and
what SINTRAN III the program needs: `NOTES.md`.
