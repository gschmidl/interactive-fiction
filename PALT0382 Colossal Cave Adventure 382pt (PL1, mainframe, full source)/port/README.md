# Adventure (PL/1 Version 4.0) — C transliteration, native Windows

A native Windows build of the SHARE/CBT-tape "Adventure — PL/1 Version
4.0" (`tapecave/PROGRAM.txt`), carried across from PL/I to C **statement
by statement** rather than rewritten. It is the sibling of the PL/I
build in `../port_qemu/`, which compiles the archived source itself
under Iron Spring PL/I in WSL2; this one needs no WSL and no PL/I
compiler.

```
build.bat
build\adventure.exe
```

Needs MinGW-w64 gcc (developed against GCC 15.2). `build.bat` points at
the compiler by absolute path — Strawberry Perl's bundled gcc by default;
edit the `GCC=` line at the top of the script if yours lives elsewhere.
`build.bat` also copies the game database (`../port_qemu/OBJECT`, produced
by that port's `decode_database.py` + `build_object.py`) next to the
executable. A blank `STORAGE` file is created on first save.

## Which build should I use?

Both. They answer different questions.

- `../port_qemu/` **runs the archived source**. The PL/I in it is the
  tape's own `PROGRAM.txt` with about 130 lines changed, all documented.
  That is the strongest provenance claim available, and it is the
  reference.
- This directory is the same program in C. It needs no WSL2 and no PL/I
  compiler, it is free of the Iron Spring compiler defects the PL/I
  build has to work around, and it runs on local time rather than UTC.
  Its authority comes from matching the PL/I build, not from being the
  original.

## How faithful is it, and how do we know?

The game is **deterministic**: `IX` is a static seed of 65549, and the
one clock-derived value (`ITIME`) is thrown away, because the
`CALL RAN(1)` inside the loop it drives is commented out in the 1978
source. Identical input therefore has to give identical output.

That turns validation into a mechanical check rather than a judgement
call. `tests/` drives both builds with the same scripted input and
diffs the transcripts byte for byte:

```bash
bash tests/runall.sh          # from WSL; needs both builds present
```

Eleven cases currently match exactly — normal play through the grate
and into the Hall of Mists, the magic words, treasure hunting, scoring
and quitting, every intransitive verb in turn, the `LOG` transcript,
and wizard mode (`HELP`, `INFO`, `DLOCS`, `@`, `WHERE`, `GO TO`,
`HOURS`, `LISTI`, `SWAP`). The harness normalises exactly two things,
both genuine and documented below: the wall-clock date banner, and the
elapsed-minute count in the final score.

## How the transliteration works

Read `src/advent.c` against `adventure.pli` and they line up: same
procedure names, same statement order, same label names (`L2630`,
`L8300`, `L30300`, …) as C labels and `goto`s, same variable names,
same `ADVARS` field order. The comments are the original's comments.

The parts that needed a decision:

- **PL/I's data model is kept, not converted.** `src/plisup.[ch]`
  implements `CHARACTER(n)` as a fixed-length, blank-padded, *not*
  NUL-terminated array, with 1-based `SUBSTR` as both rvalue and
  lvalue, blank-padded comparison, `INDEX`, `VERIFY`, `TRANSLATE`, and
  the `F`/`A`/`PICTURE` edit-directed conversions. Rewriting the engine
  around C strings would have meant re-deciding hundreds of statements;
  this way each one transliterates directly.
- **The state is globals, not a struct.** `src/advars.def` is an X-macro
  list of the `ADVARS` structure, expanded three ways: to declare the
  variables, to serialise them for `SUSPEND`, and to restore them. They
  are plain globals because the PL/I had them all in the scope of the
  outer `PROGRAM` procedure, where an internal procedure declaring a
  local of the same name shadowed them — `VOCAB`'s `I`, `SPEAK`'s `L`,
  `LIQLOC`'s `LOC` all do. C's own shadowing then reproduces that for
  free; a struct plus accessor macros would have broken on exactly
  those procedures.
- **Non-local `GO TO`.** Three labels in `PROGRAM` are branched to from
  inside an internal procedure (`BUG`, `CIAO`, `WIZPROC`, `DEMOCHK`,
  `PUTBACK`, `SCANSTG` — targets `DEALLOC`, `L31`, `L20000`). C cannot
  `goto` across a function, so these unwind through `setjmp`/`longjmp`,
  which is what a PL/I non-local `GO TO` does anyway.
- **The `DEFINED` overlays on `DATE_STG`** (`PIC_HHMM`, `PIC_HH`, …)
  are accessor macros over the character array, so the two places the
  engine deliberately writes *through* an overlay to update `DATE_STG`
  itself — `TIME_CHR = TIME();` in the scoring code and
  `PIC_HHMM = HHMM;` in `SCANSTG` — stay visible at the call site.
- **The obfuscation overlay.** `PW` is a `CHARACTER(4)` based on a
  `FIXED BIN(31)`, and the engine negates the integer to scramble user
  ids and save-game names. Here `PW` is a cast over `TEST_WORD`, so it
  picks up the host's little-endian byte order — the same order the
  PL/I build gets running on x86. The transform is its own inverse, so
  the byte order only affects what the bytes look like on disk.
- **`SYSPRINT`** (used only by the wizard `LOG` command) is buffered and
  flushed at exit, which is what the Iron Spring build does, so the two
  transcripts stay comparable.
- **`main`** joins `argv` into `INPARM`, the `CHARACTER(100) VARYING`
  parameter the mainframe passed from the TSO `CALL` command.

`src/platform.c` holds the replacements for the MVS/TSO externals, one
for one with the PL/I port's own (`tread.pli`, `twrite.pli`,
`clrscrn.pli`, `randu.pli`, `itime.pli`, `decdate.pli`, `r062a10.pli`,
`warnmsg.pli`, `whisper.pli`, `stgwr.pli`), including the upper-case
input fold the WELLPUT assembler did with `OC 0(133,R2),BLANKS` and the
UTF-8 code-page step on output.

