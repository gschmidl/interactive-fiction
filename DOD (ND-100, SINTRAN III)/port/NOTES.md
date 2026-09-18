# DOD port — notes

Where the program came from, how it was built, what the port fixed, and how
the port was checked against SINTRAN III itself.  Octal throughout unless
marked.

## Where it came from

The NDFS floppy TROTYL (`ND-disk-00317`), user SYSTEM: `DOD-ENB:SYMB`,
created 18 February 1983, last written 21 November 1983
(`..\src_original\`), by the ENB of `ADVENTURE-ENB` (the Adventure ENB port;
nothing on the floppy gives ENB's name).  It is ND BASIC with no library
calls of its own.

The source as recovered does not compile cleanly: lines 570, 580 and 590
stand twice, the second time after line 600 with a 591 of their own, and
line 3120, where line 610 goes, is not there.  The compiler of January 1985
refuses the second 570-590 (`LINE NUMBER USED PREVIOUSLY`) and says `"LINE
3120" MISSING`, and makes a program all the same (`build\original\`).  It
places the lines by their numbers, not by where they stand in the file: 591
comes before 600 in the program (the same answers meet the same dice in both
programs until the river, `tests\fixtest.py`).

Nothing leads to lines 850-2330: giant rats, a crossroads, a treasure
chamber, sleeping zombies, the zombies' leader and a balrog in the temple of
evil, the wizard Ostomar's tower, the vampires, the hydra's lair.  `SLUMP`,
drawn from 1 to 9 at line 511, sends 1 and 6-9 to a room and 2-5 on to line
600: lines 530-560, which would have sent 2-5 to those rooms, are lost.  The
port does not make them up again.

## How the programs were made

On SINTRAN III VSX/500 L under RetroCore, the machine of the other ND-100
ports (user DNF), by `tools\rebuild.py` (commands in `tools\build.cmd`,
output in `build\build.log`), as ADVENTURE-ENB's: the BASIC compiler of
January 1985, `TABLE-SIZES 1500,35000`, each program in a BASIC session of
its own; NRL with `SIZE 2700`, the program, the club library
`LIBRARY-MJ:BRF` (NILSSON-3) and `BASLIBR-H00:BRF` as the Legend port rebuilt
it.

| program | from | sha1 |
| --- | --- | --- |
| `data\DOD-ENB.PROG` | `basic\DOD-ENB.SYMB` (the fixes) | `f84e4dd524b8` |
| `build\original\DOD-ENB.PROG` | `..\src_original\DOD-ENB.SYMB` | `1a2c831b805c` |

NRL's DUMP writes words the program never uses along with the rest, holding
what ran before it in the same session; two builds with the same
`build.cmd` gave the same bytes.

## The port's fixes

At the user's direction, as for the other ND-100 ports: fix what a player
cannot be expected to understand.  Each is marked `Port fix 2026` in
`basic\DOD-ENB.txt` (`basic\fixes.diff`), and `tests\fixtest.py` shows each
on both programs, with an uptime (`--uptime`) that brings the scene about.

| lines | bug |
| --- | --- |
| 570-591 | Twice in the file; once, in order, now.  (The compiler kept the first, so the program is the same.) |
| 3120 | Lost, and line 610 goes there, the way out (30 in 100 at every turn that finds no room).  The compiler put 177777 in its line table for the missing line, and the `GOTO` jumps there: to 177777, and on through address 0 and all the memory below the program, run as instructions.  What lies there is SINTRAN's: on the reference machine it came to a question for the terminal type (`Terminal types are: ... What is your`), and the game was gone; the port has zeros there, which run through to the program's start, a new game.  Line 3124, `DU ÄR UTE I FRIHETEN DU FICK n POÄNG`, the only end that is not death, is where nothing else leads; 3120 is now `GOTO 3124`. |
| 3106 | Crossing the river one by one, each death adds 1 to `DOD`, and `DOD` is taken from the company afterwards; it was never set back to 0, so at every crossing after the first the dead of all the crossings before were taken again.  A company of 9 losing 4 was left with 2, or "all dead" with people alive. |
| 3110, 3111 | `PERSON NR. n ÖVERLEVDE/DOG` gave every person the company's number `A`, not their own, `KJG`. |
| 790, 2000, 2790 | The trolls, the harpies and the orcs killing the company to the last went straight to `END`: the game stopped without a word.  Now they go to line 3115, `NI ÄR ALLA DÖDA`, the game's own words, which nothing reached. |
| 2970-2976 | Going on (`F`) past Medusa: `FEM PERSONER DOG NÄR NI GICK FRAM`, and the company stayed as it was.  Now five are taken, and with none left, 3115. |

Not changed, as the author's own game:

- The crossroads at the start (`F/H/V`) and every answer to it lead to the
  same place: the first room is drawn whatever is typed.
- `HÄRIFRÅN FINNS DET GÅNGAR ÅT ALLA HÅLL (F/H/V)` after the orcs: only `V`
  does not fall into the pit.
- At the river `A` (all at once) gets everyone over safely.
- A company of 0 goes on (`A<0` is death, `A=0` is not), and can come out
  with 0 points.
- Line 3125, `@EN-ESC,,,,,,,,,,,,`, is a SINTRAN command in the program
  (ND BASIC runs a line that begins with `@`): `ENABLE-ESCAPE-FUNCTION` as the
  game ends.  The port does as SINTRAN does.

## The reference machine

`tools\reffuzz.py` boots RetroCore, logs in as DNF and plays games with the
random player `tools\play.py` (the game's answers, now and then something
else), typing each answer only once the program has asked.

DOD's dice decide nearly everything: the answers at the crossroads and to
the demon change nothing, so with one seed every game is the same game
(with seed 0: the demon, then the way out with 12 points).  So game NN plays
`DODF-RNN`, the port's program with `RANDOM` seeded with NN (its MON 11 and
the `COPY SD DA` after it made `SAA 0` and `SAA NN`, `tools\refprog.py`), and
the port runs the same image; `--original` plays `DOD-RNN`, the program as
recovered.  The port's uptime runs with the instructions done, as
ADVENTURE-ENB's (its NOTES).

The program as recovered took the lost way out in 5 of its 12 games, and in
the first one tried, with seed 0 (`tests\sintran-only\`): there the reference
machine and the port part, as above, and `tests\run.py` compares them up to
the jump.

## What the tests say

| | |
| --- | --- |
| `python tests\run.py` | **25 of 25 sessions identical**, 25,118 bytes: 12 random games of the port's program (`tests\ref\`, seeds 1-12), 7 of the program as recovered (`tests\ref-original\`), and 6 of it up to the lost way out (`tests\sintran-only\`) |
| `python tests\fixtest.py` | 7 of 7 fixes shown on both programs |
| `python tools\portfuzz.py 200 150` | 0 of 200 games had trouble (each its own dice, `--uptime`): no hang, no unimplemented call or instruction, no BASIC run error, no question the random player did not know.  Of the 200, 84 came out alive and 116 died: the game can be won, and by chance alone |
