# HOBBITS (Michael Jackson, PLATO, 1980)

Michael Jackson's class work for CIS4932 at Florida State University. It was written
in TUTOR for the PLATO system and kept in lesson `ciswork29`. He created the lesson
on 04/09/80 and last edited it on 05/20/80.

The lesson opens on an index of his assignments:

| Choice | Assignment | What it is |
| --- | --- | --- |
| a. Boxes | 1 | A sheet of TUTOR's drawing commands: circle, broken circle, vector, arc, ellipse, sized writing and more |
| b. Walls | 2 | A random walk of little boxes. It runs until it paints itself into a corner, then flashes and reports "It messed up in N moves" |
| c. Hobbits | 3 | **The riddle game.** Gollum asks Bilbo five of Tolkien's riddles from *The Hobbit*, with five hints each |
| d. assignment 4 | — | "Sorry assignment 4 is not complete" |

This is a native Windows build. `Hobbits.exe` contains the original lesson file, word
for word as it sits on the NOS 1.3 PLATO disk, including its character set, and runs
it on the same TUTOR interpreter and PLATO terminal as the other PLATO ports. The
TUTOR source is not translated into another language.

## Playing

Double-click `Hobbits.exe` and press a, b, c or d.

| PLATO key | PC key |
| --- | --- |
| NEXT | Enter |
| ERASE | Backspace |
| HELP | F1 or Ctrl+H |
| BACK | F9 or Ctrl+B |
| shift-STOP (leave) | Shift+F10 |

- **Index:** BACK leaves the lesson.
- **Walls:** NEXT runs it again, BACK returns to the index.
- **The riddle game:** after a wrong answer, press **Enter** once to clear it before
  typing again. This is standard PLATO behaviour.
  - Each wrong try brings the next hint. The fifth is Gollum's "very good" hint, and
    it repeats from then on.
  - A riddle only counts once it is answered right *on the first try*. Riddles you
    needed hints for come back later, until every one has been answered first time.
  - Answers may be phrased "it is a …".
- **Do not press BACK once the game has begun.** Gollum spares you, but you are
  forbidden to play again. Each time you choose c after that, the warning gets
  sterner. The fifth time, the lesson throws you out. The ban lasts until you close
  the program, because student variables were not kept between PLATO sessions.

## Notes on the original

- **Restricted file:** the lesson turns away anyone in course `cis4932` who isn't
  Jackson. That is his classmates, not the rest of PLATO. The port runs in course
  `home`, so it opens for everyone. Start `Hobbits.exe --course cis4932` to see the
  "Restricted File" screen.
- **Answer judging:** the `specs okspell,okcap,bumpshift` lines sit *above* each
  arrow. TUTOR clears specs whenever an arrow is executed, so they never took
  effect. Answers must be spelled right and typed in lower case, as on PLATO.
- **Right answers:** they aren't acknowledged. The next riddle simply appears.
- **Typos:** "drangons", "slimmy", "seaons" and others are kept as he typed them.

## Source and build

- `..\src_original\ciswork29.words`: the lesson file, 14 blocks × 320 words. Its
  header is at sector 3990 of `DD844_C02u2`, and the read follows the NOS track
  chain.
- `..\src_original\ciswork29.txt`: decoded listing.
- The engine is shared with the other PLATO ports, in
  `D:\tools\IFBackup\_PLATO_work\engine`. `build.bat` regenerates `src\game_data.c`
  and relinks.

## Answers

<details><summary>The five riddles (spoilers)</summary>

mountain · wind · dark · fish · time

</details>
