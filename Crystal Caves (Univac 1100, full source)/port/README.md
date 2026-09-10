# Cave — Windows port

A working Windows port of **Cave**, a 1980 text adventure written by Duff
Kurland (Information Systems Design, Santa Clara CA) for Univac 1100-series
mainframes running EXEC 8. It's a sibling/successor to the classic
Crowther/Woods *Colossal Cave Adventure* — same game engine, but with an
entirely different setting: a Boy Scout spelunking trip through a cave
system near a barn, pasture and sinkhole, instead of Kentucky's Mammoth
Cave. Source was recovered from `UNAD/page215-216/` (scanned Univac EXEC 8
job-stream listings).

Run `build\cave.exe` from the `build\` folder (it needs `CAVE.DAT`, the
game database, sitting right next to it — `build.bat` copies it there
automatically).

## Building

```
build.bat
```

Needs a MinGW-w64 gfortran (developed against GCC 15.2) — no WSL2/Linux
toolchain needed, unlike the PL/I `tapecave` port. `build.bat` points at the
compiler by absolute path (Strawberry Perl's bundled gfortran by default);
edit the `GFORTRAN=` line at the top of the script if yours lives elsewhere.
The resulting `cave.exe` is statically linked (only depends on the standard
Windows Universal CRT DLLs), so it's portable to another Windows machine on
its own.

## What this is

The original source is a mix of:

- **Univac ASCII FORTRAN (FTN)** for the game engine itself — `MAIN`,
  `MOVE`, `SPEAK`, `VOCAB`, `WIZARD`, etc. This is genuine 1966/77-ish
  Fortran with a handful of Univac-specific extensions (`include` for
  named common blocks, `@` end-of-line comments, `define` statement
  functions, `AND`/`OR`/`XOR` as bitwise functions, `&` for string
  concatenation, octal literals, `SUBSTR` for substrings). Almost all of
  the actual game logic — room descriptions, verb handling, dwarf/pirate
  AI, scoring, the works — is unmodified aside from mechanically
  translating those dialect quirks to standard Fortran.
- **Univac 1100 MASM assembler** for `DEFS`, `GETCLS`, `INITDB`, `SAVEDB`,
  `SAVEMA`, `TDATE` — genuinely tied to EXEC 8 (`ER PCT$`/`MCT$`/`PROP$`
  billing/accounting calls, word-addressable drum files named through the
  shared computer's account system). None of that exists anywhere any
  more, so these were rewritten from scratch rather than translated.

See `src/*.f` / `src/*.fi` for the ported sources, one file per original
module, plus a few new ones (`darkrm.f`, `upperc.f`, `savefile.f`,
`gamecom.fi`) explained below.

## Design choices / what's different from the original

- **No account/billing system, no wizard cipher.** `GETCLS` (customer vs.
  in-house class), `PRIVIL` (ISD-staff wizard bypass), and `WIZARD`'s
  Fieldata challenge/response cipher (which called two undocumented
  system routines, `FFDASC`/`FASCFD`, that aren't part of this source
  archive and no longer exist) all depended on the Univac account system.
  `getcls.f` just always returns class 1. `wizard.f`'s `WIZARD()` keeps
  the "do you know the magic word?" gate (default: `dwarf`) but drops the
  cipher entirely — same call-and-response, just no century-old system
  calls behind it. This mirrors the choice already made for the sibling
  `tapecave` PL/I Adventure port (forcing past its shared-mainframe login
  gate).
- **No prime-time/demo-game gating.** The original only let regular
  players adventure outside "prime time" business hours on the shared
  computer, offering wizards or a short demo game otherwise, and made you
  wait out a latency timer before resuming a suspended game (so you
  couldn't quickly retry a risky move). None of that makes sense for a
  private, single-player copy, so `start.f` always allows play
  immediately. The `HOURS`/`NEWHRS`/`MAINT` machinery for configuring
  those hours is still there and still works if you go looking for it
  (via wizard mode) — it just no longer gates anything.
- **Save file is a local file, not a raw memory dump.** The original
  `INITDB`/`SAVEDB`/`SAVEMA` worked by copying the *entire* live memory
  image ("D-bank") to/from a word-addressable file named through the
  account system — including a "master" pre-parsed snapshot other players
  would load from at startup instead of re-parsing the text database (a
  speed optimization that made sense when parsing was slow and expensive,
  not on modern hardware). This port instead:
  - always re-parses `CAVE.DAT` fresh at startup (milliseconds either
    way, so the master-snapshot optimization has no point),
  - keeps every piece of live game state (object positions, scoring
    counters, dwarf positions, wizard settings, etc.) in one big COMMON
    block, `gamecom.fi`, specifically so `savefile.f`'s replacement
    `SAVEDB`/`INITDB` can serialize it with a plain Fortran
    `READ`/`WRITE`. This is the same "just dump everything" approach the
    original used, just via a named COMMON block instead of a raw memory
    image — see the comment in `gamecom.fi` for why, and the two-stage
    `INITDB`/`RESTOREDB` split in `savefile.f` for how the restore is
    sequenced (it has to happen *after* `MAIN` re-parses the database,
    unlike the original where the database was part of the memory dump
    too).
  - `SUSPEND` writes `CAVE.SAV` next to `cave.exe`; the next launch offers
    to load it.
  - Wizard `MAINT`-mode tuning (magic word, short-game length, cave
    hours, etc.) persists across games via a small `CAVE.CFG` file
    instead of the master D-bank snapshot.
- **`DARKRM`/`DARK` are real functions, not statement functions.** The
  original defined them as *zero-argument* macros (`define DARKRM= ...`)
  — no parentheses at all, which only parses because Univac's `define`
  keyword itself disambiguated it from a plain assignment. Standard
  Fortran statement functions need real (even if empty) argument
  parentheses to make that distinction, so these became ordinary external
  functions in `darkrm.f`, called as `DARKRM()`/`DARK()`.
- **No site-wide "everything defaults to INTEGER" compiler flag.** Most
  of the original modules had no `IMPLICIT` statement at all and still
  used variables starting with letters outside Fortran's default I-N
  integer range (`OBJECT`, `WHERE`, `SPK`, `VERB`, ...) as plain integers
  — meaning the whole codebase was compiled under some Univac FTN
  `option` switch that changed the default. Every module here has an
  explicit `IMPLICIT INTEGER(A-Z)` added to reproduce that.
- **Packed-character tricks became real `CHARACTER` variables.** The
  original stored 4-character text chunks inside `INTEGER` array
  elements/words (reading them with `A4` format edit descriptors — a
  common trick on machines with 36-bit words and no adequate `CHARACTER`
  type support at the time) and tested for "blank" by comparing against
  an octal literal representing four space characters packed into one
  36-bit word. That packing is 36-bit/Univac-character-set specific and
  doesn't mean anything on a byte-addressed x86 machine. Anywhere the
  `LINES`/`MSG` arrays hold *text* rather than the pointer/link values
  they also double as, this port uses an `EQUIVALENCE`d `CHARACTER*4`
  alias array (`CLINES`, `CMSG`) instead, and blank-checks compare
  against `'    '` directly.
- **`GETIN`'s `INP2` (the "AGAIN"-repeat buffer) is explicitly `SAVE`d.**
  It has to survive between separate calls to `GETIN`, and a plain
  Fortran local isn't guaranteed to do that without an explicit `SAVE`
  — the original relied on it implicitly. Found and fixed while porting
  the sibling Adventure database, where the equivalent bug also broke
  ordinary multi-command-per-line parsing (there, `NXTCHR` needed the
  same fix; here `NXTCHR` happens to already be a COMMON member via
  `comblk.fi`, so only `AGAIN` was actually affected).
- **No ASA/Fortran carriage control.** The original relied on the old
  line-printer convention where the *first character* of each printed
  line is a control code, not printed text: `'0'` means "skip a line
  first" (blank line before), `' '` means print normally, `'1'` means
  page-eject. `SPEAK` (the routine behind almost all game text — room
  descriptions, messages, everything from the database) built this
  character right into its output data (`WRITE(6,20) spacng, ...`), and
  a handful of `MAIN`'s own `PRINT`/`FORMAT` statements did the same
  directly. gfortran doesn't interpret any of this — it just prints the
  control character literally — which is why unfixed builds showed a
  stray `0` in front of most messages. `speak.f` now tracks the same
  "blank line still owed?" state as a `LOGICAL` and emits a real blank
  line instead of a leading data character; the handful of `MAIN`
  formats that had `'0...'` baked in were changed to a leading `/` (a
  real blank-record separator, which is portable, standard Fortran).

## Known limitations

- The wizard-mode "prove you're a wizard" cipher is gone (see above) —
  the magic-word check alone gates it now.
- `RAN`/`DATIME` are reseeded from the real clock (`SYSTEM_CLOCK`/
  `DATE_AND_TIME`) instead of the Univac `TDATE$` system word, so exact
  random sequences won't match a real Univac run bit-for-bit — the game
  itself plays identically either way.
- Suspend/restore saves a broad blanket of state (see `gamecom.fi`) that
  should cover everything meaningful; if something is ever found to not
  survive a restore correctly, that's the file to extend.

## Interactive fiction

The sibling `page212-213/` source tree — the ISD Univac port of the
*original* Crowther/Woods game (as opposed to Cave's new setting) — is
now ported too, at
`../../Colossal Cave Adventure 400pt (Univac 1100, full source)/port/`.

## Correctness pass, 2026-08-31

Reviewed module by module against the Univac originals (`MAIN.TXT`,
`SPEAK.TXT`, `MOVE.TXT`, `VOCAB.TXT`, `GETIN.TXT`, `HOURS.TXT`,
`WIZARD.TXT`, `RAN.TXT`, `SHIFT.TXT`, `START.TXT`, `DATIME.TXT`,
`FTNDEFS.TXT`, …), including a literal-by-literal diff of every quoted
string in `main.f` to make sure no game text drifted. Three things
were wrong:

1. **The random number generator was never called.** `RAN` is also the
   name of a GNU extension intrinsic — a REAL generator whose argument
   is a *seed* — and gfortran binds to its own intrinsic in preference
   to an external function of the same name unless the name is declared
   `EXTERNAL`. `main.f` did not declare it, so every `RAN(n)` call in
   the game bound to `_gfortran_rand` instead of `ran.f`, and since the
   argument was always a nonzero constant the intrinsic re-seeded on
   every call and returned *the same value every time* (`RAN(100)`
   ≡ 7.8249E-04). Consequences: `PCT(N)`, defined as `RAN(100).LT.N`,
   was **always true** for every N, so every percentage-gated event
   fired every time and the dwarves never activated at all (the first
   dwarf encounter is gated on `PCT(85)` failing); `J=MINDWR+RAN(...)`
   always returned `MINDWR`; and `CALL PSPEAK(DRAGON,2+RAN(8))` passed
   a REAL bit pattern where the routine expected an INTEGER message
   number. Confirmed by symbol inspection (`main.o` referenced
   `_gfortran_rand` and never `ran_`, and `ran.f` was linked but dead)
   and by transcript: identical byte-for-byte output across repeated
   runs before the fix, varying after. `main.f` now declares
   `EXTERNAL RAN` / `INTEGER RAN`.
2. **`build.bat` never created its own output directory**, so
   `gfortran -o ..\build\cave.exe` failed with "cannot open output
   file" on any clean checkout. It worked here only because `build\`
   happened to be left over from the original porting session. Now
   creates it.
3. **A non-numeric answer in wizard `MAINT`/`NEWHRS` mode killed the
   process** with a raw libgfortran "Bad integer for item 1 in list
   input" backtrace, because the Univac blank-`FORMAT()` numeric reads
   became bare list-directed `READ (5,*)`. Those seven reads now use
   `IOSTAT` and treat an unparseable answer as the "zero to leave it
   alone" answer the prompts already offer.

Verified after the fixes: clean build from scratch, database parse
(table counts unchanged), normal play and scoring, `SUSPEND`/restore
round trip (position and inventory preserved through `CAVE.SAV`),
wizard mode and `MAINT` settings persisting via `CAVE.CFG`, and dwarf
behaviour now varying run to run.

Also checked and found correct, for the record: all array-size
parameters against `FTNDEFS.TXT`; every COMMON block layout against
its original (including Cave's extra `NXTCHR`/`CHECK` members and the
`TXTCOM` member order, which differs between the two games); the
`DARKRM`/`DARK` extractions against the original `define` macros; the
`SHIFT` rewrite (`ishft`) against the original 36-bit mask arithmetic,
which is only ever called as `SHIFT(1,N)` with small positive N; and
`RAN` itself, which matches `RAN.TXT` line for line apart from its
clock seed. The only remaining gfortran warning is a harmless
`-Walign-commons` note about 3 bytes of padding in `WIZCOM`, which is
consistent across every unit because they all include `comwiz.fi`.
