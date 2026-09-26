# ADVENT - Don Woods and Donald E. Knuth, CWEB, 1998 (rev. March 1999) (KNUT0350)

**Status: PORTED 2026-09-19, refined 2026-09-21 - `port\advent.exe`, see `port\README.md`.** Winnable (350/350 via
`--debug`); the same C built on Linux with AddressSanitizer and UndefinedBehaviorSanitizer gives identical transcripts.
Staged 2026-09-19: complete copy of the named directory of https://github.com/Quuxplusone/Advent at commit d38e82550600144e3547d6472bc80dbf49ca214b (2026-09-15), md5-verified against the clone.

`src_original\KNUT0350\`: `advent.w` (167 KB literate source, "Copyright 1998 by Donald R Woods and Donald E Knuth - Don't
change this file without the authors' permission!"), `advent.pdf` (the woven document), `Makefile` (ctangle -> C).
A complete rewrite of the 350-point game, faithful to Woods' FORTRAN down to the messages. The copy is Knuth's January
2010 text with his errata to 2014 (the "three bugs" of O'Dwyer's README are Knuth's own cosmetic errata of March 2011,
corrected here). Knuth's later errata (2018-2026) are not in it; the port's change file makes the ones that matter
(the ranks, the closing's liquids, a read past the input buffer) and one more of its own: SAY with an unknown word
read `hash_table[-1]`. Port = `ctangle advent.w advent-win.ch`, compile; advent.w itself is never edited.
