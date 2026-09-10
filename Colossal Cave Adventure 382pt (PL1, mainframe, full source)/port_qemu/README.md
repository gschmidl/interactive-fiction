# Adventure (PL/I Version 4.0) — recompiled for Linux/WSL2

This is the SHARE/CBT-tape "Adventure" game (PL/I port of the classic
Crowther & Woods Colossal Cave Adventure, enhanced with wizard mode,
hints, magic messages, etc.) from `tapecave/PROGRAM.txt`, recompiled to
run natively on Linux (via WSL2 on Windows) using the free
[Iron Spring PL/I compiler](http://www.iron-spring.com/), instead of
the original MVS/TSO mainframe environment.

## How to build

Requires WSL2 with a Linux distro (tested on Debian) and the Iron
Spring PL/I compiler (download `pli-1.4.1.tgz` from iron-spring.com and
`tar xzf` it anywhere — no `make install`/root needed). Point `PLIDIR`
at the unpacked `pli-1.4.1/` directory; `build.sh` carries a default at
the top of the file.

```bash
python3 decode_database.py   # (re)builds decoded_database.txt from ../DATABASE.dat
python3 build_object.py      # (re)builds OBJECT from decoded_database.txt
bash build.sh                # compiles + links ./adventure
```

## How to play

```bash
./adventure
```

The game needs an `OBJECT` file (the game database, built by
`build_object.py`) and a `STORAGE` file (scratch space for the
save-game subsystem) in the working directory. A blank `STORAGE` is
enough to play:

```bash
python3 -c "open('STORAGE','wb').write(b' '*4800*20)"
```

Say "no" to the instructions prompt to jump straight into the game.
`SUSPEND <name>` saves, `RESTORE <name>` reloads (the game refuses to
restore a game suspended less than an hour ago — that is the original
behaviour, not a bug).

## What had to change from the original mainframe source

The PL/I game-logic source (`adventure.pli`, ~4,700 lines) is almost
entirely unmodified — `diff` it against `PROGRAM.txt` (with the
sequence numbers in columns 73-80 stripped) and the whole delta is
listed below:

- **`^` as NOT operator**: compiled with `-cn(^)` (Iron Spring
  defaults to a different alternate-NOT character).
- **`RETCODE` entry option**: unsupported by Iron Spring on `ENTRY
  ... OPTIONS(ASM INTER RETCODE)` declarations; dropped (the routines
  it applied to are reimplemented natively anyway, see below).
- **`%DCL`/preprocessor macros** (`QUOTE`, `MACROTIME`, `XLATETO`,
  `XLATEFR`): Iron Spring's `plic` doesn't run the `%` preprocessor
  pass; replaced with plain `DCL ... STATIC INIT(...)` constants.
  `MACROTIME` was the compile timestamp, so the wizard `INFO` command
  now says "Program was compiled on unknown".
- **`DISPLAY ... EVENT(...)`**: an MVS operator-console WTOR (used
  only as a last-resort wizard-password fallback); replaced with a
  direct `CALL BUG(35)` since there's no operator console to wait on
  standalone.
- **`DEFINED ... POS(n)` is broken in Iron Spring 1.4.1** for `n>1`
  (confirmed with a minimal repro — both reads *and* writes silently
  go to POS(1)). The seven date/time fields overlaid on `DATE_STG`
  (`TIME_CHR`, `CHR_HHMM`, `PIC_MONTH`, `PIC_DAY`, `PIC_HHMM`,
  `PIC_HH`, `PIC_MM`) are therefore declared `BASED` on interior
  pointers (`DSPTR5/7/9/11`, set from `POINTERADD(ADDR(DATE_STG),n)`
  just before the first executable statement) instead. That is an
  exact stand-in: reads *and* writes go through to `DATE_STG`, which
  matters, because the engine deliberately updates `DATE_STG` *through*
  two of these overlays — `TIME_CHR = TIME();` in the scoring code and
  `PIC_HHMM = HHMM;` in `SCANSTG`. `DATE_PIC` and `PIC_YEAR` use
  `DEFINED` with no `POS`, which works correctly, and are untouched.
- **`FIXEDOVERFLOW` is trapped by default**, which the RANDU
  generator's classic multiply-and-wrap step relies on; the one
  `IY = IX * 65539;` statement is now prefixed `(NOFIXEDOVERFLOW):`.
- **`REWRITE` is hard-stubbed to fail in Iron Spring 1.4.1** —
  `lib/source/ior.pli`'s `file_op` literally does `when(12) /* REWRITE
  */ signal condition(UNIMPLEMENTED);` for every file organization (so
  are `LOCATE`, `DELETE`, and `UNLOCK`, for what it's worth). This
  broke SAVE/SUSPEND and RESTORE, both of which update a specific
  already-read `STORAGE` record in place. Worked around with a new
  routine, `stgwr.pli` (see table below), that bypasses PL/I's FILE
  abstraction entirely and does a raw positioned `lseek`+`write` to
  the record instead. `CIAO` (SUSPEND/SAVE) and `PUTBACK` (RESTORE)
  now track which record they're sitting on (`STGREC`, incremented on
  every `READ FILE(STORAGE)`, reset on every `OPEN`) and call
  `STGWR(STGREC, ADDR(...))` instead of `REWRITE`. Verified end to
  end: saved mid-game (inside the building, lamp and keys taken),
  back-dated the saved timestamp past the one-hour lockout, restored
  into a fresh process, and resumed at the same location with the same
  inventory. (`ADDUSER`/`DELUSER`/`SEND`/`GETMSGS`/`SCANSTG` — the
  wizard/sysadmin commands for a shared multi-user system — still use
  `REWRITE` and so still silently no-op; not worth fixing for a
  single-player build.)
- **MVS control-block chain-walking** (`TSOID = TIOCNJOB;`, reading
  the TSO userid via a hardcoded low-memory PSA→TCB→TIOC pointer
  chain) would segfault on Linux; replaced with a fixed `TSOID =
  'PLAYER  ';`.
- **The shared-mainframe login/authorization gate** (checks a
  user-id list stored — obfuscated via a bit-negation trick through a
  `BASED` overlay on a `FIXED BIN` — in `STORAGE` record 1, plus
  trading-hours/demo-mode restrictions) is meaningless for a
  single-player standalone build, so `WIZARD` is forced true right
  before that check to skip it entirely. See "Known limitations" for
  what that changes in play, and flip the one line back if you want
  the vanilla restrictions.

## Native replacements for MVS/TSO/assembler externals

The original external routines (`CLRSCRN`/`DECDATE` assembler,
`TREAD`/`TWRITE`/`WHISPER`/`ITIME`/`WARNMSG` from the `WELLPUT`
WYLBUR/TSO terminal-I/O assembler module found in `DECDATE.txt`,
`RANDU` Fortran, `R062A10` MVS dynamic allocation) are reimplemented
as plain PL/I in this directory, using raw Linux syscalls
(`OPTIONS(ASM LINKAGE(SYSTEM)) EXT('_pli_Syscall')`, the same
mechanism Iron Spring's own runtime library uses internally) for
console I/O and file access:

| File | Replaces | Notes |
|---|---|---|
| `tread.pli` | TREAD (WYLBUR TGET) | byte-at-a-time `read(2)` up to `\n`, so it works identically over a pipe or a real tty. Folds the line to upper case, which is what the assembler's `OC 0(133,R2),BLANKS  UPPER CASE THIS SUCKER` did in EBCDIC; without it the engine only understands commands typed in capitals. End of file on stdin has no mainframe counterpart, so it ends the process instead of spinning in `GETIN` forever. |
| `twrite.pli` | TWRITE (WYLBUR TPUT) | `write(2)` + `\n`, plus a code-page step that encodes bytes ≥ `X'80'` as UTF-8 on the way out — the four non-ASCII bytes in the database (see below) are then rendered by a modern terminal the way a 3270 rendered them. Set `XLATE8` to `'0'B` for raw Latin-1. |
| `clrscrn.pli` | CLRSCRN (assembler) | ANSI clear-screen escape |
| `randu.pli` | RANDU (Fortran) | direct translation of the classic (and famously bad) RANDU LCG; line for line the same as `tapecave/RANDU.txt` |
| `itime.pli` | ITIME (assembler) | seeds off `TIME()` builtin instead of the S/370 `TIME TU` instruction, `MOD ... ,512` like the assembler's `N R0,X'000001FF'`. (The engine throws the value away — the `CALL RAN(1)` inside the loop it drives is commented out in the original source.) |
| `decdate.pli` | DECDATE (assembler) | recomputed via PL/I's `DATE()` builtin + a day-of-year table, instead of replicating the `TIME`-macro `STCM R1,7` packed-decimal `YYDDD` extraction |
| `r062a10.pli` | R062A10 (MVS DYNALLOC) | no-op — files are just opened directly by name on Linux |
| `warnmsg.pli` | WARNMSG (TSO broadcast) | no-op — no other logged-on users to notify |
| `whisper.pli` | WHISPER (3270 dark-field password mask) | no-op — typed passwords are visible (this build has no real secrets to protect anyway) |
| `stgwr.pli` | *(new — not a mainframe routine)* | works around Iron Spring's unimplemented `REWRITE`; see above |
| `plibool.pli` | *(new — patched compiler runtime)* | fixed copy of Iron Spring's own `lib/source/bool.pli`; see below |

### `plibool.pli` — why a compiler runtime routine is shipped here

`plic` calls the runtime's `_pli_Bool` for `&`, `|` and `^` on bit
strings wider than 32 bits. For a unary `^` it passes the operand in
the routine's "Y" argument slots and leaves the "X" slots —
`ptrarray(3)` and `ptrarray(4)` — **uninitialised**. But `_pli_Bool`
dereferences `ptrarray(4)` before it ever looks at the function code,
guarded only against an exactly-zero pointer. Every `^` on a long bit
string therefore dereferences whatever stack garbage happens to sit in
that slot.

`adventure.pli` does exactly that in two places: `ATABB(TABNDX) =
^ATABB(TABNDX);` while loading the encoded vocabulary (326 times at
start-up) and `IDB = ^IDB;` in `VOCAB` (three times per word looked
up, i.e. several times per turn). The garbage pointer is usually a
readable address, which is why the game mostly ran — but it was one
unlucky stack layout away from `SIGSEGV` at any moment, and it died
reproducibly on entry to wizard mode ("Fatal error # 99").

`plibool.pli` is Iron Spring's own `bool.pli` with four added lines
that make the routine ignore the operand its function code says is
unused, and give `lenX`/`lenY` defined values. `build.sh` compiles it
with the same flags the runtime library itself is built with and links
it ahead of `libprf.a`, so the archive member is never pulled in.
`diff -u "$PLIDIR/lib/source/bool.pli" plibool.pli` shows the entire
change.

## The game database

`DATABASE.dat` on the tape is the actual game content (room
descriptions, objects, vocabulary, hints, magic messages — the
"DATABASE FORMAT" sections 1–12 documented in `adventure.pli`),
deliberately disguised to look like a linkage-editor object deck so
casual dataset browsing on the mainframe wouldn't spoil the game. The
disguise is a per-record 16-bit two's-complement negation, reversed by
the `REVERT` procedure inside `adventure.pli` itself (search for
`REVERT: PROCEDURE`) — the game's own built-in `WIZARD` → `DECOD`
command runs this in place to let you edit the database.

`decode_database.py` is a line-for-line transcription of that
procedure (the PL/I original is quoted in its header) and regenerates
`decoded_database.txt` from `../DATABASE.dat`. The 29 leading ESD
cards are dropped, exactly as the engine's own read loop drops them
(`IF SUBSTR(CARD,3,1) = 'S'  /* IGNORE ESD CARD */  THEN GO TO
L1002;`), leaving 2004 records.

Output is Latin-1, one byte per character, so `build_object.py` can
pad every record back to exactly 80 columns — the engine parses
section text as `(F(8),14 A(5),A(2))` and calls `BUG(0)` if columns
79-80 of a text card are not blank, so a multi-byte encoding here
would shift text into them. Only four bytes in the whole database fall
outside ASCII: `X'4A'` (cent sign in cp037) in the `"FEE FIE FOE FOO"
¢SIC!` and `¢WITT CONSTRUCTION COMPANY!` signs, and `X'5F'` (logical
not) twice in the wizard help line for the `SCAN` `¬DEMO` subcommand.
`twrite.pli` turns those into UTF-8 at the terminal.

## Known limitations / things not (yet) replicated faithfully

- **The clock runs on UTC.** Iron Spring's `DATE()`, `DATETIME()` and
  `TIME()` builtins all return UTC regardless of `TZ`, where the
  mainframe returned local time. Everything stays internally
  consistent (elapsed-minute arithmetic is unaffected), but the
  "MONDAY 31 AUGUST 2026" banner, the wizard `SCAN` listing's "on
  <date> at <time>", and the one-hour SUSPEND/RESTORE lockout are all
  in UTC. Near midnight the displayed date can be a day off from the
  player's own.
- **`WIZARD` is forced on**, which is not just an extra set of
  commands: it also makes the engine print the database-capacity
  report at start-up ("11085 of 11200 words of messages" …, normally
  wizard-only), skip the 1500-turn limit, skip the 150-minute limit on
  SUSPEND, skip the 2-hour half of the RESTORE lockout, and append `!`
  to the `SCAN` listing. Removing the `WIZARD = '1'B;` line restores
  vanilla play, but then wizard mode becomes unreachable: `WIZPROC`
  grants it only when `TREAD` returns both the text `RC=04` *and*
  `CCODE = 11` (a 3270 PF-key number), and there are no PF keys here.
  Without the force you would also want to seed `STORAGE` record 1
  with an authorized user id, or the first turn drops into the
  "unauthorized, demo game?" path.
- `ADDUSER`/`DELUSER`/`SEND`/`GETMSGS`/`SCANSTG` (wizard/sysadmin
  commands for managing a shared multi-user system) still hit Iron
  Spring's unimplemented `REWRITE` and silently no-op, same as SAVE
  used to. Not fixed, since they're not meaningful for solo play —
  could use the same `STGWR` trick as `CIAO`/`PUTBACK` if that ever
  matters.
- `DECDATE`'s "magic parm of the day" wizard-login puzzle
  (`SUBSTR(INPARM,1,4) = '$¢^%'` checked against `DAY#`) is left
  intact but moot, since `WIZARD` is forced true unconditionally. It
  would not work as written anyway: the guard
  `IF SUBSTR(INPARM,5,1) > 'Z'` relies on EBCDIC collation, where the
  digits sort *above* the letters; in ASCII they sort below.
- The `ON ATTENTION` handler still walks the same MVS PSA/TCB/JSCB/
  RLGB/ECT pointer chain we had to patch out of the TSOID lookup — it
  should be unreachable in this build (nothing raises PL/I's
  `ATTENTION` condition), but a Ctrl-C during play hasn't been tested
  and could theoretically hit it.

## Correctness pass, 2026-08-31

Reviewed against `PROGRAM.txt`, `DATABASE.dat`, `RANDU.txt` and the
`WELLPUT` assembler in `DECDATE.txt`. Fixed:

1. **Random `SIGSEGV` / "Fatal error # 99"** from the compiler
   runtime's `_pli_Bool` reading an uninitialised argument on every
   `^` of a `BIT(40)` — i.e. on every vocabulary lookup. Wizard mode
   crashed the game 100% of the time. Fixed by `plibool.pli` (above).
2. **Lower-case input was rejected.** `tread.pli` did not reproduce
   the assembler's `OC ...,BLANKS` upper-case fold, so `take lamp`
   failed where `TAKE LAMP` worked. Fixed.
3. **The elapsed-time clock never advanced.** The overlays on
   `DATE_STG` had been converted to plain copies refreshed after each
   `DATE_STG = DATETIME();`, which silently dropped the two places the
   engine writes *through* an overlay. `TIME_CHR = TIME();` in the
   scoring code no longer updated `PIC_HH`/`PIC_MM`, so the final
   "…using N turns in M minutes" always reported the elapsed time as
   of program start (measured: 0 minutes reported after 130 seconds of
   play; now 2). Fixed by the `BASED`/`POINTERADD` overlays.
4. **The wizard `SCAN` listing showed the wrong time.** Same cause:
   `PIC_HHMM = HHMM;` in `SCANSTG` no longer wrote the saved game's
   time into `DATE_STG`, so the listing printed the current clock
   instead of when the game was suspended. Fixed.
5. **`decoded_database.txt` was UTF-8 while `build_object.py` read it
   as Latin-1**, so the three records containing a `¢`/`¬` were one or
   two columns wide of the fixed 80-column format (harmless only
   because columns 79-80 happened to stay blank), and one wizard help
   line had been truncated by a byte to compensate, losing its final
   `.`. The END card also carried 40 bytes of junk, because the
   decoder used ASCII blanks on an EBCDIC buffer and skipped
   `REVERT`'s `SUBSTR(CARD,33,20) = ' ';` for `EN` cards. All four
   records now match a faithful `REVERT`, and the decoder that
   produces them ships as `decode_database.py`.
6. **End of file on stdin hung the game** in `GETIN`'s
   `DO WHILE (WORDSTRT = 0)` loop forever. `tread.pli` now exits.

   **Correction, later the same day:** that first attempt did not
   actually work, and this file said it did. The fix was written as

   ```
   DO WHILE('1'B);
     RC = SYSCALL(3, 0, ADDR(CH), 1);
     IF RC <= 0 THEN DO; EOFHIT = '1'B; LEAVE; END;
   ```

   and **Iron Spring PL/I 1.4.1 mis-compiles that `LEAVE`**: when the
   statement sits inside a nested non-iterative `DO; … END;` block, it
   leaves only that block instead of the enclosing iterative loop, as
   PL/I defines. Confirmed with a minimal repro, and visible under
   `strace` as an endless `read(0, "", 1) = 0`. The loop is now driven
   by a `DONE` flag with no `LEAVE` at all, and the game exits with
   status 0 at end of file. (`adventure.pli` itself never uses `LEAVE`
   — every occurrence in it is inside a comment — so nothing else in
   the port was affected.) The lesson for anything else built with this
   compiler: check the exit status, not just the output.

Note for reference: `../decode.py` (the standalone decoder in the
parent directory) has the same two decoding bugs mentioned in 5 —
it never blanks columns 33-52 of the `EN` card, and its
`bytedata[76:77] = '  '` / `bytedata[78:79] = '  '` assignments splice
two bytes into one-byte slices, which grows the record to 81 bytes and
shifts everything after column 77. `decode_database.py` supersedes it.
