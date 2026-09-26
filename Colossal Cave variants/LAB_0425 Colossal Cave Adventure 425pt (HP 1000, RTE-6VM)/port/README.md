# Adventure II 2.2 (425 points) — Windows port

A working Windows port of **"HP 1000 Adventure" version 2.2**, the 425-point
Colossal Cave variant distributed by LABtec (the HP Technical Computer User's
Group of Los Angeles) in 1987 for HP 1000 minicomputers running RTE-6/VM and
RTE-A.

Run `build\adventure.exe` from the `build\` folder. It needs `ADVENTURE.DAT`
(the compiled game database) beside it; `build.bat` produces that for you.

## Building

```
build.bat
```

Needs a MinGW-w64 gfortran (developed against GCC 15.2) — the same toolchain
as the Univac ports, no WSL2 needed. `build.bat` points at the compiler by
absolute path (Strawberry Perl's bundled gfortran by default); edit the
`GFORTRAN=` line if yours lives elsewhere.

The build produces two programs, mirroring the original release:

- `abuild.exe` compiles the human-editable `ADVENTURE.TXT` into the binary
  `ADVENTURE.DAT` that the game memory-maps at startup;
- `adventure.exe` is the game.

`build.bat` runs `abuild.exe` for you, so a plain `build.bat` leaves a
ready-to-play `build\` directory.

## Verifying

```
regress.bat
```

replays two recorded walkthroughs through the port and diffs the result
against transcripts captured from the *original* `ADVENT.RUN` executing on
an emulated HP 1000 (SIMH `hp2100`, booted off the RTE-6/VM disc image in
`..\src_original\`). Both currently come out **identical**, line for line —
101 and 133 lines of game output covering the surface, the grate, the bird
chamber, the Hall of Mists, the gold nugget and scoring.

To recapture the reference transcripts, start the simulator in a console
window of its own (SIMH refuses to run when its command console is a pipe)
with `set console telnet=2325` in the command file, then:

```
python tools\simdrv.py 2325 tests\walk1.sim tests\walk1.simh.out
```

## Where the source came from

The two `HP-2100` folders in the eXo collection's *Colossal Cave Adventure
(1976)* package ship
a SIMH `hp2100` simulator, a 50 MB HP 7920 disc image, and a saved machine
state that drops you at the game's prompt. The disc image turned out to hold
the **complete FORTRAN 77 source**, not just the built program, in an RTE-6/VM
CI directory `/ADVEN425`.

`..\src_original\rte6vm_extract.py` is the file-system reader written to get
it out; `..\src_original\disc\` is everything it recovered, and
`..\src_original\disc_listing.txt` is the full volume listing. The layout it
decodes is documented at the top of that script — briefly: the image stores
16-bit words byte-swapped, the FMP logical unit starts at image sector
0xC4E0, blocks are 128 words, the root directory sits at block 0x24, and
record-structured files frame each record with a leading and trailing length
word whose top bit flags an odd character count.

The recovered release is:

```
ADVENT.FTN     main program and BLOCK DATA
AINIT.FTN      initialisation overlay
AMAIN.FTN      the game itself (1889 lines)
ASUB.FTN       resident subroutines (VOCAB, MOVE, CARRY, SCORING, ...)
AIOSUB.FTN     I/O, MAGIC maintenance mode, SAVE and RESTORE
ABUILD.FTN     database compiler
*.INCL         the ten named COMMON blocks
ADVENTURE.TXT  the game database in editable form
ADVENTURE.DOC  the 1987 release notes
CHEAT.TXT      the author's hint sheet
MESSAGE.TXT    the LABtec banner
```

Its own revision history: original version 3/07/81 by "AW", made FTN77/LINK/
MACRO/CI compatible 12/12/86 by "JLA", cave expanded 2/10/87, scoring and
database bugs fixed 7/13/87. Above that it carries the whole lineage —
Crowther and Woods on the DECsystem-10, FORTRAN IV-PLUS under IAS on the
PDP-11/70, Kent Blackett's and Bob Supnik's RT-11 FORTRAN IV version, and
Beasley's September 1978 HP-21MXE adaptation. A further set of edits marked
`DWH` dates from September 2025, when the disc image was made.

## What was changed

`src\*.f` and `src\*.fi` are **generated** from `..\src_original\disc\` by
`tools\convert.py`; rerun it any time. It applies two stages:

`tools\hp2gfortran.py` handles the mechanical HP FTN7X dialect differences:

- compiler-control lines (`FTN77,L`) and `$EMA` / `$FILES` directives become
  comments; `$INCLUDE FOO.INCL` becomes a standard `include`;
- HP lets a program unit carry a descriptor —
  `SUBROUTINE SPEK(WORD),Print a Message <250929.1142>` — which is dropped,
  as is the partition sizing in `PROGRAM ADVENT(4,150)`;
- Hollerith constants become packed `INTEGER*2` literals, two characters per
  word in memory order, which is what the `EQUIVALENCE`d `CHARACTER`
  variables and `A2` edit descriptors around them expect on a little-endian
  host. Hollerith handed straight to a subroutine is left alone: `VOCAB`
  reads `6HKEYS  ` as `ID(1:3)` through sequence association, and gfortran
  passes it correctly;
- `15501B` octal literals become decimal (gfortran only takes `O'...'` in
  `DATA`, and these appear in expressions);
- bare `INTEGER` and `LOGICAL` become `INTEGER*2` / `LOGICAL*2`, since HP
  defaults both to 16 bits and gfortran to 32;
- `MAX0`/`MIN0`/`IABS`/`FLOAT`/`IFIX` become the generics, which accept the
  16-bit arguments the specific forms reject;
- the game's own `RAND` is renamed `RNDM` so it does not collide with
  gfortran's `RAND` intrinsic;
- a trailing `_` in a FORMAT — HP's "no newline" marker, used for the `>`
  prompt and for phrases the code continues with an object name — becomes
  gfortran's `$` edit descriptor;
- `BUFSIZ=` is dropped from `OPEN`.

The `FIXUPS` table in `tools\convert.py` then applies the edits that needed
judgement, each one commented in place:

- **Terminal I/O.** RTE drives one terminal LU in both directions. gfortran
  preconnects standard input and standard output as separate units, so reads
  move to a second unit `KBD`, added to `/IOCCOM/` — the one COMMON block the
  game deliberately excludes from `SAVE`/`RESTORE`, so the database format is
  untouched.
- **Statement functions.** gfortran type-checks their arguments strictly, and
  its integer literals are 32-bit where every variable here is 16-bit. The
  dummies that only ever receive literals (`DUMMY`, and new `NPCT`, `NBIT`,
  `PBOTL`) are declared 32-bit; `BITST`'s two dummies are renamed so they
  stop colliding with the real variables `L` and `N`.
- **`WD2.EQ.0`** at label 1660 tests an unsubscripted array in a scalar
  context, which HP reads as element 1; label 2220 spells the same test out
  as `WD2(1).EQ.0`, so that is what it becomes.
- **File status codes.** The original tests FMP status 506 ("not found") and
  502 ("already exists"); gfortran reports its own `errno`, so those become
  `INQUIRE`-based helpers `NOFILE` / `HASFILE`.
- **`FORM='UNFORMATTED'`** added to the database revision probe. FMP files
  are typeless, so the original could read a binary word off a unit opened
  without `FORM=`.
- **End of file.** An RTE terminal never reports one, so no terminal read
  carries an `END=` branch. A redirected standard input does, so every one
  of them gets a branch — the game prints `[end of input]` and stops.
- **The `:::3:300` suffix** that `SAVE` appends to a file name is RTE's file
  type and size specification; it would otherwise become part of the name.

`src\hprte.f` is new code, not translated: stand-ins for the RTE-6/VM system
library routines the game calls — `LOGLU`, `TRIMLEN`, `CASEFOLD`, `CLCUC`,
`SPLITSTRING`, `FPARM`, `FTIME`, `SSEED`/`URAN`, `NFIOB`, `EXEC` (calls 6, 7
and 11), `FMPPURGE`, `FMPREPORTERROR`, `UserIsSuper`. Two are worth a note:

- `SPLITSTRING` delimits on **blanks and commas** and consumes exactly one
  delimiter. Both matter: the database reader splits `1001,KEYS` and the
  MAGIC menu lines `31,  OH - Opening Hours`, whose leading blanks have to
  survive, while `GETIN` splits `get lamp`.
- `URAN` is an ordinary 32-bit LCG. The HP generator's exact stream cannot be
  reproduced off the machine, and the game only ever seeds it from the wall
  clock, so any decent generator is faithful in substance.

Nothing in the game logic itself was touched.

## Notes on the game

- 425 points. `ADVENTURE.DOC` lists what version 2 added over the 350-point
  original: `SAVE`/`RESTORE` (synonyms `SUSPEND`/`PAUSE` and `RESUME`), fast
  initialisation, upper and lower case output, `MAGIC` maintenance mode and
  an expanded cave.
- `SAVE` and `RESTORE` prompt for a file name on the following line. A saved
  game is a complete image of the COMMON blocks, the same format as
  `ADVENTURE.DAT`, so `adventure.exe mygame.dat` resumes directly.
- The cave has opening hours and a prime-time lockout, configurable from
  `MAGIC` mode. `ABUILD` sets `IHOURS(1,1) = -1`, meaning always open, so the
  shipped database has no restriction.
- `MAGIC` mode is guarded by an arithmetic puzzle: six digits are displayed,
  and the password is the middle four added together minus the outer two.
  `abuild.exe` always enters MAGIC mode at the end of its run; `build.bat`
  feeds it no input, which fails the guard harmlessly — the database is
  written either way.
- The version 2 "echo back of words input" feature is off: it drove HP 262x
  terminal escape sequences, and was commented out in the 2025 RTE-6/VM
  adaptation. Both the original and this port therefore print no echo.
- `CHEAT.TXT` is the author's own hint sheet, copied into `build\` for
  reference. It is not read by the game.
