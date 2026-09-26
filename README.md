# AI-ASSISTED PROJECTS
are here at github

# NON AI-ASSISTED PROJECTS
are at https://codeberg.org/gschmidl/interactive-fiction.

# STATUS

The games are filed in category folders (`Colossal Cave variants/`, `PDP-10 games/`, `Type-Ins/`, ...), and every
folder inside a category is one game. A game folder with a `port/` directory (or, for the type-ins, the rebuilt
program itself) is a finished recovery or port; its own README says how it was done and how it was checked. The shared
working trees are in `_work/`, which is not published apart from the tools in `_work/tools/`.

## THE 2026-09 PORTING PLAN (DONE)

The folders below were staged on 2026-09-19 and worked through in this order, least effort first, by 2026-09-22;
nothing on the list is still in progress. Each folder's README starts with its status and says how the game was
recovered and checked (the working notes, `_work/_bits_sweep_work/PORT_PLAN.md`, are not published).

The order, with what came of each:

1. `Colossal Cave variants/HOWE0301 Adventure! 301pt (Atari 8-bit BASIC, Robert Howell, 1982)` - emulation only (no port)
2. `Colossal Cave variants/KNUT0350 Colossal Cave Adventure 350pt (Knuth CWEB, 1998)` - ported and refined: Knuth's later errata as fixes, a
   `hash_table[-1]` read in SAY found by UBSan, identical to a sanitized Linux build
3. `Colossal Cave variants/POHL0350+DAIM0350 Colossal Cave Adventure 350pt (Jerry Pohl C port 1984 + Daimler Turbo C 1990)` - ported and
   refined: five original bugs fixed, Daimler's port identical to his own DOS program under DOSBox
4. `Colossal Cave variants/HALL0501 Colossal Cave Adventure 551pt (Robert Hall's C version 7.0, MINIX, 1994)` - ported and refined: a
   RETREAT crash and three game-ending bugs fixed, RESTORE hardened
5. `Miscellaneous games/Wander worlds + Dune + Beasts (Peter Langston, Usenix 80.1 and 85.1 tapes)` - emulation only (no port)
6. `Colossal Cave variants/ROBE0665 Colossal Cave Adventure 665pt Wellesley (Eric Roberts, FORTRAN source + WebFor SVM bytecode)` - ported
   and refined: SAVE now keeps what is in the containers
7. `Colossal Cave variants/ROBE0240 Colossal Cave Adventure 240pt Starter (Eric Roberts, WebFor SVM bytecode)` - ported
8. `Prime-50 games/Dungeon by Jonathan Reed (Prime 50-Series, PL1G, full source)` - ported and refined: REMEMBER's free
   coordinates no longer index outside the dungeon
9. `Colossal Cave variants/MOOR0350 Colossal Cave Adventure 350pt (IBM MVS, MSU FORTRAN, CBT COV466)` - ported and refined: SUSPEND
   after RESTORE no longer loses the game, CARRY no longer walks off its list
10. `Colossal Cave variants/HORV0350 Colossal Cave Adventure 350pt (SEL 32, RTM, 1979 printout, full source)` - ported; refine-pass
    fuzzing found nothing to fix
11. `Colossal Cave variants/PRIM0350 Colossal Cave Adventure 350pt (Prime 50-Series, PRIMOS FORTRAN, full source)` - ported;
    refine-pass fuzzing found nothing to fix
12. `Colossal Cave variants/SCOP0366 Colossal Cave Adventure 366pt with palantir (CDC NOS 1.3, ACCA, FORTRAN source)` - ported
13. `Colossal Cave variants/JAZE0350 and JAZE0500 Colossal Cave Adventure 350pt + 500pt castle (CDC Cyber 74, MCAUTO, FORTRAN source)` - ported
14. `Dungeon and Zork variants/Qork V3.0A (CDC NOS, 1984, FORTRAN source)` - ported last (first pass), seven sessions identical with the
    original compiled by FTN5 on NOS 2.8.7
