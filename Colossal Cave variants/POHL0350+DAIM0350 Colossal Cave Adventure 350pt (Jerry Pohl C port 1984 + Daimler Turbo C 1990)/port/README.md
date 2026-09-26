# Jaeger/Pohl C Adventure (1984) and Daimler's Turbo C version (1990) - Windows console ports

Two programs, each compiled from its own original C source:

| launcher | program | source |
|---|---|---|
| `play-pohl.bat` | `pohl\advent.exe` - BDS C conversion by J. R. Jaeger, Unix standardisation by Jerry D. Pohl; PC-SIG disk 259, "Author Version: 03/90" (Pohl's 1990 revision of his 1984 program) | `..\src_original\POHL0350` |
| `play-daimler.bat` | `daimler\advent.exe` - the same after Martin Heller's OS/2 conversion (1988) and Daimler's Turbo C 2.0 conversion (1990, if-archive `advtc2.zip`) | `..\src_original\DAIM0350` |

    advent [-r] [-d ...] [--restore] [--no-fixes] [-h]

      -r, --restore  start from a saved game (the game asks its name)
      -d             the authors' debug output: three times for Pohl, twice for Daimler
      --no-fixes     the program as it was (the fixes below off)
      -h, --help     the options

`-r` and `-d` are the authors' own (an unknown `-x` still gets their "unknown flag: x"); an unknown `--option` is
refused, exit 2. Each program reads its four text files (`advent1..4.txt`) from beside the .exe. SUSPEND (or SAVE,
PAUSE) asks for a name, writes `NAME.adv` and stops; the launchers keep saves in `saves\pohl` and `saves\daimler`,
because the two formats differ (145 vs 140 locations). QUIT ends the game with the score; the end of input (Ctrl-Z
Enter, a closed pipe) ends it at once, exit 0. Pohl's game starts at the end of the road, Daimler's in the building.

## The DOS originals (reference)

Fetched 2026-09-21 from the IF Archive (with the user's go-ahead) into `..\archive_original\`: `adv.arc` (Pohl's
DOS build, 10 June 1984, with its text files) and `advtc2.zip` (Daimler's DOS build of 6 May 1990 with his sources,
identical to the ones ported here). Unpacked in `..\reference\pohl-dos` and `..\reference\daimler-dos`
(`_bits_sweep_work\tools\unarc.py` reads the ARC). `tests\dosref.py` runs either under DOSBox 0.74-3, headless.

- **Daimler's shipped program printed most of its messages wrongly.** The zip's `ADVENT4.TXT` is dated 23 June, seven
  weeks after the EXE: Daimler added the line "-Conversion to TurboC 2.0 by Daimler" to the welcome (message 65)
  without rebuilding, so every message from 66 on was read 39 bytes too early (INVENTORY began "8"). Removed again,
  that line gives back the May file: all 201 offsets compiled into the EXE then match. The port indexes the text it
  ships with, so it has the credit and the right messages; the reference runs use the May file.
- **Daimler compiled with Turbo C's signed char** - see Fix 3.
- **`tests\crossdos.py`: the port with `--no-fixes` and Daimler's own program print the same, byte for byte** (20 random
  sessions of 250 commands).
- **Pohl's DOS program is not a reference for this port.** It is the 1984 build, and the source here is his 1990
  revision, which differs from it: the game starts at the road, not in the building; DOWNSTREAM moves you (in 1984 a
  known word with nowhere to go); commands of three words or more, FEE FIE FOE FOO and very long unknown words are
  parsed differently. Its text files are the 1984 ones, with typos the 1990 files correct.

## How it is built

`build.bat` / `build.sh` (MinGW-w64 gcc), once per program:

1. the text files are staged with DOS line endings (CR LF), as they were distributed, and end at a Ctrl-Z if they
   have one (left in, it upsets the Windows C library's text-mode `ftell()` by two bytes in the buffer that holds it);
2. the authors' `advent0.c` is compiled and run, and indexes the text files into `advtext.h` (byte offsets from
   `ftell()`);
