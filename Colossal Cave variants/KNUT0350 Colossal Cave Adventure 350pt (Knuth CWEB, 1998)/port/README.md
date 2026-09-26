# ADVENT (Woods & Knuth, CWEB 1998) - Windows console port

`advent.exe` is Don Knuth's CWEB rewrite (1998, revised 1999) of Don Woods' 350-point Adventure, tangled from the
untouched `..\src_original\KNUT0350\advent.w` and compiled for the Windows console. Run `run.bat` (or `advent.exe`).

    advent [--seed N] [--no-fixes] [--debug] [-h]

      --seed N    replay a game: the random numbers start from N instead of the clock
      --no-fixes  the program as advent.w has it (Fixes 1-3 below off)
      --debug     the # commands below, for testing
      -h, --help  the options

An unknown option, or a `--seed` that is not a number, is refused (exit 2). QUIT ends the game with the score; the end
of input (Ctrl-Z Enter at the console, or a closed pipe) ends it at once, exit 0.

## The source

`advent.w` is the KNUT0350 folder of https://github.com/Quuxplusone/Advent (the repository at d38e825; the folder was
last changed on 31 January 2020):

| commit | what |
|---|---|
| 0d7a838 (May 2012) | Knuth's advent.w of January 2010 |
| 839acbe (May 2012) | Knuth's three errata of March 2011, two surplus `ditto`s in the travel table among them: the "three bugs" of the repository's README, all cosmetic |
| 7bd8b1c (September 2012) | Knuth's errata of August 2012 (TOSS, RUB, the bear, "want to quit", ...) |
| 2a9d682 (January 2020) | his errata of 3 September 2012 (breaking the vase) and 9 October 2014 ("velvet" names the pillow), both reported by O'Dwyer |

Knuth's own text has moved on: his programs page lists the "version of 05 June 2026", and the errata to pages 235-394 of
*Selected Papers on Fun and Games* (his `fg.html`) record what changed. Those not in this copy, and what the port does
with each:

| Knuth's erratum | the change | here |
|---|---|---|
| p. 382, 6 October 2018 and 29 April 2019 | the ranks: `<k`, `[j]+1-k`, `==k` | Fix 1 |
| p. 370-371, 9 February 2020 | the liquids to limbo before the closing's destroy loop | Fix 2 |
| p. 317, 21 January 2022 | listen(): `p++;` becomes `;`, twice | always (a read past the buffer) |
| p. 236, 5 June 2026 | `int main()` | done (the change file rewrites that line anyway) |
| p. 305, 5 June 2026 | `rem_count>=rem_size` in a check at start-up | left out: no effect with 14 remarks in a table of 15 |
| p. 359 and 365, 5 June 2026 | commentary | - |

Knuth's file of 5 June 2026 itself (`..\reference\knuth-2026\advent.w`, fetched 2026-09-21) confirms the table: it has
every change above. It is not, however, this copy plus those: it lacks some book errata that O'Dwyer applied here -
the two surplus `ditto`s (p. 262), `place[t]` for `prop[t]` in a comment (p. 307), "of that action" (p. 320) and
the vase (p. 332, 3 September 2012: in his file, filling the vase smashes it in your hands, and the shards, now
immovable, stay in the inventory). And it prints "Peculiar.  Nothing unexpected happens." with his usual two
spaces, where this copy has one (the erratum as a web page shows it). SAY with an unknown word (item 4 below) is
unchanged there.

## How it is built

`build.bat` / `build.sh`: `ctangle advent.w src\advent-win.ch` -> `.build\advent.c`, then MinGW-w64 gcc
(`-std=gnu89 -funsigned-char`). advent.w carries "Don't change this file without the authors' permission!", so every
port change is in the CWEB change file `src\advent-win.ch`, the mechanism CWEB provides for exactly this.
`-std=gnu89` because it is K&R-style 1998 C whose `typedef enum{false,true}boolean` no longer compiles as C23 (gcc 15's
default); `-funsigned-char` keeps 8-bit input inside the C runtime's ctype tables (with glibc a negative `char` is
harmless, with the Windows CRT it is not).

ctangle: MiKTeX's refuses to run while its user/admin updates are out of step, so the script falls back to WSL Debian's
(`texlive-binaries`, CTANGLE 4.7).

## What the change file changes

For a Windows console:

1. **End of input ends the run.** The program ignored fgets() returning NULL, so at end of input it repeated the last
   command or printed "Tell me to do something." for ever. Now it prints a newline and exits 0.
2. **Options** (above). `main()` gets `argc, argv`; the clock is still the default seed.

Two reads outside the program's arrays, mended whatever the options (what the program found there depended on how the
compiler laid out its globals):

