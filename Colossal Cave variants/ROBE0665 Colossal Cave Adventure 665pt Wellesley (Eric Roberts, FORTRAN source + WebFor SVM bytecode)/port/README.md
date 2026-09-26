# Wellesley Adventure (Eric Roberts) - Windows console ports of two revisions

| launcher | program | revision |
|---|---|---|
| `run.bat` | `newadv.exe` - Roberts' FORTRAN source (`..\src_original\ROBE0665`, newadv.F last modified 3 March 2010) compiled with gfortran | `-- ADVENTURE (V6.2),  3-Mar-10 --`, "a possible 655" |
| `play-wellesley.bat` | `wellesley.exe` - the browser edition's compiled program (`..\archive_original\Browser\Big.js`, database dated 7 June 2021) run by a C implementation of Roberts' SVM | `-- ADVENTURE (V6.4.2) --`, "a possible 665" |

The folder's "665" is the browser edition; the FORTRAN source is the earlier 655-point V6.2 (its `setup` counts 625).
No source of V6.4.2 is known.

## newadv.exe (the FORTRAN source)

SAVE and RESTORE use `saves\newadv.sav`. `build.bat` / `build.sh`, following Roberts' Makefile (gfortran
`-std=legacy -ffixed-line-length-none`, gcc `-fcommon`, statically linked):