## Where it deliberately differs from the PL/I build

- **Local time, not UTC.** Iron Spring's `DATE`, `DATETIME` and `TIME`
  builtins return UTC regardless of `TZ`; the mainframe returned local
  time, and so does this. That is why the test harness normalises the
  date banner and the elapsed-minute count.
- **`REWRITE` works.** Iron Spring 1.4.1 leaves `REWRITE` unimplemented
  for every file organisation, so in the PL/I build the wizard's
  `ENCODE`, `DECODE`, `SAUCE`, `SEND`, `ADDUSER` and `DELUSER` commands
  silently do nothing. Here they do what they say.
- **The `STORAGE` files are not interchangeable.** Both use 4800-byte
  records and both put `NAME || USERID` at the front — which is all the
  engine itself depends on, since `PUTBACK` and `SCANSTG` identify a
  saved game by comparing `SUBSTR(ADVREC,1,16)` — but PL/I packs
  `BIT(1)` into bits and prefixes `VARYING` strings with a halfword,
  and this does neither. Save with one build, restore with the same one.
  `ADVARS`'s 29-word `ZZZZZZ` "SPARE FILLER" is the only field left out
  of the record; nothing in `adventure.pli` ever references it.

## Inherited from the PL/I port

These are decisions made for `../port_qemu/` and carried across unchanged;
see that README for the reasoning.

- `WIZARD` is forced on, skipping the shared-mainframe login gate. That
  also means the start-up database-capacity report is shown, and the
  1500-turn and 150-minute limits do not apply.
- `TSOID` is fixed at `PLAYER` instead of being read out of an MVS
  control-block chain.
- The operator-console `WTOR` in `WIZPROC` becomes a direct `BUG(35)`.
- The "magic parm of the day" wizard login is left intact but is dead:
  its guard `SUBSTR(INPARM,5,1) > 'Z'` relies on EBCDIC collation, where
  the digits sort above the letters. In ASCII they sort below.
- `ADVARS`'s `MACROTIME` is `unknown`: it held the PL/I compile
  timestamp, produced by a preprocessor Iron Spring does not run.

## Status

Complete. The whole of `adventure.pli` is transliterated — the
`PROGRAM` body in `src/advent.c`, all 38 internal procedures in
`src/advsubs.c` — and the eleven comparison cases match the PL/I build
byte for byte. `SUSPEND`/`RESTORE` round-trips within this build
(saved in the well house holding lamp and keys, back-dated past the
one-hour lockout, restored to the same location and inventory). Builds
warning-clean at `-Wall -Wextra`.
