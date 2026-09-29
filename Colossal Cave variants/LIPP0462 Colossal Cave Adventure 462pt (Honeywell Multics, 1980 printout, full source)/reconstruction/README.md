# Adventure 1.2 (462 points) for Honeywell's Phoenix Multics, 1980 (LIPP0462)

**Status: PORTED 2026-09-28.** `port\adv462.exe` - see `port\README.md`.

`src_original\adv462\` is a complete copy of https://github.com/lippard661/adv462 at commit
d5eafb3f7b8b700c5dcda2b321665a4f07db1fb6 (2026-09-27), made with `git archive` and md5-verified against the
clone: 44 files.

**The game.** This is the Colossal Cave that ran on Honeywell's Phoenix Multics system in 1980:
- Don Woods' 350-point game in Gary Palter's MIT-Multics Fortran port (PALT0350), with its common blocks,
  five-character packed words, wizard, prime time and SUSPEND/RESTORE of the common blocks;
- with early Dave Platt material related to his 550-point game (PLAT0550): rooms 141-217, objects 65-98,
  messages 202-270;
- and local changes: named SUSPEND/RESTORE, the news of May 1980, and a maximum of 462.

Jim Lippard maintained it for the Scouting project (Explorer Post 414). Its listings survived among his papers,
and no other copy is known.

**What upstream holds:**
- `artifacts\`: Lippard's transcriptions (2026), uncorrected, with notes and photographs of the printouts:
  - `Multics\adventure_.fortran` and `Multics\adventure.data`, the game;
  - `gcos\adv.fortran`, a separate GCOS port of Woods 350 from 1983.
- `Multics\`: the working game, the master copy.
  - It is corrected, finishes Lippard's own 1980 TAKE ALL / DROP ALL, and is converted to mixed case.
  - It supplies the lost site routines in PL/I, and runs on MR12.8.
  - `CHANGES.md` lists every change.
- `unix\`: the gfortran port, generated from the master copy.
- `openbsd\`: an OpenBSD package.

**The Windows port** builds on the Unix one with a few generated edits and a C file of site routines. It found
one bug that every version has: `vial` and `mushroom` are in no common block, so the saved new-game image did
not carry them, and in every game both held whatever was on the stack. On Windows THROW VIAL never worked; on
Linux a bare EAT could crash the game. The fix is in `port\src\mkwin.py`, and the account is in
`port\README.md`.
