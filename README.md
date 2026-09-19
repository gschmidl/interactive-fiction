# AI-ASSISTED PROJECTS
are here at github

# NON AI-ASSISTED PROJECTS
are at https://codeberg.org/gschmidl/interactive-fiction.

# STATUS

Every folder is one game. A folder with a `port/` directory (or, for the type-ins, the rebuilt program itself) is a
finished recovery or port; its own README says how it was done and how it was checked.

## WORK IN PROGRESS - porting has NOT begun

The folders below were **staged on 2026-09-19 and nothing in them has been built, run or verified.** They hold only the
original material (`archive_original/`, `src_original/`, `reference/`) and a `README.md` that starts with
"Status: STAGED" and records where the files came from, what they are, and what a port would need. Treat every claim
in those READMEs as a first reading of the files, not as a result.

Planned order, least effort first:

1. `HOWE0301 Adventure! 301pt (Atari 8-bit BASIC, Robert Howell, 1982)`
2. `KNUT0350 Colossal Cave Adventure 350pt (Knuth CWEB, 1998)`
3. `POHL0350+DAIM0350 Colossal Cave Adventure 350pt (Jerry Pohl C port 1984 + Daimler Turbo C 1990)`
4. `HALL0501 Colossal Cave Adventure 551pt (Robert Hall's C version 7.0, MINIX, 1994)`
5. `Wander worlds + Dune + Beasts (Peter Langston, Usenix 80.1 and 85.1 tapes)`
6. `ROBE0665 Colossal Cave Adventure 665pt Wellesley (Eric Roberts, FORTRAN source + WebFor SVM bytecode)`
7. `ROBE0240 Colossal Cave Adventure 240pt Starter (Eric Roberts, WebFor SVM bytecode)`
8. `Dungeon by Jonathan Reed (Prime 50-Series, PL1G, full source)`
9. `(MOOR0350) Colossal Cave Adventure 350pt (IBM MVS, MSU FORTRAN, CBT COV466)`
10. `HORV0350 Colossal Cave Adventure 350pt (SEL 32, RTM, 1979 printout, full source)`
11. `(ANON0350) Colossal Cave Adventure 350pt (Prime 50-Series, PRIMOS FORTRAN, full source)`
12. `(HEHO0366) Colossal Cave Adventure 350pt with palantir (CDC NOS 1.3, ACCA, FORTRAN source)`
13. `JAZE_XXX Colossal Cave Adventure 350pt + 500pt castle (CDC Cyber 74, MCAUTO, FORTRAN source)`
14. `Qork V3.0A (CDC NOS, 1984, FORTRAN source)` (optional)
15. `Dave's Dungeon - DAVESCAVE (PDP-11 TRAX, MITRE, FORTRAN source)`
16. `PLAT0550 Colossal Cave Adventure 550pt original (Xerox Sigma CP-V, David Platt, full source)`
17. `Mystery Mansion (HP 1000, RTE, FORTRAN source)`
18. `Dungeon (HP 1000, RTE, FORTRAN source)`
19. `ARNA0660 Colossal Cave Adventure 660pt Glaxo 4.3 (Prime 50-Series, A-code, 1984)`
20. `(ANON0350) Abenteuer (Harris VULCAN, German Colossal Cave, full source)`
21. `Unnamed volcano adventure (NorthStar Horizon BASIC, orphaned fragment)`
22. `Dungeon (Prime 50-Series, PRIMOS, binary)`
23. `(ANON0550) Colossal Cave Adventure 550pt (VAX VMS 1.5, 1979, exe only)`
24. `(ANON0350) Colossal Cave Adventure 350pt (TI 990, DX10, binary)`
25. `(ANON0350) Colossal Cave Adventure 350pt (Philips P7000, Four-Phase IDOS)`, then
    `Quest (Philips P7000, Four-Phase MFE, multiplayer)`

Some staged folders name an `archive_original/` file that is not in this repository: tapes that carry a whole operating
system kit, a whole site's disc save or a licensed source tape are kept on disk only (see section 3 of `.gitignore`).
All of them came from bitsavers.org/bits, and each README gives the path.

## Folder names

Colossal Cave variants start with their name in the scheme of the
[Adventure Family Tree](https://mipmip.org/advfamily/advfamily.html) and
[Quuxplusone/Advent](https://github.com/Quuxplusone/Advent): up to four letters of the author's or porter's last name,
then the four-digit maximum score (`_XXX` if unknown). A name **in parentheses** - `(BEAS0385)`, `(ANON0350)` - does
not appear in either list: it was constructed here by the same rules and is preliminary.
