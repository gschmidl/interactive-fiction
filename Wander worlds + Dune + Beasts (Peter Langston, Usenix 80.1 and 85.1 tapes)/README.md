# Peter Langston's games on the Usenix distribution tapes

**Status: STAGED 2026-09-19 - not ported, nothing built.** Files here are verified copies (md5) of the originals named below.

Sources: `F:\bits\Usenix\usenix_80.1_tp.tap` (tp format; `boulder/dpw/wand/...`, `boulder/dpw/emp/...`) and
`F:\bits\Usenix\usenix_85.1.zip` (tar; `langston/vax/` and `langston/sun/`). `src_original\85.1\` = the WAND, DUNE and
BEASTS directories cut from the 85.1 tar.

- WANDER (1974-): 85.1 has `WAND/castle.wrld + .misc`, `advent.wrld + .misc`, `wander.c wandglb.c wandsys.c wanddef.h`,
  `wand1.o wand2.o` (objects only for the core), `wander.nr`. 80.1 has the earlier `wand` directory incl. the a3 world
  (`t-damsel m="pretty unchivalrous! (puce smoke)"`) and `libr...`. eXo has only Castle (1974) as a DOS rebuild; the other
  worlds (a3 = Aldebaran III, advent, library, tut) are the un-had part. Wander was restored from these tapes in 2015
  (Anthony Hay) - check that work before doing anything.
- DUNE: `dune.o`, `duneglb.c`, `dune.doc`, `dune.6` - object only ("D U N E (with apologies to Frank Herbert) You are in a
  strange, barren world consisting of nothing but sand dunes")
- BEASTS: `beasts.o bsnoop.o beaglb.c beastfile questfile` - object + data
The 80.1 tape has not been unpacked (tp format, 512-byte blocks, directory at block 1).

## Update 2026-09-19 - what eXo already has, and what is left
eXo runs three Wander worlds with one DOS interpreter (`WANDER.EXE`, DJGPP, identical md5 in all three):
`E:/EXO/Castle (1974)`, `E:/EXO/Aldebarran III (1977)` (a3), `E:/EXO/Library (1978)` - each `MS-DOS/drives/c/` with `<world>.wld` + `.msc`.
The 80.1 tape's second file is a binary cpio archive (not tp); `work/bcpio.py <tape> <path-substring> <outdir>` unpacks it.
`src_original/80.1/boulder/dpw/wand/` = the complete "Export Wander Tape" of 29 Jan 1980: READ_ME, `a3 castle library tut`
worlds (.wrld + .misc), `wanddef.h wandglb.c`, PDP-11 objects `wander.o Fwander.o NFwander.o`, nroff docs
`wander.nr wrld.nr misc.nr wandaid.nr mac`.
Compared with eXo (CR stripped): castle.wrld identical (misc 2 lines differ); library.wrld 29 lines / misc 60 lines differ;
**a3 is a different, much shorter edition** (25,033 bytes on the 1980 tape against eXo's 44,543 - 1031 differing lines).
Not in eXo at all: **tut** (1978, a tutorial on binary logic written as a world, 4.4 KB) and **advent** (85.1 tape,
4.7 KB, the start of a Colossal Cave world).
So nothing needs porting: the job is to try `tut`, `advent` and the 1980 `a3`/`library` editions under the existing
WANDER.EXE (rename .wrld/.misc to .wld/.msc) and add the ones that load as eXo entries. The interpreter already accepts
1980-format files, because its castle is byte-for-byte the 1980 one. Dune and Beasts remain object-only.

## Objects and the rest of the 1985 distribution (added 2026-09-19)
`src_original/85.1/langston/` is now the complete `langston/` tree of the 85.1 tape (744 files, 3.8 MB; `vax/` and `sun/`
builds): besides WAND, DUNE and BEASTS it has GLIB and TCAP (the libraries Dune links against, see DUNE/READ_ME), EMP
(Empire), CONVOY, WAR, BOG, BOLO, GOMOKU, GRID, MM, ORACLE, SD, FF and others.
The `.o` files are kept in git on purpose - they are the only form these programs were released in:
- 80.1 `wander.o`, `Fwander.o`, `NFwander.o`: PDP-11 a.out objects, `@(#)wander.c 2.5 WITH FTELL() last mod 12/27/79 --
  (c) psl 1978` - the 1979 Wander interpreter itself (with / without `ftell`).
- 85.1 `WAND/wand1.o`, `wand2.o` (VAX and Sun): the 1984-85 interpreter core; only `wander.c`, `wandglb.c`, `wandsys.c` came as source.
- 85.1 `DUNE/dune.o`: `@(#)dune.c 1.7 2/6/84 -- (c) psl 1980` - a termcap screen game about hauling spice home, not a parser adventure.
- 85.1 `BEASTS/beasts.o`, `bsnoop.o`: "Welcome to the bestiary!" - the animal-guessing game with `beastfile` / `questfile` data.
