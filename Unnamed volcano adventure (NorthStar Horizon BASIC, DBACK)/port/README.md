# The unnamed volcano adventure (North Star BASIC) - Windows console port

`volcano.exe` is the adventure in the BASIC file DBACK on the NorthStar
Horizon disk `101DISK.NSI`, run by the North Star BASIC on the same disk
(the file HYBASIC) on an emulated Z80. Run `play.bat`.

You escape headhunters down a volcano into a lava tube, find a lantern
and a gold brick, cross a maze and reach a room with a trap door. Moves are
single letters - `U` `D` `B` (back) `R` `F` `T` (take) - and `INVENTORY`.
The game is unfinished: every way on ends it. You fall into the pit (the
sign says "DEAD END, DUNGEON UNDER CONSTRUCTION, PLEASE COME BACK NEXT WEEK"),
the orcs eat you, or they chain you in their prison. There is no score and
no win. **`WALKTHROUGH.md`** shows the way to the prison: the route, what
kills you where, and twelve moves that always get there with `--seed 5`.

## How it is built

`build.sh` (or `build.bat`):

1. `src/mkdata.py` cuts three files out of the disk image
   (`..\archive_original\101DISK.NSI`: NorthStar DOS 5.1, double density,
   350 blocks of 512 bytes):
   - HYBASIC (26 blocks, loaded at 2D00H);
   - the DOS file (its bytes go to 2000H);
   - DBACK.

   It also lists DBACK as HYBASIC's LIST would. The token values come from
   HYBASIC's own keyword table.
2. `src/volcano.c` and `src/z80.c` are compiled into a Z80 that runs
   HYBASIC from its cold start.
3. `volcano.exe --check`. At the start, the port types the listing into
   HYBASIC a line at a time with the output hidden, then types RUN.
   HYBASIC tokenises the program again. `--check` stops at RUN and compares
   the result with DBACK's bytes on the disk: all 5891 bytes match, at
   6000H. So the text the port types is the program on the disk.

## What the interpreter settled

**DOS is not run.** HYBASIC reaches DOS through the jump table at 2000H.
The port does the four calls the game needs itself:

| Address | Routine | What it does |
|---|---|---|
| 200DH | COUT | writes the character in B, and returns it in A |
| 2010H | CIN | reads a key |
| 2013H | TINIT | initialises the terminal |
| 2016H | CONTC | Ctrl-C check; always NZ, "none typed" |

Any other call into 2000H-2CFFH, and any IN or OUT, stops the port with
exit code 3. The game has no file I/O, so it never makes one.

**RND.**
- **RND(0)** gives the next number. The series starts again at every RUN
  (.98681641, .78627014, .4095459, ...). So the maze, chosen with
  `INT(6.1*RND(0))+1`, always takes the same number of moves from a given
  point: from MAZE1, three.
- **A positive argument is a seed** and repeats: RND(.5) is .74450684 every
  time.
- **A negative argument reseeds from the disk.** `RND(-1)` chooses where
  the maze lets you out. HYBASIC counts how many times it reads the disk
  controller's sector number (EB30H, low four bits) before the number
  changes. That count, up to about 2350, becomes the seed.

  The disk turns at 300 rpm with ten sectors, so a new sector comes every
  20 ms, whatever the program is doing, and while the player types. The
  port runs the disk from its own clock:
  - each emulated instruction counts as 6.8 T-states at 4 MHz (the
    counting loop is 34 T-states for 5 instructions);
  - the real time spent waiting for keys is added.

  The clock starts at RUN, so typing in the program and its fixes does not
  move it. `--seed MS` makes the waiting time a fixed MS milliseconds, and
  a game then repeats. MS may be a fraction. Seeds 20 apart are the same
  game.

- **The generator.** RND is a 16-bit shift register: 23 shifts per number,
  the feedback bit being the parity of the low byte AND 2DH. The result is
  (HL+1)/65536. A model of it in Python reproduces the RND(0) series
  exactly. Over all ~2350 counts, the maze's `INT(3.499*RND(-1))+1` lets
  you out as follows:

  | Exit | Share |
  |---|---|
  | the pit | 28.8% |
  | the dirt passage | 28.4% |
  | the narrow passage | 28.7% |
  | the crevice to the trap door | 14.1% |

  That is what the 3.499 asks for: the crevice gets half a share. Whole
  milliseconds of `--seed` see only 20 of those counts, and only one of
  them, 2, is the crevice.

  Rogers' *User's Guide to North Star BASIC* (1978, `..\doc`) describes
  the argument differently: a seed between -1 and 1, made as `-P/100`. It
  documents Version 6 Release 3. In this HYBASIC, a negative argument is
  the disk timing: `JP M` goes to the counting loop at 43D8H.

**INPUT.** HYBASIC echoes every key itself; the console's own echo and
line editing are switched off:
- Backspace and DEL rub out a character.
- Other control keys ring the bell.
- A line holds 80 columns, the prompt included. A move of 74 characters or
  more after "MOVE? " stops the program with "LENGTH ERROR IN LINE 32001",
  as it did on the Horizon. The port then ends with exit code 1.

