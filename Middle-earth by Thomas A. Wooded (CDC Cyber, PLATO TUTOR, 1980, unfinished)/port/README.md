# MIDDLE-EARTH (Thomas A. Wooded, PLATO, 1980, unfinished)

*"A game of travel and adventure based on the works of J.R.R. Tolkien"*, created by
Thomas A. Wooded (he signs his notes "tom wooded"). He wrote it in TUTOR for the PLATO
system at Florida State University, in lesson `midearth`. The lesson header says it
"contains two games: 1. Space Invasion & 2. Middle-earth". He created it on 03/20/80
and last edited it on 05/29/80.

Neither game was finished. This is a native Windows build. `MiddleEarth.exe`
contains the original lesson file, word for word as it sits on the NOS 1.3 PLATO
disk, including its two character sets, and runs it on the same TUTOR interpreter
and PLATO terminal as the other PLATO ports. The TUTOR source is not translated into
another language.

## Playing

Double-click `MiddleEarth.exe`. Press NEXT (Enter) past "PLATO is feeding the
dragons!" to reach the title page: the Middle-earth title, a wizard drawn in the
lesson's `runes` character set, and a menu.

| Key | On the title page |
| --- | --- |
| NEXT (Enter) | Choose a character |
| LAB (F3) | Map of Middle-earth |
| SHIFT-LAB (Shift+F3) | Space Invasion |
| HELP (F1) | Help section |
| DATA (F2) | The latest Tolkien quiz |

| PLATO key | PC key |
| --- | --- |
| NEXT / shift-NEXT | Enter / Shift+Enter |
| ERASE | Backspace |
| HELP / LAB / DATA | F1 / F3 / F2 (Shift for HELP1, LAB1, DATA1) |
| BACK | F9 or Ctrl+B |
| shift-STOP (leave) | Shift+F10 |

## What there is

- **Choose a character:** only *a) Wizard* was written.
  - It rolls random strength, mobility, hit points, power, intelligence, luck, magic,
    money, food and water, next to the maxima. LAB re-rolls.
  - Every other type says "This option not currently available. Please press BACK."
  - There is no game past the roll.
- **Map:** the western coastline of Middle-earth, plotted point by point and "adapted
  from *The Lord of the Rings* (Ballantine Books: 1965)". It has a scale in miles,
  a north arrow and the first strokes of a river. The map says "Press BACK for now
  please!"
- **Help:** fifteen topics are listed, but only *1) Character types* exists, and in it
  only *a) Wizards* has a page. The other topics lead nowhere.
- **Quiz (DATA):** Wooded's Tolkien quiz of April 25, 1980. NEXT shows the April 24
  quiz, with its answers. The screen says the April 25 answers "will appear soon".
  They are in the source, as comments.
- **Space Invasion:** "undergoing major construction and re-working". Wooded's draft
  sits between `cstop` and `cstart`, so PLATO never compiled it. It is kept in the
  source listing but cannot run, here or on PLATO.

## Notes on the original

- **The roller screen is garbled as written.** The rolled values are written in
  `mode rewrite`. Their continuation lines begin with blanks, and blanks in rewrite
  mode erase what is under them, so they wipe out most of the "Maximum:" column.
  PLATO would have shown the same: a continued write returns to the margin set by
  the `at`, and `showt` pads to a fixed field.
- **Pressing BACK at the roller:** it takes two presses. BACK is armed only after the
  pause, and the first press just ends the pause.
- **Help topics 2–15:** they jump to units that don't exist. As with any command that
  failed to condense, nothing happens.
- **The source listing** also keeps, outside the condensed code, an access-list
  scheme with `jumpout`, and a numbered list of magical spells signed "radgast/bcc501".

## Source and build

- `..\src_original\midearth.words`: the lesson file, 14 blocks × 320 words. Its
  header is at sector 7926 of `DD844_C02u2`, and the read follows the NOS track
  chain.
- `..\src_original\midearth.txt`: decoded listing.
- The engine is shared with the other PLATO ports, in
  `D:\tools\IFBackup\_PLATO_work\engine`. `build.bat` regenerates `src\game_data.c`
  and relinks.
