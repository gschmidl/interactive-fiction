# ADVENTURE-ENB port — notes

Where the program came from, how it was built, what the port fixed and what
SINTRAN III it needs, and how the port was checked against SINTRAN III
itself.  Octal throughout unless marked.

## Where it came from

The NDFS floppy TROTYL (`ND-disk-00317`), user SYSTEM:
`ADVENTURE-ENB:SYMB`, created 18 February 1983, last written 3 June 1983
(`..\src_original\`).  The same hand signed `DOD-ENB` (the DOD port),
`LABYRINT-ENB`, `INVADERS-ENB` and `STARHAWKS-ENB` there; nothing on the
floppy gives ENB's name.

The program opens two files of another user's, `(MIMER)ADV-BEGIN-ENB:DATA`
(a title picture, printed between ESC F and ESC G, the Facit terminal's
graphics characters on and off) and `(MIMER)INST-ENB:DATA` (instructions:
pages ended by a line `*`, the whole by `SLUT`).  Neither is on TROTYL or on
any floppy of the catalogue of 899 (`floppies.json` of nd100x).  The README's
account of the game is from the source.

It uses the DNF club library (`LIBRARY-MJ`: `CURS`, `PATCH`, `INBYTE`) and
the monitor calls `DESCF`, `EESCF` and `COMND`.

## How the programs were made

On SINTRAN III VSX/500 L under RetroCore, the machine of the Mordor, Legend
and Cave Fun ports (user DNF), by `tools\rebuild.py` (commands in
`tools\build.cmd`, output in `build\build.log`), as Cave Fun's: the BASIC
compiler of January 1985, `TABLE-SIZES 1500,35000`, each program in a BASIC
session of its own; NRL with `SIZE 2700`, the program, the club library
`LIBRARY-MJ:BRF` (NILSSON-3) and `BASLIBR-H00:BRF` as the Legend port rebuilt
it.  Numbers are real, the compiler's default: the program declares the
integers it wants (`100 INTEGER INBYTE,IN,PLAN,...`).

| program | from | sha1 |
| --- | --- | --- |
| `data\ADVENTURE-ENB.PROG` | `basic\ADVENTURE-ENB.SYMB` (the fixes) | `b1477fe5da2c` |
| `build\original\ADVENTURE-ENB.PROG` | `..\src_original\ADVENTURE-ENB.SYMB` | `f5bd4ade9319` |
| `build\start\ADVENTURE-ENB.PROG` | the same with only lines 131-138, 150, 211-212, 2970 and 3041-3042 of the fixed source (`startable()` in `tools\rebuild.py`) | `905bf446f4ce` |

The program as recovered never gets past its start without the lost files
(below); `build\start\` is it with just enough mended to start, so that
the other fixes can be shown against it (`tests\fixtest.py`) and it can be
played on the reference machine.  NRL's DUMP writes words the program never
uses along with the rest, holding what ran before it in the same session:
the fixed program's bytes changed, its source not, when `build.cmd` got the
third compile.  Every test passes on each build.

## The port's fixes

At the user's direction, as for the other ND-100 ports: fix what a player
cannot be expected to understand.  Each is marked `Port fix 2026` in
`basic\ADVENTURE-ENB.txt` (`basic\fixes.diff`), and `tests\fixtest.py` shows
each but the last on both programs.

| lines | bug |
| --- | --- |
| 131, 136, 150, 211, 2970, 3041 | Each file is read under `ON ERROR GOTO` the line that closes it, and the handler stays in force: with the file missing, the `CLOSE` of a file never opened was an error too and went to the same line, for ever.  The program as recovered hangs before its first question (and did on the reference machine); the `J` and `H` instructions hung the same way.  Now the handler ends before the `CLOSE`, and the game goes on without the files. |
| 3280 | The princess sits at 15,1 (line 3180), but asking her (`F`) was only taken at 20,1, in caves of 15 by 15: she never followed, and the game could not be won. |
| 530, 570, 580 | `INT(RND*85+2)` runs from 2 to 86, and `PLAN` from 0 to 85.  The run-time library only checks an index against the size of the whole array, so `PLAN(86,J)` is `PLAN(0,J+1)`, outside the land, and only `PLAN(86,85)` and beyond stop the game: about one game in sixteen ended `DIMENSION OUT OF RANGE` while the land was made (river or road at 86, 9 of 150 starts), one in 85 had its cave at row 86, where no one could reach it, and some started the player there, at `Du är vid ` nothing.  Now 2 to 85. |
| 1010, 1070 | Walking north or south on the road takes two steps; from row 84 (or 2) the second one left the land, to 86 (or 0), nowhere (or `DIMENSION OUT OF RANGE`, with the road at column 85).  Now the second step is not taken there. |
| 1270-1281, 1355-1405, 3820 | What was taken or bought went into `HA$(HI+1)`, and `HA$` has 12 places: the 13th in a game (water from the river 13 times, say) ended it, `DIMENSION OUT OF RANGE`, though gifts and the old women's curse emptied places.  Now it goes into the first empty place, and with all 12 full: `Du kan inte bära mer`, which the caves already said at 13.  A purchase of something not sold said the last thing bought was bought again; now it says nothing. |
| 1765 | `Hur många vill du leja (1-5)`: 11 or more stopped the game (`LEJA` has 10).  Now more than 5 is asked again. |
| 2111, 2220 | A fight above ground with men hired (as in the caves, lines 2460, 2640, the men take the blows): `DOED`, their share, was never set, so the first blow killed a man; and `MAN` was never counted down, so every blow after it killed him again, took 10 more from defence and armour, and the player was never alone to take a blow.  Defence and armour went below nothing, and no fight after could be won. |
| 3495, 3496 | A room of the caves shows one of seven things, chosen again at random until one not yet taken comes up: with all seven taken, it looked for ever. |
| 155, 740 | `CALL PATCH` without its argument.  PATCH (club library) adds its argument to `SAA 0` and stores the instruction into BASIC's input routine, where it gives the character INPUT and LINPUT prompt with (Johansson's own programs call `PATCH(0%)`, none, and `PATCH(63%)`, `?`): with none given, it took the word at address 0, whatever SINTRAN or an earlier program had left there.  On the reference machine that was 0177, `SAA 177`, and every question after the first ended in DEL, which a terminal does not show; another word there would have made another instruction, run by every INPUT.  `PATCH(0%)`: no character, as it looked.  (`tests\ref-start\` shows the DEL; fixtest does not show this one.) |

Not changed, as the author's own game:

- One gift of `vatten` takes all the water carried: line 3710 empties every
  place holding what was given.
- `GE` to `PR`, `GU` or `DR` in capitals works anywhere, without the
  princess, an old woman or a dragon: `.AND.` binds before `.OR.` (3620-3640),
  so only the small-letter answers ask for them.  A dragon anywhere tells the
  cave's place, and an old woman anywhere pays 10.
- In the caves, `G` before an old woman prints `Fråga` and goes into the
  fight (2350).
- The monsters' `ett troll-spö` is `ett trollspö` where its strength is set
  (2420), so it keeps the last weapon's.
- The thirty monsters start in the same places every game: line 370 draws
  them before the first `RANDOM`.
- `Du kan inte bära uer` (1350).
- `T` in a cave room takes the thing last drawn for the room it was last
  shown in, seen or not.

## SINTRAN III as the program needs it

Added to the emulator the other ND-100 ports share (`src\`), each seen on the
reference machine:

| | |
| --- | --- |
| keys | the arrow keys send ESC A to ESC D, Home ESC H, as a Facit's did; the program switches the escape function off (`DESCF`) at every `ORDER:`, and reads the ESC as a key |
| capitals | a key read without echo (`INBYTE`, the commands) is made a capital; lines are left as typed.  The reference machine's terminal (user DNF) is in capital-letter mode (the My World port's sessions show it), which would hand a gift over in capitals, where the game asks for it in small letters (`Skriv vad du vill ge med små bokstäver`) and never finds it: ENB's terminal was not.  (No session typed a line in small letters, so the two agree.) |
| the uptime | `RANDOM` seeds from the uptime (MON 11, basic time units of 20 ms).  Line 470 and line 630 each call `FNSLUMP`, which calls `RANDOM` and draws the river's and road's place, then the player's ("UTSLUMPNING AV SPELARE", line 620): 22,000 instructions apart, a unit or more on an ND-100, but a tenth of a millisecond on the host, so the host's clock had not moved, both drew the same, and every game started on the bridge where river and road cross.  (RetroCore, faster than the machine it emulates, started 3 of 4 games there too, `tools\startplace.cmd`.)  The port's uptime now also runs with the instructions done, 20,000 to a unit (an ND-100 did about a million a second) |
| `--uptime` | the uptime starts at the number given and runs with the instructions alone: one land for each number, the same every time |
| `@` | `COMND` (MON 70) runs the few commands the port knows (`TERMINAL-MODE`, the escape functions); the rest are ignored |
| address 0 | an ND BASIC program starts with 0177 there, as it did on the reference machine (below the program is what SINTRAN left, not the program's own) |
| `--debug` | a `#` line is taken at a key read too, not only at the start of a line (the one-key `ORDER:` follows no line) |

## The reference machine

`tools\reffuzz.py` boots RetroCore, logs in as DNF and plays games with the
random player `tools\play.py` (the map, the towns, buying, hiring, asking the
way, giving, fighting), typing each answer only once the program has asked.
It plays `AENBF-REF`, the port's program with `RANDOM` not reading the
uptime (its MON 11 and the `COPY SD DA` after it made `SAA 0`), so that every
game is the same land for the same typing, and the port plays it with `-Z`
(its uptime stays 0).  `--start` plays `AENBS-REF`, the startable program as
recovered; `--script` types a file's answers (`tests\scripts\`).

In the reference land the cave is 57 steps from the start, and thirty
monsters close in all the way: in 300 games on the port, told where the cave
was, the random player never got there alive.  So the caves are played on
the port only (`tests\fixtest.py`, `tests\winnable.py`); they use the same
statements of the same BASIC run-time library as the rest.

## What the tests say

| | |
| --- | --- |
| `python tests\run.py` | **18 of 18 sessions identical**, 47,399 bytes: 8 random games of the port's program (`tests\ref\`), 6 of the startable program as recovered (`tests\ref-start\`, its questions ending in DEL), and water taken 13 times and 11 men hired, in each (`tests\walk\`, from `tests\scripts\`: `DIMENSION OUT OF RANGE` on SINTRAN as on the port) |
| `python tests\fixtest.py` | 8 of 8 fixes shown on both programs |
| `python tests\winnable.py` | the game won: `Du har lyckats fullständigt du fick prinsessan och halva kungariket` |
| `python tools\portfuzz.py 40 200` | 0 of 40 games had trouble (each its own land, `--uptime`): no hang, no unimplemented call or instruction, no BASIC run error, no question the random player did not know |
| `python tests\consoleplay.py` | 4 checks at a real console (ConPTY): the Swedish letters, the arrow keys and Home, commands in small letters, `S` and the window held open |