Keys are folded to capitals and masked to 7 bits, and NULs are dropped.
CR, LF and CR LF each end a line.

**Two details of LIST and typing.** HYBASIC keeps the space typed after a
line number (`100 IF` is stored with it), so the listing is typed exactly
as LIST prints it. The program sits at 6000H.

**The Z80 core** is the Wang OIS port's, with one bug fixed: AND, OR and
XOR must clear the carry. With the carry left set, HYBASIC's BCD
arithmetic gave `1+1 = 2.0000001`, and RND, LEN and FREE were syntax
errors.

## Options

    volcano [--seed MS] [--basic] [--no-fixes] [-u] [-h]

- `--basic`: stay in BASIC when the game ends (READY). Normally the port
  ends there, with exit code 0, or 1 after a BASIC error stop.
- `--direct`: HYBASIC alone, without the game.
- `--check`: see above.
- `--trace`, `--sample`: debugging.
- `-u` / `--unlimited`: accepted; there is nothing to lift.

## Debug mode

The author's own warps, typed at any `MOVE?`:

| Warp | Line | Where it goes |
|---|---|---|
| `MAZE1` | 10000 | the maze |
| `TRAPDOOR` | 15070 | the trap-door room |
| `PIT` | 800 | the pit, which ends the game |
| `SHAFT` | 292 | under the shaft |

`INVENTORY` lists what you carry.

## Fixes (`--no-fixes`: none)

A fix is a line typed after the listing that replaces the author's line,
the way a North Star owner would have made it.

1. **The capture is shown.** The trap door opens, and unless the orcs eat
   you (16010), 16005 goes to 17070: "AS YOUR EYES SLOWLY BECOME
   ACCOSTOMED TO THE DIM LIGHT, YOU CAN MAKE OUT ... LUCKLESS CREATURES
   CHAINED TOO THE WALLS". That skips 17000-17060: the orcs drag you up
   the spiral staircase into a dim, foul-smelling room and chain you to
   the wall. Nothing else reaches 17000, so you would be among prisoners
   in the trap-door room. Now 16005 is `GOTO 17000`.

## Kept as they are

- The spelling of the messages: VIN, LWEADING, YYOU, SCREEMING,
  SULPHURUS, ACCOSTOMED, SPIRIAL, TOO THE WALLS, and "OF THE" twice
  (15150/15160).
- "AHEAD THE DARKNESS CLOSES IN." is followed by "SOMETHING, AND DISCOVER
  A FASCINATING LANTERN-SHAPED OBJECT" (350, 360). The words in between
  are not in the program. Between the two lines are only 351 and 352, the
  lantern's test.
- Any move but the right one on the slopes or the rim means the
  headhunters. At the lava tube it repeats the tube; at the shaft it asks
  again.
- `F` at the junction goes on into the maze, as `D` does. Two variables
  are set and never used: `S=10` at 560, and `K`, which counts the maze
  moves at 10001.

## Checked so far (first pass, 2026-09-21)

- `--check`: the program HYBASIC tokenises from the listing is DBACK, byte
  for byte.
- `tests\regress.py`:
  - HYBASIC's arithmetic, and RND's series and seeds;
  - the opening and the headhunters;
  - the lantern, the gold brick and INVENTORY;
  - the four ways out of the maze, by seed;
  - both trap-door endings;
  - fix 1, with and without `--no-fixes`;
  - SHAFT and PIT;
  - the 74-character LENGTH ERROR and its exit code 1;
  - CR / LF / CR LF;
  - `WALKTHROUGH.md` (written 2026-09-21 at the user's request): its twelve
    moves reach the prison with every seed it names, and the feast with
    seed 6; TRAPDOOR, Enter, U reaches the prison at any time. The seed-5
    route was also played at a ConPTY console, key by key in lower case.
- `tests\fuzz.py`: 300 runs, 200 with fixes and 100 with `--no-fixes`,
  about 26,700 moves read. Moves are mostly the game's letters, plus
  words, junk, control keys and bytes above 127, and every fifth run types
  only junk at the shaft. No run failed: each one ended at one of the four
  endings or when its moves ran out.
- `tests\consoleplay.py`: a game at a real (pseudo) console. It checks
  that:
  - the cursor waits after "MOVE? ";
  - a key shows once;
  - Backspace rubs out on the screen and in the move;
  - the pit ends the program with exit code 0 and no READY.

## Still to do (refine pass)

- A second implementation: run DBACK in another North Star BASIC, a
  Horizon emulator, or real hardware, and compare transcripts. With
  `--seed` the RND(0) series and the text should agree line for line. The
  disk timing, 6.8 T-states an instruction, is a model: the machine's
  counts would come from its own instruction mix.
- Whether HYBASIC is North Star's release 6 or a local variant: the name
  and the disk's lab software suggest a site build. Its keyword table and
  tokens match North Star BASIC.
- The disk's other programs (WDIG, DIGITIZE, CATASM ...) are lab tools. No
  other game is on the disk.