15. `Miscellaneous games/Dave's Dungeon - DAVESCAVE (PDP-11 TRAX, MITRE, FORTRAN source)` - ported
16. `Colossal Cave variants/PLAT0550 Colossal Cave Adventure 550pt original (Xerox Sigma CP-V, David Platt, full source)` - ported (first pass)
17. `Miscellaneous games/Mystery Mansion (HP 1000, RTE, FORTRAN source)` - ported (first pass)
18. `Dungeon and Zork variants/Dungeon (HP 1000, RTE, FORTRAN source)` - ported (first pass)
19. `Colossal Cave variants/ARNA0660 Colossal Cave Adventure 660pt Glaxo 4.3 (Prime 50-Series, A-code, 1984)` - ported (first pass), eight
    sessions identical with the original on PRIMOS
20. `Colossal Cave variants/VULC0350 Abenteuer (Harris VULCAN, German Colossal Cave, full source)` - ported (first pass); its set-up
    equals the site's own saved game (NEUSPIEL, 1980) in every variable
21. `Miscellaneous games/Unnamed volcano adventure (NorthStar Horizon BASIC, DBACK)` - ported (first pass); a complete program
    (renamed 2026-09-21 from "..., orphaned fragment)")
22. `Dungeon (Prime 50-Series, PRIMOS, binary)` - Reed's own runfile, a duplicate: merged into 8 and removed
23. ~~`(ANON0550) Colossal Cave Adventure 550pt (VAX VMS 1.5, 1979, exe only)`~~ - not portable (no executable on
    the kit, only the page file's copy of a 1979 session's text, and it is the 350); folder deleted by the user
    2026-09-21
24. `Colossal Cave variants/TIDX0350 Colossal Cave Adventure 350pt (TI 990, DX10, binary)` - ported (2026-09-21): the original task on
    an emulated TI 990/10 with DX10's supervisor calls; three sessions identical with the original on sim990 +
    DX10 3.7
25. `Colossal Cave variants/PP7K0350 Colossal Cave Adventure 350pt (Philips P7000, Four-Phase IDOS)` - ported (2026-09-21): the
    site's own IDOS disc pack on an emulated Four-Phase IV/70 (`_work/_FourPhase_work`); then
    `Quest Variants/Quest (Philips P7000, Four-Phase MFE, multiplayer)` - ported (2026-09-22): the same pack on an emulated
    IV/90 Model 2, MFE/7000 started as the operator did; up to six players, each window a terminal over TCP

After the plan, the last two (2026-09-22):
- `Dungeon and Zork variants/Dungeon (Burroughs B7700, MCP, FORTRAN source)` - ported: the B7700 FORTRAN source (DECUS Dungeon V1.2c code,
  V2.0 text) that the HP 1000 Dungeon (18) came from, on the same library's release 2213 tape
- `Miscellaneous games\Mystery Mansion (HP 1000, RTE, FORTRAN source)\port\rev9` - runs: revision 9, compiled, on the site's own
  RTE-IVB system (7906 disc save on the Crisis Computer tape) in SIMH hp2100, with a 2645 terminal front end

Some staged folders name an `archive_original/` file that is not in this repository: tapes that carry a whole operating
system kit, a whole site's disc save or a licensed source tape are kept on disk only (see section 3 of `.gitignore`).
All of them came from bitsavers.org/bits, and each README gives the path.

## Folder names

Colossal Cave variants start with their name in the scheme of the
[Adventure Family Tree](https://mipmip.org/advfamily/advfamily.html) and
[Quuxplusone/Advent](https://github.com/Quuxplusone/Advent): up to four letters of the author's or porter's last name,
then the four-digit maximum score (`_XXX` if unknown). Where the eXo collection these games are packaged for files a
game under a different name, the collection's name is used. A game that has no name in any of them got one made by
the same scheme, from the machine or site when the porter is unknown (`TIDX0350` for the TI 990 DX10 version,
`VULC0350` for the Harris VULCAN one).
