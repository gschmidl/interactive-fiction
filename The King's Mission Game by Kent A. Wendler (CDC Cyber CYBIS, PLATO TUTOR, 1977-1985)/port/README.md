# THE KING'S MISSION GAME (Kent A. Wendler, PLATO / CYBIS)

© 1977 Kent A. Wendler, © 1981 University of Illinois Board of Trustees. The game
was written in TUTOR at the University of Illinois (AISS/LCS) and later distributed
in Control Data's CYBIS "Authors Library". This copy is lesson `2tkm` with its
companion `2tkmb`, created 09/02/80 and last edited 09/19/85.

The King sends you on a mission: slay every goblin, zombie, werewolf or demon in one
of his enchanted realms. A realm is a square of 1 to 10 sections on a side, and each
section is a 10×10 field of trees, treasure chests, living pyramids and monsters. As
your rating grows, the missions get harder.

This is a native Windows build. `KingsMission.exe` contains both original lesson
files and the game's dataset, word for word as they sit on the NOS 2.8.7 CYBIS pack,
and runs them on the TUTOR interpreter shared with the other PLATO ports. The TUTOR
source is not translated into another language.

## Playing

Double-click `KingsMission.exe`.
1. Press NEXT (Enter) past the Authors Library notice.
2. On the title page:

| Key | Action |
| --- | --- |
| NEXT (Enter) | Receive a mission |
| DATA (F2) | The King's Roster |
| SHIFT-DATA (Shift+F2) | The King's Statistics |
| HELP (F1) | The King's Scribe: seven chapters of rules |
| LAB (F3) | The Honor Roll |
| BACK (F9) | Leave |

Tell the Gatekeeper your name, then whisper a secret name twice. Nothing is shown
while you type it. Choose a realm (1–10).

On a mission, one key per move:

| Key | Action |
| --- | --- |
| `q` `w` `e` / `a` `d` / `z` `x` `c` | Strike with your sword NW N NE / W E / SW S SE |
| `s` | Shoot an arrow. Give a clock direction: 12 or 0 is north, 3 east |
| `m` | Change bearing (clock direction) and speed (0–3) |
| `T` (Shift+t) | Pick up an adjacent treasure chest |
| `p` | Ask an adjacent pyramid for help: buy strength, arrows or a magic sword, or end a completed mission |
| NEXT (Enter) | Let a turn pass |
| HELP (F1) | The list of commands |
| DATA (F2) | Redraw the section |
| TERM (F4) | Spells: `s` / `see` redraws the character set; `p` / `paratime` and `n` / `normal` |
| STOP (F10) | Suspend the mission. Enter your name again later to resume it |
| shift-STOP (Shift+F10) | Leave at once. The King's laws about abandoned missions apply |

After a rejected answer, press Enter (or Backspace) to clear it, as on PLATO.

## What is saved

These go in `saves\` next to the exe:
- **`2tkmb.common`:** the King's Roster (60 adventurers), the Honor Roll, the
  Boldest Adventurer, and the King's Statistics.
- **`2tkmds.dataset`:** suspended missions.

The statistics were found in the lesson common as the game left them on its last
CYBIS system: 4,169 missions, 2,295 of them completed and 1,450 ending in the
adventurer's death. Your own missions are added to them.

The roster itself is empty. The game's documentation says "all player records are
cleared upon intersystem transfer", and that happened before the pack was made. The
dataset still holds records of missions suspended on that system, but their adventurers
are no longer on the roster, so nobody can resume them.

## How faithful it is

`2tkm` was the first CYBIS-era lesson on this interpreter, so the engine gained the
later TUTOR language to run it:
- `if`/`elseif`/`else` and `loop`/`outloop`/`reloop`, with "." indentation
- unit-local variables
- `helpop` and `termop` units that run on the same display
- `finish` units
- `getcode`
- `force micro` with the game's `keykill` micro table
- `time` and TIMEUP
- `segmentv` and `segmentf`
- datasets with 150-word records
- `jumpout` between the two lessons, and a common shared with the help lesson

Known differences:
- **The © signs** on the title page are blank. The terminal fonts available here
  have no glyph for PLATO's access-shift-c.
- **The paratime and normal-time spells** (`backgnd`/`foregnd`) do nothing. On CYBIS
  they moved the lesson between processing queues.
- **Only one player is ever signed in.** The "Kingdom at maximum capacity" check never
  fires, and the cleanup of records left by crashed sessions runs on every entry, as
  it would on a quiet system.
- **Pauses keep their original lengths.** A collision message still holds the screen
  for several seconds, just as it did on a PLATO terminal.
- **An abandoned mission always drops you from the roster, a bug in the original.**
  The finish unit `mexit` should keep you if no monsters were near you when you
  left. That test names a variable, `backout`, that the lesson never defines. On
  CYBIS such a statement fails to condense and is skipped, and so it is here.

## Source and build

- `..\src_original\2tkm.words`: the main lesson, 28 blocks × 320 words.
- `..\src_original\2tkmb.words`: the companion lesson, 14 blocks. It holds the help
  chapters, the character set `tkmchars`, the roster common `tkmers`, and the micro
  table `keykill`.
- `..\src_original\2tkmds.dataset`: the dataset, 35 blocks × 320 words. It is a
  320-word header followed by 60 records of 150 words.
- All three files come from `DQ24_PUB1` (sectors 643466, 643606 and 643676). The
  `.txt` files are decoded listings.
- The engine is shared with the other PLATO ports, in
  `D:\tools\IFBackup\_PLATO_work\engine`. `build.bat` regenerates `src\game_data.c`
  and relinks.