3. the game is compiled with that header; `.build\` holds the intermediates.

Flags: `-std=gnu89` (K&R C), `-funsigned-char`, `-fcommon`, `-fno-builtin` (the 1984 files redeclare library
routines with their own types), `-fwrapv` (the game's random number generator multiplies a `long` until it wraps).

`src\pohl` and `src\daimler` are copies of the originals; every change is marked `PORT:`. Most of the port is
`src\port\portcompat.h` (force-included) and `src\port\winport.c`:

- `exit()` with no argument, BDS C `setmem()`, and the C86 two-argument / Turbo C three-argument `ltoa()`.
- `rand()` and `srand()`: the game brings its own (turn.c, `rnum * 0x41C64E6D + 0x3039`, seeded with 511 at every
  start), renamed so the C library's does not replace it. Its bits 16-30 do not depend on the width of `long`, so the
  numbers are those the DOS programs drew.
- Pohl calls his timer `clock`, which is also a C library function: renamed `adv_clock`.
- Turbo C `putw()`/`getw()` read and write 16-bit words, so Daimler's save files have the DOS layout.
- `fopen()` looks for the text files beside the .exe when they are not in the current directory.
- The options above.

Edits in the sources, whatever the options:

1. **End of input ends the game** (english.c `getwords()`, database.c `yes()`): the unchecked `fgets()` otherwise
   re-parsed a stale buffer for ever.
2. **Pohl's SUSPEND/restore.** It wrote the raw bytes between `&clock2` and `&tally`, which only works with CII C86's
   placement of the globals. `savevars()` now writes the play variables listed in Pohl's own `advdec.h` one by one, as
   16-bit words.
3. The saved-game name is read with `%12s` (it was `%s` into `char[13]`).
4. The K&R declarations `extern int printf(); scanf(); sscanf(); fprintf();` are commented out: they contradict the
   variadic prototypes in `stdio.h`, which every file includes.
5. **Reads past the arrays**, each giving what the DOS program read there: Daimler's four loops to `i <= MAXOBJ` (or
   from 100 down) read one past `place[]`, which on the PC was `fixed[0]`, never a carried object; his `move()` read
   `fixed[obj]` for the second place of a two-place object, which only decides whether `carry()` runs, and `carry()`
   does nothing with such an object; and `rspeak()` of a message number 0 or below read the tables before `idx4`,
   which `port_idx4()` now looks up explicitly (in both EXEs idx1, idx2, idx3 with 100 slots and idx4 lie end to end).
   In Pohl's BACK (`goback()`), `gettrav()` is only called for a destination that is a location, as Woods' FORTRAN
   has it ("IF(LL.GT.300) GOTO 22"): for a special move or a message it copied a string from far past the end of
   `cave[]` (Hall's version of the same code crashed on RETREAT). For Daimler's the same is Fix 5.

## Fixes (on unless `--no-fixes`)

- **Fix 1 (both): a dwarf blocks the way.** turn.c line 36 tests `cond[loc]&NOPIRAT == 0`, which C reads as
  `cond[loc] & (NOPIRAT == 0)`, always 0: "A little dwarf with a big knife blocks your way." could never appear. With
  the fix a dwarf that has come from where you want to go blocks you, as in Woods' FORTRAN.
- **Fix 2 (Daimler): the pirate keeps out of the rooms marked NOPIRAT.** Daimler changed Pohl's `cond[j]&NOPIRAT` to
  `cond[j]&NOPIRAT == 1`, again `cond[j] & 0`, so the pirate could be placed anywhere.
- **Fix 3 (Daimler): message numbers above 127.** Daimler keeps message numbers in `char` variables (`char msg`, and
  `char i` in `actspk()`), and Turbo C's char is signed: on the PC every number above 127 went to `rspeak()` negative
  and printed text from a place picked out of the tables before `idx4` - READ LAMP said "e an object are attempting
  something beyond their ...". With the fix the message is the one Daimler wrote; with `--no-fixes` it is what his
  program printed (verified against it).
- **Fix 4 (Daimler): LOG with an object.** Daimler cut `actmsg[]` to 32 entries, dropping LOG's (verb 32; Pohl's is
  13, "I don't understand that!"), and `actspk()` stops the game for any verb above 31: LOG LAMP ended it with "Fatal
  error number 39". With the fix LOG gets Pohl's answer.
- **Fix 5 (Daimler): BACK past a forced move.** Daimler's `goback()` set `k2 = temp`, a location number, and then
  took the motion from `travel[k2]`, far past the end of `travel[]`; and it called `gettrav()` for special moves and
  messages as well, reading far past `cave[]`. With the fix it uses the travel entry's own index, as Pohl's does, and
  looks only at destinations that are locations. With `--no-fixes` his lines run as they were: in the sessions
  compared, what they read there made the port do what his DOS program did.

## Tests

    python tests\regress.py        options, end of input, SUSPEND and --restore, each fix with and without
                                   --no-fixes (Fix 1 on a random walk in the cave that meets a dwarf), then a short
                                   fuzz and a short crossdos
    python tests\fuzz.py [N]       N random sessions of each program's own words (default 100 x 300), every other
                                   one with --no-fixes; a session that ends in SUSPEND is restored and played on:
                                   no crash, no hang, exit 0
    python tests\crossdos.py [N]   the port with --no-fixes against Daimler's DOS program under DOSBox, N random
                                   sessions: the same transcript, byte for byte
    python tests\dosref.py P FILE  a transcript of the DOS program P (pohl or daimler) given the lines of FILE

## Checked (refine pass, 2026-09-21)

- regress: all passed. fuzz: 100 sessions of 300 commands per program, 118 of them restored from SUSPEND, 0 failed.
  crossdos: 20 sessions of 250 commands, identical to Daimler's DOS program.
- The first crossdos runs found Fix 3's signed char, the Ctrl-Z in the DOS text files (above), and the fuzz Fix 4;
  Fix 5 came from Hall's version, where the same code crashed.

## Still to do (refine pass)

- Deferred (user, 2026-09-21: bugs and fuzzing only): winnability through a debug mode, and a ConPTY console test.
- Pohl's 1984 build and his 1990 revision differ as listed above; nothing in the port depends on it.
