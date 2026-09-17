# ADVENTURE (Eric Pepke, PLATO, 1979–80)

*"Ninnies and Cretins: a variation on Adventure"*, version 2.2, © 1980 Eric Pepke.
Written in TUTOR for the PLATO system at Florida State University. It ran as
lesson `adventure` in master file EMAST.

This is a native Windows build. `Adventure.exe` contains Pepke's original lesson
file, word for word as it sits on the NOS 1.3 PLATO disk. It runs that file on a
TUTOR interpreter with a PLATO terminal built in. The TUTOR source is not
translated into another language.

## Playing

Double-click `Adventure.exe`. Type commands at the arrow and press **Enter** (PLATO's NEXT).

| PLATO key | PC key |
| --- | --- |
| NEXT / shift-NEXT | Enter / Shift+Enter |
| ERASE / shift-ERASE | Backspace / Shift+Backspace |
| HELP | F1 or Ctrl+H |
| DATA | F2 or Ctrl+D |
| LAB | F3 or Ctrl+L |
| TERM | F4 or Ctrl+T |
| MICRO | F6 or Ctrl+M |
| BACK | F9 or Ctrl+B |
| STOP / shift-STOP (leave) | F10 / Shift+F10 |

- **Commands:** the parser takes whole sentences ("get the lamp and light it", "open
  the doors with the keys"). HELP at the arrow lists the simple commands.
- **MICRO shortcuts:** MICRO followed by a letter types a stock phrase. MICRO then
  `0` is "in,get all,in".
- **Rejected commands:** start typing and the complaint erases itself.
- **Yes/no prompts:** after "YES OR NO?", press Enter first to erase your answer.
- **Saving:** "quit" offers to save the game. DATA on the title screen resumes it.
  Saves are keyed by your PLATO name, which is taken from your Windows user name.
  Use `Adventure.exe --name someone` to change it.
- **TERM `games`:** lists the saved games. It still shows the 1980 FSU players
  (dave/bio2, ginny/facmelb, …), because the lesson's common storage is used as it
  was found.

Saved state goes in `saves\adventure.common` next to the exe.

## What had to be rebuilt

The version on the disk loads its rooms and map from a dataset, `adventds`. That
dataset lived in master file CMAST. Every surviving copy of CMAST is an empty
re-creation from 1989: the one on the NOS 1.3 packs, and the one on the
`plato4nos13` PFDUMP tape they were restored from. The dataset is gone.

An older version of the game kept the same data inside the lesson, and the
editor never overwrote those blocks:

| Data | Where it survived | Coverage |
| --- | --- | --- |
| Short room names | unused block 16(8), a WRITEC list | all 48 |
| Long descriptions | unused block 15(8), and the slack after block `describe` | rooms 3–10 and 30–48; room 2 partly |
| Map | lesson common `info`, nc1–nc96 (2 words per room, 6 nine-bit fields per word) | all 48 |

`data\adventds.dataset` is rebuilt in the record layout that `unit getloc`
reads: 8 records of 6 rooms, 52 words per room (2 travel words as 10-bit
segments, then 8 lines of 6 words each).

- **Rooms 1 and 11–29:** the long description is lost, so the room's own short
  name is shown in its place. No prose was invented.
- **Room 2:** shows its short name plus its surviving second sentence.

The map agrees with every surviving description. The recovery is documented in
`..\src_original\adventure_rooms_recovered.txt`, and `tools\adventure_data.py` in the
shared PLATO working tree rebuilds the dataset from the lesson file.

## How faithful it is

The interpreter follows TUTOR's execution model as PLATO's own AIDS lessons
describe it:
- regular, judge and search states
- the arrow and specs markers
- `join` executing in all three states
- `judge continue` / `ignore` / `noquit`
- `force firsterase`
- automatic erasing of feedback after a "no"
- comments written three lines below the arrow

Characters use the PLATO ROM font, rasterised from DtCyber's pixel-exact
PlatoAscii font. The runic parchment and the NEXT/HELP/DATA key caps come from
the lesson's own character set `letters`. MICRO uses its micro table
`lazytwits`.

Known approximations:
- **Spelling tolerance (`specs okspell`):** approximated. One edit is allowed, only
  on words of five or more letters with the same first letter.
- **Line-drawn text (size ≠ 0):** drawn by connecting the ROM font's dots, not from
  PLATO's own vector character set.
- **The © before "1980":** blank, because `letters` never defines those two cells.

## Source and build

- `..\src_original\adventure.words`: the lesson file, 28 blocks × 320 words, read
  from `DD844_C02u2` starting at sector 46631. The read follows the NOS track
  chain; it matches the PFDUMP tape copy.
- `..\src_original\adventure.txt`: decoded listing.
- The engine is shared with the other PLATO ports and is not published here. `build.bat` regenerates `src\game_data.c`
  and relinks.
