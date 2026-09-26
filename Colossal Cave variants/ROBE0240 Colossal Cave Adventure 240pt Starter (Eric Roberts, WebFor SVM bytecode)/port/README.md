# Starter Adventure (Eric Roberts, 240 points) - Windows console port

No source of the Starter Adventure is known: it exists only as `Small.js`, the program WebFor (Roberts' FORTRAN for
the web) compiled to his stack machine, SVM. `starter.exe` is a C implementation of that machine and of the WebFor
run-time library, with `Small.js` embedded; it runs the compiled program unchanged. Run `run.bat` - SAVE writes
and RESTORE reads `saves\newadv.txt` (the browser offered the same text as a download and read it back with a file
picker).

    starter [--seed N] [--image FILE.js] [--no-fixes] [--echo | --no-echo]

`--seed N` fixes the random numbers (the browser used `Math.random`), `--image` runs another compiled image - the
Wellesley Adventure's `Big.js` runs as well - and `--echo` repeats typed lines in the output (the default when input
is not a console, so transcripts read like the browser page).

`Small.js` and `Big.js` are two builds of one engine: their 20193-word main programs have the same instruction
sequence (only operands differ, where string and subroutine addresses shift), and their reachable code is 60325 vs
60423 words; the databases differ. The Starter reports `-- ADVENTURE (V2.1) --`, the Wellesley `(V6.4.2)`.

## The machine (`src/svm.c`)

Written from the browser edition's own JavaScript (in the sibling ROBE0665 folder, `archive_original\Browser\js\edu\
stanford\cs\`): `svm.js` (instructions, frames, Core/Console/Global classes), `webfor.js` (WFLib), `exp.js` (Value:
integers and doubles are JavaScript numbers, `toString` as JavaScript prints them), `utf8.js` (strings are packed
big-endian into the code array). The compiled program uses 33 of the 58 instructions and 28 library methods; all of
them are implemented with the JavaScript's semantics, including its run-time errors, which stop the machine as they
stopped the page. A reachability disassembly found no other instruction in either image; anything else stops with
"not implemented". Memory is a small mark-and-sweep heap collected between instructions.

Two places where the page's behaviour was decided rather than copied:

- `WFLib.exit` (FORTRAN STOP) ends the program. java2js ignores `System.exit()`, so the page ran on: into `END` after
  the final "[Hit return to exit]", or back out of the fatal-error routine `BUG`.
- Ordered string comparisons are `localeCompare` in the page. The program uses them only to classify a typed character
  against 'A'..'Z', 'a'..'z', '0'..'9'; for ASCII the browser's collation and byte order put exactly the letters and
  digits in those ranges, so byte order is used (`-W` reports such a comparison).

## FIX 1 (on unless `--no-fixes`)

WebFor gives every FORTRAN array an element 0 holding `undefined`. The parser reads `VERBS(0)` (and similar) when a
command starts with a preposition or ALL, and arithmetic on `undefined` is a run-time error: in the browser the game
stops responding after `at`, `with rod`, `from`, `off`, `all` ... (checked in the page: it shows "Illegal to apply
IDIV to undefined and 1000" and ignores all further input). The FORTRAN program read whatever preceded the array and
answered "I'm afraid I don't understand." With the fix element 0 reads as 0; `--no-fixes` restores the error (the
port prints it and exits).

## Checked so far (first pass, 2026-09-19)

- **Against the real browser edition**: the page served locally and driven through its own console input path with
  `Math.random` replaced by the port's seeded generator (`tests\browser_harness.js`): a 150-command random session
  (`tests\cmds11.txt`, seed 11, 58 commands until the game ended with a pit death) gives a byte-identical transcript
  to `starter.exe --seed 11 --no-fixes --echo` (SHA-256 90241c13...; `tests\seed11_browser_transcript.txt`).
- 20 random 500-command sessions (10 Starter, 10 Wellesley) run without a run-time error with FIX 1.

## Still to do (refine pass)

- Refine pass 2026-09-21: no open bug here; the rest is deferred (user: bugs and fuzzing only).
- Deferred: longer browser comparisons, Starter and Wellesley, including SAVE/RESTORE and reading the billboard;
  winnability via a debug mode; ConPTY console check.
- Known difference, left as is: non-ASCII input. The page's collation would count accented letters as letters; the
  port treats their bytes as separators. No word of the game has one, so either way such a word is unknown; only how
  a line is cut into words differs.
