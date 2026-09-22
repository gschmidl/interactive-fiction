# Colossal Cave Adventure 665pt Wellesley - Eric Roberts (Stanford), browser edition

**Status: PORTED 2026-09-19 (first pass) - two revisions: the FORTRAN source (V6.2, 655 points, `port\play.bat`)
and the browser edition's Big.js (V6.4.2, 665 points, `port\play-wellesley.bat`, run by the SVM implementation in
the ROBE0240 folder); see `port\README.md`; refined 2026-09-21 (Fix 1: SAVE forgot what was in the
containers).** Files here are verified copies (md5) of the
originals named below.

Source: `E:\EXO\Colossal Cave Adventure (1976)\0665-Point Adventure\Browser\` (eXo's `0240-Point Adventure\Browser` is
the identical folder). This project = **Big.js / const BIG - "Wellesley Adventure (665 points)"**; the sibling folder covers the other game.

## What it is
`index.html` offers "a Starter Adventure reminiscent of the early releases and a larger Wellesley Adventure, available
online for the first time". Credits in the text: "made at Wellesley College by Mark Edwards, Mark Sylvester, and ..."
"Mark Sylvester, Eric Roberts, and Kristin Powers", mail to eroberts@cs.stanford.edu.
Neither game is JavaScript source. `Big.js` / `Small.js` are arrays of ~95,000 32-bit words: compiled code for Roberts'
**SVM** stack machine, produced by **WebFor** (his FORTRAN-for-the-web compiler), run by `js\edu\stanford\cs\svm.js`
(3399 lines, opcodes in `svmops.js`) with the FORTRAN runtime in `webfor.js` and a Java-to-JS support layer
(`java2js.js`, `js\java\*`, `js\javax\swing.js`, console in `jsconsole.js`). `NewAdv.js` wires the two buttons:
`svm.setCode(BIG | SMALL); svm.run()`.
Both images contain the whole program *and* the whole text database, plus the FORTRAN symbol names (CLOSNG, TRVSIZ,
NEWLOC, DROPVB, TABNDX, ATHAND...). They differ in only ~190 of ~2900 strings (Starter: "Bumblebees", bear "gobbles down
the food"; Wellesley: honeycomb/beehive, pantry, overgrown roadway, Grandmaster message), so the 240/665 split is mostly
in code and tables, not in text.

`work\decode_svm.py <Browser> <out>` rebuilds `decoded\Big.bin / Small.bin` (words written big-endian; strings are 4
characters per word) and `*_strings.txt`.

## What a port needs
Either (a) re-implement the SVM in C and run the word arrays unchanged - the faithful route, svm.js is the spec; or
(b) decompile SVM back to FORTRAN using the embedded symbol table. Console I/O is line based; SAVE/RESTORE goes through
WebFor's file layer (browser storage) - find those ops first.

## FORTRAN source (added 2026-09-19)
`src_original/ROBE0665/` = https://github.com/Quuxplusone/Advent/tree/master/ROBE0665 at commit d38e82550600144e3547d6472bc80dbf49ca214b
(2026-09-15), complete directory, fetched with a sparse clone; `Quuxplusone-Advent_README.txt` is that repository's README.
It is Roberts' own Unix source tree ("Makefile for newadv directory"): `newadv.F` (main), `aparse.F` (parser), `advlib.F`,
`wizard.F`, `setup.F` (database compiler), the `*com.h` COMMON includes, `text.dat` (the database), `data.c` (database
compiled to C), C helpers `csub.c readln.c busy.c`, notes `advmap.txt treasures.txt bug.txt billboard.txt`, plus stale
`.o` files and a `busy` binary. Builds with gfortran `-std=legacy -ffixed-line-length-none`.
**So the 665 is a source port, not an SVM job**: build this, and use the browser edition (WebFor-compiled from the same
FORTRAN, symbol names match) as the reference transcript generator. Check whether `text.dat` here and the text inside
`Big.js` are the same revision before trusting either as the reference.
