# Adventure — Windows port

A working Windows port of the Univac 1100 EXEC 8 port of **Adventure** —
the original Crowther/Woods *Colossal Cave Adventure*, adapted for Univac
ASCII FORTRAN by Duff Kurland (Information Systems Design, Santa Clara CA),
building on a University of Maryland port with database enhancements from
Bob Elman (Four Phase Systems). This is the sibling/predecessor to the
Cave port at `../../Crystal Caves (Univac 1100, full source)/port/` (same
engine, same author, Cave came a year later with a new setting). Source
was recovered from `UNAD/page212-213/` (scanned Univac EXEC 8 job-stream
listings).

Run `build\adventure.exe` from the `build\` folder (it needs `ADV.DAT`,
the game database, sitting right next to it — `build.bat` copies it there
automatically).

## Building

```
build.bat
```

Needs a MinGW-w64 gfortran (developed against GCC 15.2), same as the Cave
port. `build.bat` points at the compiler by absolute path — Strawberry
Perl's bundled gfortran by default; edit the `GFORTRAN=` line if yours
lives elsewhere. `adventure.exe` is statically linked (only the standard
Windows Universal CRT DLLs), portable to another Windows machine on its own.

## What this is, and how it was ported

This is the same engine as the Cave port — same module list (MAIN, MOVE,
SPEAK, VOCAB, WIZARD, GETIN, HOURS, MOTD, POOF, RAN, SHIFT, START, YES,
BUG, CIAO, DATIME in Univac ASCII FORTRAN; DEFS/GETCLS/INITDB/SAVEDB/
SAVEMA/TDATE in genuine Univac 1100 MASM assembler tied to EXEC 8's
account/billing system), same category of dialect quirks (`include` for
common blocks, `@` end-of-line comments, `define` statement functions,
`AND`/`OR`/`XOR` as bitwise functions, `&` string concatenation, octal
literals, `SUBSTR`, packed 4-character text stored in `INTEGER` words).
Read `../../Crystal Caves (Univac 1100, full source)/port/README.md` for the
full explanation of each of those and why the port handles them the way it
does — this file only calls out
what's specific to *this* database/engine variant.

**Differences from the Cave port, source-side:**
- Simpler `MOVE`: this version splits it into separate `MOVE`/`CARRY`/
  `DROP`/`PUT` routines (`move.f`) rather than Cave's single self-
  contained `MOVE`/`CARRY`. No rope/carpet special-travel section (no
  `RCTRVL` parameter, no section 13) — that was added later, for Cave.
- Two more zero-argument "define" macros needed the same real-external-
  function treatment as Cave's `DARKRM`/`DARK`: here it's `DARK` and
  `LIQ` (`liqdark.f`), since this game's `DARK`/`LIQ` don't take the
  `UNICORN`-aware form Cave's do (no unicorn in the original game).
- A handful of the database's "section" reads use Univac FTN's blank-
  `FORMAT()` free-form numeric input (e.g. Section 3's travel table,
  each line ending in a literal `/` terminator) rather than fixed
  columns — ported to Fortran list-directed `READ(unit,*)`, which is
  the direct standard equivalent.
- `GETIN`'s `NXTCHR`/`INP2` locals need to survive between separate
  calls (that's how mid-line multi-command parsing and the `AGAIN`
  command work) — a plain Fortran local isn't guaranteed to do that
  without an explicit `SAVE`, which the original relied on implicitly.
  This was actually a latent bug shared with the Cave port too (fixed
  there at the same time as this one; see its README/memory for why it
  went unnoticed there — `NXTCHR` happens to be a COMMON member in
  Cave's `comblk.fi`, so only `INP2`/`AGAIN` was actually silently
  broken there).
- Two message formats needed a space typed in at a continuation join:
  the score-rating message (`' more'` continued into `'point'`) and the
  QUIT-preview message (`'you would'` continued into `'score'`). These
  are **not** typos in the 1978 source, despite what an earlier version
  of this file claimed. In standard fixed-form Fortran a source line is
  blank-padded out to column 72, so a character literal left open at
  end of line picks those blanks up — and both of these lines are 71
  characters long, so the original supplied exactly the one space that
  looks missing. `build.bat` compiles with `-ffixed-line-length-none`,
  which switches that padding off (documented behaviour of the flag),
  so the space has to be explicit here. Output is identical to the
  original either way. Worth knowing before re-wrapping anything in
  `main.f`: four other formats also leave a literal open at end of
  line, and all four land exactly on column 72 so they need no fix —
  but shortening one of those lines would silently eat characters out
  of the message.
- **No ASA/Fortran carriage control**, same bug and same fix as the
  Cave port (see its README for the full explanation): `speak.f` —
  shared engine code, identical between both ports — built a leading
  `'0'`/`' '` line-printer control character right into its output
  data, which gfortran just prints literally instead of interpreting,
  showing up as a stray `0` in front of most game text. Only one of
  `MAIN`'s own formats had this baked in directly (`'0Bad class
  number...'`, an unreachable error path since `GETCLS` always returns
  class 1) — everywhere else in this database's `MAIN`, unlike Cave's,
  already used the portable `/`-prefix convention for blank lines.

