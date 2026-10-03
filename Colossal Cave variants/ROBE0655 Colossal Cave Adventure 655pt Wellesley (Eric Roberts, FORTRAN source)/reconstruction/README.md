# Colossal Cave Adventure 655pt Wellesley - Eric Roberts (Stanford), FORTRAN source V6.2

**Status: PORTED 2026-09-19 (first pass) - `port\run.bat` runs `newadv.exe`, Roberts' FORTRAN source compiled; see
`port\README.md`; refined 2026-09-21 (Fix 1: SAVE forgot what was in the containers).** Split off the ROBE0665
folder on 2026-10-03: the source is V6.2 with 655 points, the browser edition (ROBE0665) is V6.4.2 with 665. Files
here are verified copies (md5) of the originals named below.

## FORTRAN source
`src_original/ROBE0665/` = https://github.com/Quuxplusone/Advent/tree/master/ROBE0665 at commit d38e82550600144e3547d6472bc80dbf49ca214b
(2026-09-15), complete directory, fetched with a sparse clone; `Quuxplusone-Advent_README.txt` is that repository's README.
The folder keeps its upstream name, although the program in it is the 655-point V6.2.
It is Roberts' own Unix source tree ("Makefile for newadv directory"): `newadv.F` (main), `aparse.F` (parser), `advlib.F`,
`wizard.F`, `setup.F` (database compiler), the `*com.h` COMMON includes, `text.dat` (the database), `data.c` (database
compiled to C), C helpers `csub.c readln.c busy.c`, notes `advmap.txt treasures.txt bug.txt billboard.txt`, plus stale
`.o` files and a `busy` binary. Builds with gfortran `-std=legacy -ffixed-line-length-none`.

## What it is
The Wellesley College *Colossal Cave*: "additional modifications and extensions have been made at Wellesley College
by Mark Edwards, Mark Sylvester and, most recently, Eric Roberts" (the credits in `text.dat`). The banner reads
`-- ADVENTURE (V6.2),  3-Mar-10 --`, and SCORE counts to 655.

## Related
The sibling ROBE0665 folder holds the browser edition (V6.4.2, 665 points), Roberts' later revision compiled with his
WebFor for his SVM stack machine, and `port\TEXTS_V62_V642.md` there compares the two texts. The browser edition was
used as a reference while porting this source: it is WebFor-compiled from the same FORTRAN, and the symbol names match.