3. **listen()** stepped past the terminating NUL when a word ended exactly at the end of the 72-byte buffer (a
   71-character line with no newline). Knuth's own erratum for page 317 is the change made.
4. **SAY with a word the program does not know** ("say blorp") looked the word up, got -1, and read `hash_table[-1]` to
   see whether it was a magic word. Found by UndefinedBehaviorSanitizer (`tests\crosscheck.py`); not in Knuth's errata,
   and still there in his text of 5 June 2026. An unknown word is now no magic word, and the answer is `Okay, "blorp".`, which is also
   what this build printed before: here the byte before the table happened to be harmless.

Fixes, on unless `--no-fixes`:

- **Fix 1: the ranks as Woods had them.** Knuth's loop moved every exact class score one class up, so 349 of 350 was
  already "Adventure Grandmaster ... would be a neat trick!", and 35 points (still "rank amateur" in Woods' FORTRAN)
  was "novice class". Knuth's erratum for page 382, reported by Q P Liu (O'Dwyer's same fix in his ODWY0350:
  Quuxplusone/Advent commit 531e061).
- **Fix 2: the liquid in a carried bottle when the cave closes.** The closing destroyed everything carried through
  drop(), which took the water (never counted as carried) off the count of things carried: the count went to -1 and the
  endgame let you carry eight things. Woods' bug, carried over. Knuth's erratum for pages 370-371, on the recommendation
  of Arthur O'Dwyer (his own fix in ODWY0350: commit ea27c76).
- **Fix 3: the rest of an over-long line is dropped.** fgets() left what did not fit in the 72-byte buffer in the input,
  where it became the next command (a long line ending in "north" moved you). Whether a line filled the buffer is told
  by a newline put in its next-to-last byte before each read, which fgets overwrites only if it did; searching for the
  newline would stop at a NUL typed in the line and throw the next line away.

## --debug

With `--debug`, a line starting with `#` is a command to the port, not to the game (without it, `#show` is just a word
the game does not know). The tests use them to set up states instead of playing to them.

    #show               location, things carried, tally, lost treasures, dflag, clock1, clock2, deaths, bonus, score
    #loc N              go to location N (1 to 140)
    #where OBJ          place and prop of an object (its number, or a word that names it)
    #put OBJ PLACE      move it: -1 carried, 0 nowhere, 1 to 140 a location (not WATER or OIL: they go with the bottle)
    #prop OBJ N         set its prop, within its descriptions in advent.w (and -1, "not yet found", for a treasure;
                        the plant takes 0, 2 or 4)
    #set VAR N          tally, lost, dflag, clock1, clock2, deaths (0 to 2), bonus, limit

The numbers are those of advent.w's enums; `tests\common.py` reads them from there. A number that would index past
the program's tables is refused with a `# ?` line saying what is allowed. The commands can still make states the game
never makes; they are test tools.

## Tests

    python tests\regress.py        options, end of input, long lines and NULs, SAY, Fixes 1-3 with and without
                                   --no-fixes, then win.py and a short fuzz
    python tests\win.py            350 of 350 and "Adventure Grandmaster" (with and without --no-fixes): --debug puts the
                                   treasures in the building and starts the closing clocks; the program then closes
                                   the cave and the repository endgame is played to the blast
    python tests\fuzz.py [N]       N random sessions of the game's own words, long lines and y/n (default 100 x 400
                                   commands): no crash, no hang, exit 0
    python tests\consoleplay.py    at a real console (a pseudo console): prompts drawn before each read, lower case,
                                   a long typed line, QUIT, Ctrl-Z Enter
    python tests\crosscheck.py [N] the same C built on Linux (WSL Debian gcc) with AddressSanitizer and
                                   UndefinedBehaviorSanitizer: N seeded random sessions (default 60 x 400), with 8-bit
                                   bytes, NULs and, in half of them, --debug commands with numbers out of range; the
                                   transcripts must be identical and neither build may report anything

## Checked (refine pass, 2026-09-21)

- regress: all passed; win: 350/350 both ways; fuzz: 300 sessions of 500 commands, 0 failed; consoleplay: all passed;
  crosscheck: 60 sessions of 400 commands identical, no sanitizer report. The first crosscheck run found item 4.
- The sanitizer build runs under `setarch -R`: gcc 12's AddressSanitizer and the WSL2 kernel's mmap randomness do not
  get on, and now and then a run loops printing "AddressSanitizer:DEADLYSIGNAL".

## Still to do (refine pass)

- Nothing in the port. Two things Knuth might like to hear (his errata pay 0x$1.00): SAY with an unknown word reads
  `hash_table[-1]` in his 2026 text too, and that text lacks his own vase erratum of 3 September 2012 (and four
  cosmetic ones). The report, `..\knuth_erratum_report.txt`, was mailed to Knuth by the user on 2026-09-22.
