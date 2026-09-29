# QUEST (Gary Stollman, PLATO, 1980, unfinished)

A student's Adventure-style game, written in TUTOR for the PLATO system at Florida
State University. It lived in lesson `ciswork18`, the "class work space" of course
CIS4932. The lesson header reads:

> cis4932 lesson space · anybody who cares · class work space so i can be as good a
> programmer as jd

Stollman created the lesson on 04/09/80 and last edited it on 05/31/80. The visitor
counter was reset on 05/27/80. He never finished the game.

This is a native Windows build. `Quest.exe` contains the original lesson file, word
for word as it sits on the NOS 1.3 PLATO disk. It runs that file on the same TUTOR
interpreter and PLATO terminal as the Pepke *Adventure* port. The TUTOR source is not
translated into another language.

## Playing

Double-click `Quest.exe`.
- **Title page:** press **Enter** (PLATO's NEXT) to start, or F1 for the help page.
- **Commands:** type them at the arrow and press Enter. Every command is judged
  "no", so press **Enter** (or Backspace) once to clear it before typing the next.
  This is how the lesson was written.
- **Directions:** n s e w u d (or spelled out), "in", "out".
- **Other verbs:** read writing, lift mat, get mat, get keys, open door, knock door,
  kick door, enter house, get ladder.

| PLATO key | PC key |
| --- | --- |
| NEXT | Enter |
| ERASE | Backspace |
| HELP | F1 or Ctrl+H |
| BACK (leave the help page) | F9 or Ctrl+B |
| STOP / shift-STOP (leave) | F10 / Shift+F10 |

The visitor count ("You are the 116 person to use this lesson since 05/27/80")
comes from the lesson's own common storage, as it was found on the disk. It goes up
by one on every start, and is kept in `saves\ciswork18.common` next to the exe.

## What there is

Twelve locations, ten of them reachable:
- **Outside:** the front of the little house, the cliff edge (west), the drainage
  ditch (east) with a tunnel beyond it (north), and the gully (down).
- **Inside:** the hall, the library, the study with its fireplace, upstairs, a bare
  room with a closet door ("This is not a closet."), and the bedroom.

The help page promises a cave entrance that "will not be as easy as you think", and
says it "will be expanded in the near future". Neither the cave nor the rest of the
game was ever written.

These are bugs in Stollman's own code, and they are kept as they are:
- **Moving outside:** until the door is kicked in, every move from the front of the
  house answers "The door is locked!", because the door test comes before the
  direction test.
- **Keys:** they can be found and taken, but "none of the keys seems to work".
- **The closet:** `unit closet` and `unit closeit` exist, but nothing leads to them.
- **The tunnel:** it doesn't record your location, so you are still in the ditch as
  far as movement goes.
- **The ladder:** "get ladder" works anywhere, as often as you like. The check uses a
  variable, `gotlad`, that was never defined.
- **The counter's ordinal suffix:** the lists are off by one, so it usually prints
  none ("the 116 person").

## Source and build

- `..\src_original\ciswork18.words`: the lesson file, 14 blocks × 320 words. Its
  header is at sector 2092 of `DD844_C02u2`, and the read follows the NOS track
  chain.
- `..\src_original\ciswork18.txt`: decoded listing.
- The engine is shared with the other PLATO ports and is not published here. `build.bat` regenerates `src\game_data.c`
  and relinks.