**Design choices** (all identical reasoning to the Cave port — see its
README for the full writeup): no account/billing system (`GETCLS`
stubbed to always return class 1), no wizard-mode Fieldata cipher
(`WIZARD()` keeps the magic-word gate, drops the `PRIVIL`/`FFDASC`/
`FASCFD` machinery), no prime-time/demo-game gating or suspend-latency
enforcement (`START` always allows play immediately), and SUSPEND/
RESTORE uses a local `ADV.SAV` file plus one big `gamecom.fi` COMMON
block holding all live game state, instead of a raw Univac memory-image
dump — with the same two-stage `INITDB`/`RESTOREDB` split, since the
database now has to be freshly re-parsed on every run before a saved
game's state can be laid on top of it. Wizard `MAINT`-mode tuning
persists via `ADV.CFG`, the same as Cave's `CAVE.CFG`.

## Known limitations

Same as the Cave port: no wizard-mode cipher (magic word alone gates
it), `RAN`/`DATIME` reseed from the real clock instead of the Univac
`TDATE$` word (so random sequences won't match a real Univac run bit-
for-bit, though the game plays identically either way), and SUSPEND/
RESTORE saves a broad blanket of state that should cover everything
meaningful — if something's ever found not to survive a restore
correctly, `gamecom.fi` is the file to extend.

## Correctness pass, 2026-08-31

Reviewed module by module against the Univac originals, including a
literal-by-literal diff of every quoted string in `main.f` to make sure
no game text drifted. The same three defects as the Cave port were
present here (see `../../Crystal Caves (Univac 1100, full source)/port/README.md`
for the full write-up) and are fixed the same way:

1. **The random number generator was never called.** `RAN` collides
   with a GNU extension intrinsic — a REAL generator whose argument is
   a *seed* — and gfortran prefers its own intrinsic over an external
   function of the same name unless the name is declared `EXTERNAL`.
   Every `RAN(n)` in `main.f` therefore bound to `_gfortran_rand`
   rather than to `ran.f`, re-seeded on each call, and returned the
   same constant forever. `PCT(N)` (`RAN(100).LT.N`) was **always
   true**, so the dwarves never activated — the first dwarf encounter
   is gated on `PCT(95)` failing, and it never failed. Measured: with
   a scripted route through the Hall of Mists, the pre-fix build
   produced byte-identical transcripts and zero dwarf activity across
   every run; the fixed build varies from 0 to 23 dwarf events over
   the same script. `main.f` now declares `EXTERNAL RAN` /
   `INTEGER RAN`.
2. **`build.bat` never created `build\`**, so this port did not build
   at all from a clean checkout — `gfortran -o ..\build\adventure.exe`
   failed with "cannot open output file". (The Cave port only appeared
   to work because its `build\` was left over from the original
   session.) Now created.
3. **A non-numeric answer in wizard `MAINT`/`NEWHRS` mode aborted the
   process** with a libgfortran list-input backtrace; those seven
   reads now use `IOSTAT` and fall back to the "zero to leave it
   alone" answer the prompts already offer.

Verified after the fixes: clean build from scratch, database parse
(table counts unchanged), normal play and scoring, `SUSPEND`/restore
round trip (position and inventory preserved through `ADV.SAV`),
wizard mode via `magic mode` + the magic word, and dwarf behaviour
varying run to run.
