# ADVENT - Don Woods and Donald E. Knuth, CWEB, 1998 (rev. March 1999) (KNUT0350)

**Status: STAGED 2026-09-19 - not ported, nothing built.** Complete copy of the named directory of https://github.com/Quuxplusone/Advent at commit d38e82550600144e3547d6472bc80dbf49ca214b (2026-09-15), md5-verified against the clone.

`src_original\KNUT0350\`: `advent.w` (167 KB literate source, "Copyright 1998 by Donald R Woods and Donald E Knuth - Don't
change this file without the authors' permission!"), `advent.pdf` (the woven document), `Makefile` (ctangle -> C).
A complete rewrite of the 350-point game, faithful to Woods' FORTRAN down to the messages; O'Dwyer's README lists
three bugs he found in the 2010 text. Port = `ctangle advent.w`, compile; do not edit advent.w - use a change file.