1. `allcom.F` includes every COMMON block; `nm` of its object gives the block sizes (`common.h`, which comes out
   identical to the one in Roberts' tree) and `volatile.fg` picks the blocks SAVE writes (`volatile.h`). `pcom.h` is
   left out: it is a stray list of the `params.h` names, not a COMMON include.
2. `setup.exe` (setup.F + advlib.F) compiles `text.dat` into the COMMON blocks and `common.c` dumps them as `data.c`.
   The result is byte-identical to Roberts' own `data.c` except the build date setup stamps into `VERDAT`; the build
   checks that and then compiles his `data.c`, which carries " 3-Mar-10".
3. `newadv.exe` = newadv.F + advlib.F + aparse.F + data.c + csub.c (wizard.F is not linked, as in the Makefile, so
   there are no prime-time restrictions).

Changes, all marked `PORT:`:

- `csub.c`: save files are opened in binary mode, and Windows headers replace `<sys/file.h>`.
- `csub.c`: the program never seeds `rand()`, so every game drew the same numbers from the C library. Roberts built on
  Mac OS X (the leftover objects in his tree are Mach-O), whose `rand()` is Park-Miller "minimal standard" with
  RAND_MAX 2^31-1; `crand_()` now uses that generator (seed 1) instead of the Windows C library's 15-bit one.
- `newadv.F`: the billboard and notebook were read from `/usr/games/billboard.txt` and `/usr/games/notebook.txt`; they
  are now read from the program's own directory (new `cgamdir_()` in csub.c). `billboard.txt` ships beside the .exe;
  there is no notebook file, so reading the notebook gives the game's own "blank" message as before.
- `csub.c`: options, read before the FORTRAN program starts: `--no-fixes` (the fix below off), `-h`/`--help`; any
  other `--option` is refused, exit 2.

**Fix 1 (newadv.exe; off with `--no-fixes`): SAVE forgets what is in the containers.** `volatile.fg` lists `hdlcom`,
but the block is `/HLDCOM/` (HOLDER and HLINK), so SAVE never wrote it and a restored game got the containers as they
were at the start: pour out the bottle, SAVE, RESTORE, and it is full of water again. The build now also extracts
HLDCOM's entry (`holder.h`), and SAVE appends that block and RESTORE reads it back. A game saved with `--no-fixes`
lacks it, and restoring one leaves the containers as they are.

`CALL MOVE(AMULET,299,0)` and `CALL MOVE(SCROLL,LOC,0)` pass three arguments to the two-argument `MOVE` (gfortran
warns): the third is never read, so they do what `MOVE(AMULET,299)` does.

## wellesley.exe (the browser edition)

Built by the same `build.sh` (step 4) from `Big.js` and `..\..\ROBE0240 ...\port\src\svm.c`, the SVM implementation
written for the Starter Adventure, which has no other form; see that folder's `port\README.md` for the machine, its
options (`--seed`, `--no-fixes`, `--echo`) and FIX 1 (the browser edition stops dead after commands such as `at` or
`all`; with the fix it answers as the FORTRAN program does). SAVE and RESTORE use `saves\wellesley\newadv.txt`.

## Checked so far (first pass, 2026-09-19)

- newadv.exe: opening, pantry, poster, T. Sawyer's note, pit death and reincarnation, SAVE / RESTORE round trip. A
  Linux build (gfortran 12, glibc) of the same sources gives byte-identical transcripts for 8 random 400-command
  sessions.
- wellesley.exe: against the real browser page (served locally, `Math.random` seeded like the port, lines fed through
  the page's own console input path - `..\..\ROBE0240 ...\port\tests\browser_harness.js`) a random session
  (`tests\wellesley_cmds21.txt`, seed 21, 32 commands until a death ended the game) gives a byte-identical transcript
  to `wellesley.exe --seed 21 --no-fixes --echo` (`tests\wellesley_seed21_browser_transcript.txt`).

## Refine pass (2026-09-21)

- Fix 1 checked both ways (the emptied bottle after SAVE and RESTORE); the options.

## The texts of V6.2 and V6.4.2

`tests\textdiff.py` compares the two, line by line, into `TEXTS_V62_V642.md` (every changed line with its V6.2
section and number, what only one of them has, and the vocabulary). V6.2's `text.dat` calls itself 6.3.2 in its
first line (VERSN, SUBVER, BUGFIX; the banner prints only VERSN.BUGFIX). Of its 2,800 lines of text, 2,636 are in
V6.4.2 unchanged. What changed:

- **The outdoors** ("7-Jun-21 Changed outdoor geography to make navigation easier", says V6.4.2's billboard): the
  road now goes on west towards a grassy knoll instead of ending in impassable forest; the forest has a tree you can
  climb, and a buzzing in its branches; the stone spire's doorway "shimmers slightly", and a magical force blocks some
  ways in; the marsh is closed ("It's too annoying").
- **The bees:** the Flower Room (193) is gone, and with it the swarm over the fresh flowers and the inventory name
  "Bumblebees"; the beehive is simply "here", its bees guarding it in new words.
- **A good-luck charm** near the building: worn, it keeps the dwarves away and makes the random passages
  deterministic. New words (the game keeps five letters): ACCIO, BRINK, CERBE, MELLO, PALAN, REPLA, WZGET.
- **Wording:** the compass abbreviations are spelled out (NE, SW ... become northeast, southwest), so many long
  descriptions are wrapped anew; commas before "and"; "Persian" capitalised; the rank names in single quotes;
  "Master's Section" becomes "endgame"; the credits add Kristin Powers; the photographs show women "wearing Wellesley
  T-shirts riding mopeds" rather than riding motorcycles; "7, 22, 34" becomes "7-22-34".
- **The instructions** (message 51) are rewritten for the browser - SAVE downloads the game to a file, RESTORE loads
  one - and lose the paragraphs on hints and the billboard; the WELCOME TO ADVENTURE box (602) is drawn wider.
- **The carved picture** (message 467) has lost every backslash - the only ones in V6.2's text - so it comes out
  broken in the browser edition (`_/_\_` is printed `_/__`, `\ \_/ /` is printed ` _/ /`): dropped as escape
  characters on the way into the page's strings, not an edit.
- **New program messages** of V6.4.2's parser and page: "That verb requires an object.", "I don't understand what
  you want to do with that.", "The file was saved from a different version.", "Your health rating is", "Time
  passes.", "[Hit return to exit]" and a few more.

The image keeps each string once and in no text order, so V6.4.2's lines are placed only by their likeness to
V6.2's; a changed line is paired with its nearest new one (compass abbreviations spelled out first).

## Still to do (refine pass)

- Deferred (user, 2026-09-21: bugs and fuzzing only): longer browser comparisons, a ConPTY check, winnability via a
  debug mode.
