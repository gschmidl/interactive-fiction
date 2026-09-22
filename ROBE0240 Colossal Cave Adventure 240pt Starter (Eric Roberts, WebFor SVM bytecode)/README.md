# Colossal Cave Adventure 240pt Starter - Eric Roberts (Stanford), browser edition

**Status: PORTED 2026-09-19 (first pass) - `port\starter.exe` is a C implementation of the SVM running `Small.js`;
`port\play.bat`, see `port\README.md`; refinement pending.** Files here are verified copies (md5) of the originals
named below.

Source: eXo's `Colossal Cave Adventure (1976)\0665-Point Adventure\Browser\` (eXo's `0240-Point Adventure\Browser` is
the identical folder). This project = **Small.js / const SMALL - "Starter Adventure (240 points)"**; the sibling folder covers the other game.

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

## Related source
The FORTRAN source of the 665-point game is in the sibling folder (`src_original/ROBE0665`, from
github.com/Quuxplusone/Advent). No separate source for the 240-point Starter is known; `Small.js` carries the same symbol
names and almost the same text, so it is most likely the same program built with a reduced configuration - compare the
two SVM images against `newadv.F` / `text.dat` to find the switch before choosing between an SVM port and a rebuilt source.
