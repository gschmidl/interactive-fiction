# Colossal Cave Adventure 665pt Wellesley - Eric Roberts (Stanford), browser edition

**Status: PORTED 2026-09-19 (first pass) - the browser edition's Big.js (V6.4.2, 665 points), `port\run.bat`, run by
the SVM implementation in the ROBE0240 folder; see `port\README.md`.** On 2026-10-03 the FORTRAN source (V6.2, 655
points) moved to its own folder, ROBE0655. Files here are verified copies (md5) of the originals named below.

Source: eXo's `Colossal Cave Adventure (1976)\0665-Point Adventure\Browser\` (eXo's `0240-Point Adventure\Browser` is
the identical folder). This project = **Big.js / const BIG - "Wellesley Adventure (665 points)"**; the ROBE0240 folder
covers the other game.

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
WebFor's file layer (browser storage) - find those ops first. Route (a) was taken: the SVM in C is in the ROBE0240
folder.

## Related source
Roberts' FORTRAN source (Quuxplusone/Advent's `ROBE0665` directory) is the earlier V6.2 with 655 points, not this
revision; it is ported in the sibling ROBE0655 folder. No source of V6.4.2 is known. `port\TEXTS_V62_V642.md`
compares the two texts.
