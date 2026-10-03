# Wellesley Adventure V6.4.2 (Eric Roberts) - Windows console port of the browser edition

| launcher | program | revision |
|---|---|---|
| `run.bat` | `wellesley.exe` - the browser edition's compiled program (`..\archive_original\Browser\Big.js`, database dated 7 June 2021) run by a C implementation of Roberts' SVM | `-- ADVENTURE (V6.4.2) --`, "a possible 665" |

No source of V6.4.2 is known. Roberts' FORTRAN source of the earlier 655-point V6.2 is ported in the sibling ROBE0655
folder.

## wellesley.exe

Built by `build.bat` / `build.sh` from `Big.js` and `..\..\..\ROBE0240 ...\reconstruction\port\src\svm.c`, the SVM
implementation written for the Starter Adventure, which has no other form; see that folder's `port\README.md` for the
machine, its options (`--seed`, `--no-fixes`, `--echo`) and FIX 1 (the browser edition stops dead after commands such
as `at` or `all`; with the fix it answers as the FORTRAN program does). SAVE and RESTORE use
`saves\wellesley\newadv.txt`.

## Checked so far (first pass, 2026-09-19)

- Against the real browser page (served locally, `Math.random` seeded like the port, lines fed through the page's own
  console input path - `..\..\..\ROBE0240 ...\reconstruction\port\tests\browser_harness.js`) a random session
  (`tests\wellesley_cmds21.txt`, seed 21, 32 commands until a death ended the game) gives a byte-identical transcript
  to `wellesley.exe --seed 21 --no-fixes --echo` (`tests\wellesley_seed21_browser_transcript.txt`).

## The texts of V6.2 and V6.4.2

`tests\textdiff.py` compares the two, line by line, into `TEXTS_V62_V642.md` (every changed line with its V6.2
section and number, what only one of them has, and the vocabulary); V6.2's `text.dat` is read from the sibling
ROBE0655 folder. V6.2's `text.dat` calls itself 6.3.2 in its first line (VERSN, SUBVER, BUGFIX; the banner prints
only VERSN.BUGFIX). Of its 2,800 lines of text, 2,636 are in V6.4.2 unchanged. What changed:

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
