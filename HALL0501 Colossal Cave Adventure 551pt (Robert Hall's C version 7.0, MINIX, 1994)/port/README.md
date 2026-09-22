# Generic Adventure 7.0 (Robert R. Hall, MINIX, July 1994) - Windows console port

`advent.exe` is Hall's C program compiled for the Windows console, with the text database `advent1..4.dat` beside
it. Run `play.bat`: SAVE and RESTORE use `saves\advent.sav`, and `play.bat NAME` restores the saved game `NAME` at
start (Hall's only command-line argument). End of input ends the game with Hall's own "PANIC: end of input".

    advent [--no-fixes] [-h] [SAVEDGAME]

      SAVEDGAME    start from a game saved with SAVE
      --no-fixes   the 1994 program as it was (the fixes below off)
      -h, --help   the options

An unknown `--option` is refused, exit 2. The game is McDonald's 551-point "Adventure 6.6" content with Long's
parser, on top of Jerry Pohl's C engine; Hall's scoring tops out at 501 (see `..\src_original\HALL0501\README`).

## How it is built

`build.bat` / `build.sh` (MinGW-w64 gcc, `-std=gnu89 -D_POSIX_SOURCE`, as Hall's Makefile):

1. Hall's `setup.c` reads `advent1..4.txt` and writes the encoded `advent1..4.dat` and the index header `advtext.h`
   (it runs in text mode, so the .dat files and their `ftell()` offsets are CR LF on Windows - written and read by the
   same C library, so they agree);
2. the game is compiled with that header; intermediates in `.build\`.

`src\` is a copy of `..\src_original\HALL0501\*.c *.h`; every change is marked `PORT:`. The port layer
(`src\port\portcompat.h`, force-included, and `winport.c`) makes `fopen()` look for the .dat files beside the .exe
when they are not in the current directory (Hall's Makefile installed them in `/usr/lib/advent/`), and reads the
options. `build.sh --test` also builds `.build\advent_t.exe`, whose `rand()` is a fixed generator seeded from
`ADV_SEED`, for comparison with a Linux build.

## Changes, whatever the options

- **RESTORE takes only a saved game.** Hall's `restore()` read the file straight over the game in play and kept
  whatever it got: a short file left the game half overwritten ("read 1000 bytes, expected 8656", then "Restored"), and
  a foreign file put locations far outside the arrays. Now the file is read aside and taken only if it is exactly one
  `struct playinfo` with its locations in range; otherwise "NAME is not a game saved by this program", and at start
  the program ends (exit 1), in play the game goes on.
- **Reads past the arrays.** `initialize()` ran its two object loops from/to `MAXOBJ`, one past `plac[]`, `fixd[]`
  and the game's arrays (the objects are 1 .. MAXOBJ-1, as every other loop has it). And BACK/RETREAT (`goback()`)
  crashed: it called `gettrav()` for destinations that are special moves or messages (Woods skips those: "IF(LL.GT.300)
  GOTO 22"), reading a pointer far past `cave[]`, and it took the motion from `travel[k2]` with `k2` a location
  number (Pohl's code, which Hall's descends from, kept the travel entry's index there). Both are now as Woods and
  Pohl had them.

## Fixes (on unless `--no-fixes`)

- **Fix 1: a dwarf blocks the way.** Pohl's precedence bug, carried over (turn.c: `loc_attrib & NOPIRAT == 0` is
  `loc_attrib & 0`): "A little dwarf with a big knife blocks your way." could never appear.
- **Fix 2: RESTORE without a saved game** ended the program ("advent.sav: No such file or directory"); now the game
  goes on.
- **Fix 3: LEAVE with nothing to leave** - LEAVE ALL, LEAVE EVERYTHING, DROP LEAVE - reached Hall's placeholder
  `bug(29)` and ended the game ("Fatal error, probable cause: action verb 'leave' has no object"); now "leave what?".
- **Fix 4: an object and then SAY** (KEYS SAY, GRATE MUMBLE) reached the placeholder `bug(34)` and ended the game; now
  it is said, as SAY KEYS is: `Okay, "keys".`

## Checked (refine pass, 2026-09-21)

- SAVE and restore (at start and with RESTORE) round-trip; a truncated file, a file with a byte too many and a
  missing file are refused as above; each fix with and without `--no-fixes`.
- 300 random sessions of 300 commands from the vocabulary (every third with `--no-fixes`): no crash, no hang, no
  fatal error with the fixes. The first such runs found the RETREAT crash and Fixes 3 and 4.

## Still to do (refine pass)

- Deferred (user, 2026-09-21: bugs and fuzzing only): a ConPTY console test, winnability through a debug mode, and
  the comparison with McDonald's 6.6.
