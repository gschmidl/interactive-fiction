# QUEST — Ball State University, 1984-85 — ported to Windows

QUEST is a dungeon crawler written in VAX FORTRAN by **Chris K. Kelley** at
Ball State University, with **Owen G. Anthony** and **W. T. Konopa**, and
submitted to DECUS for the Spring 1985 symposium tape (VAX85A).  Six
dungeons of eight levels each, 119 monsters, 91 magic items, a magic shop,
a temple, four character classes, and permanent death.  The title screen
dates it: *Version 1.00 December 16, 1984.  Version 3.36 April 27, 1985.*

This directory builds it as a native Windows executable.

    build.bat            (or  sh build.sh)
    quest.exe

`test/regress.sh` runs 35 scripted cases against the built game.

---

## What survived, and what it is

The DECUS area <https://www.digiater.nl/openvms/decus/vax85a/quest/> holds
28 files: the complete FORTRAN sources, four MACRO-32 modules, the five
linked VMS images, the object library, and the data files.

| | |
|---|---|
| `quest.for`   | time-of-day check and terminal setup |
| `quest1.for`  | character menu — create, run, kill, rename, list, sort |
| `quest2.for`  | the city of Exeter — magic shop, temple of Ra |
| `quest3.for`  | dungeon adventuring, 3707 lines |
| `dndop.for`   | operator tool: edit characters, edit and print dungeons |
| `lib.for`     | 1802 lines of shared subroutines |
| `qstcom.for`  | the common block, INCLUDEd by everything |
| `format.mar` `inp.mar` `getname.mar` `debug.mar` | MACRO-32: terminal I/O, `$GETJPI` |
| `quest.exe` `quest1.q7r` `quest2.q7r` `quest3.q7r` `dndop.exe` | the linked images |
| `library.olb` | the compiled `lib.for` + the MACRO modules |
| `dungeon.dta` `mon.dta` `magic.dta` `moral.dta` `dunnam.dta` `character.dta` `access.fil` | the data |

`.Q7R` is not a special format — VMS lets an image have any file type, and
`CALL CHAIN('...QUEST1.Q7R')` runs it.

On the VAX these were five separate images that handed control to one
another with `LIB$RUN_PROGRAM`, passing the player through the 252-byte
process common (`LIB$PUT_COMMON` / `LIB$GET_COMMON`).  Here they are five
subroutines in one executable and `CHAIN` longjmps back to a dispatcher in
`src/main.c`, which closes what the abandoned image had open — a new VMS
image would have started with no files of its own — and enters the next.

`character.dta` still contains the five characters that were live on the
Ball State VAX when the tape was cut, owned by `00AFHOOGENBO`.  ISMEL, a
level 13 cleric with 1,027,376 experience points, is still flagged as
adventuring, four levels down in Thorlyn's Maze, where he has been since
1985.  `P` at the main menu lists them.

---

## DUNGEON.DTA had to be repaired

**The copy on the web is damaged, and the game cannot run on it.**  This is
not a transfer error at this end: the images and the object library arrive
byte for byte, and an independent restorer (*El Explorador de RPG*) hit the
same thing and also had to rebuild the file.

`DNDOP` writes the dungeon with

    WRITE(21'N,3) K
    3  FORMAT(I4)

into a relative file of `RECL=4`, so every record is four characters.  The
file carries RMS "FORTRAN carriage control" (`RAT=FTN`), which means the
first byte of each record *is* a carriage-control character — and it is
also the first digit of the number, because `I4` right justifies.  Whatever
converted the file for distribution **applied** that carriage control:
character 1 was consumed and replaced by the control it stands for.

    ' '       ->  LF (0x0A)     value recoverable, it was a blank
    '1'       ->  FF (0x0C)     value recoverable, it was a 1
    '2'..'9'  ->  LF (0x0A)     leading digit LOST

Every one of the 10173 records in the distributed file begins with LF or
FF, and LF and FF appear nowhere else — the signature of exactly this.

### What the repair recovers, and on what evidence

`tools/repair_dungeon.py` rebuilds the file; `tools/dungeon.py` is the
decoder it uses.  Neither touches `src_original/`.

**Leading digits that can be proved.**  `I4` never writes a leading zero,
so a surviving second character of `'0'` proves the first one was a digit;
map codes run to 2788 at most, so that digit can only be `'2'`.  144 map
records and 5 pointer records come back this way.

**The pointer table (records 1–96) is regenerated.**  The table shipped in
the file is stale as well as digit-lossy: it says dungeon 1 level 4 begins
at record 550, where the dimensions are 20×20, which would overrun its own
level 5 at record 766.  The level data, by contrast, is entirely
self-describing, and walking it proves itself four ways:

* stepping `start -> start + 2 + LL*LW + 4` from record 97 yields exactly
  **48 levels** and ends on **exactly** the last record of the file;
* every length is within 1..40 and every width within 1..21 — the bounds of
  `MAP(40,21)`;
* in all 48 levels the square named by `STAIRSUPX/Y` carries object 18 or
  16 (stairs up, or disappearing stairs up) and the square named by
  `STAIRSDOWNX/Y` carries object 19 or 17 — **90 cross-checks, no misses**;
* exactly six levels have down-stairs of `(0,0)`, and they are chain
  positions 8, 16, 24, 32, 40 and 48 — level 8 of each dungeon, which has
  no way down.  That also fixes which chain position belongs to which
  dungeon and level.

Rebuilding the table from the chain is therefore the only reading
consistent with the data, and `maps/` holds all six dungeons drawn by
`DNDOP`'s own map printer from the repaired file.

### What is lost

A surviving first character of `'1'`..`'7'` under an LF is genuinely
ambiguous: the lost character was either a blank (object 1..7 — fountain,
the three teleporters, throne, pool, pit) or a `'2'` (object 21..27 — 50%
treasure, never meet a monster, never find treasure, unused, magic three
times as often, no magic, **dragon**).  **658 of the 9789 map records are
affected**, and the surviving bytes cannot tell them apart.  They are
written as the low object and listed, one per line with its dungeon, level
and coordinates, in `data/dungeon_ambiguous.txt`.

The low reading is the better estimate, on the file's own evidence:

* object 24 is documented "not used", so all 73 records of the `'4'` group
  must be object 4 — and that group is mid-range among the seven (they run
  68 to 150).  Were the other six groups each carrying a high object too,
  they would stand out as roughly twice the size of group 4.  They do not.
* the objects that *can* be counted exactly — 8, 9 and 10 through 19 — occur
  16 to 88 times each, the same range as the ambiguous groups.

But it is an estimate, not a recovery, and one thing is definitely gone:
`SUBROUTINE DRAGON` gives dungeons 3, 4, 5 and 6 one dragon each (`J =
DUNGEON-2`, monsters 116-119, breathing fire, lightning, poison gas and
frost).  **Those four squares are object 27, inside the ambiguous group,
and cannot be identified.**  Nothing here invents them: guessing four
squares out of 68 would be fabricating map content, and a wrong guess would
put a dragon somewhere the author did not.  If a clean `DUNGEON.DTA` ever
surfaces, `data/dungeon_ambiguous.txt` is the list to correct.

### The other data files

`MAGIC.DTA`, `MORAL.DTA` and `MON.DTA` were stored with ordinary carriage
return carriage control (`RAT=CR`), so each record simply gained a trailing
LF.  Nothing was consumed; `tools/prepare_data.py` drops the LF and checks
that every record is the length its `OPEN` declares (54, 80 and 34 bytes).

---

## The translation

`port/src/*.f` is generated from the untouched `src_original/*.for` by

    sh tools/build_sources.sh      # detab.py, then portify.py

`detab.py` expands VAX tab-format source into standard fixed form.
`portify.py` applies 319 edits, **every one asserted to apply exactly the
number of times it is expected to**, so a change in the input cannot pass
unnoticed.  Nothing else is hand-edited.  The categories:

**VMS run-time.**  `LIB$PUT_COMMON`/`LIB$GET_COMMON`, `LIB$RUN_PROGRAM`,
`LIB$ERASE_PAGE`, `LIB$SET_SCROLL`, `LIB$DAY`, `LIB$LEN`, `SYS$DELPRC`,
`BAS$SLEEP`, `FOR$EXIT`, `SECNDS`, `RAN`, `TIME`, `$GETJPI`, `$QIOW`,
`$FAO`, `SCR$SET_CURSOR` — all in `src/vmsrt.c`.

**The generator is bit-exact.**  `RAN` is VAX `MTH$RANDOM`:
`seed = 69069*seed + 1` modulo 2³², the result `seed / 2³²` unsigned.  A
given seed deals the same dungeon, the same monsters and the same dice as
it did on the VAX.

**Terminal output.**  QUEST drove its terminal three ways at once —
`FORMAT.MAR` and `INP.MAR` went straight to `$QIOW` past RMS; `TRIMMER`,
`OUTNUM`, `VARFORMAT`, `SINGLE` and `PROMPT` wrote through FORTRAN unit 6
with a `'+'` carriage control and a `'$'` descriptor, which together mean
"print here and move nothing"; and a handful of statements wrote ordinary
carriage-controlled records.  All three now go through one unbuffered
writer, so they interleave in the order the program emits them.  VMS
carriage control is reproduced as the FORTRAN RTL rendered it on a
terminal — blank is *line feed before, carriage return after*, `'0'` two
line feeds, `'+'` no prefix, `'$'` no suffix — and because a VT100 line
feed moves down **without** returning the carriage, the console is put in
`ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN`, where
LF behaves the same way.

`$FAO` is reimplemented for the three directives QUEST uses: `!/` carriage
return and line feed, `!_` **horizontal tab** (this is what lines up the
two-column statistics display), `!!` a literal exclamation mark.

**VAX FORTRAN extensions.**  `READ(24'K+1,2)` becomes `REC=`; `A<n>` and
`I<n>` variable field widths become fixed widths or a call to the writer;
`SEED1=SEED1.OR.1` — a bitwise OR on two integers — becomes `IOR`;
`READONLY` becomes `ACTION='READ'`; `ORGANIZATION='RELATIVE'` becomes plain
direct access, which is the same thing for fixed-length records.

**The indexed character file.**  `CHARACTER.DTA` was an RMS indexed file
keyed on the character name (no duplicates) and the owner's user name
(duplicates allowed).  `src/keyed.c` holds it in memory in primary-key
order — the order a sequential read of an indexed file returns, which is
what `LISTPLAYERS` and `SORT` rely on — and writes it back on close.  The
record-locking retry loops (`FOR$IOS_SPERECLOC`) and the `UNLOCK`
statements have nothing to do for a single player.

**Three missing `LOGICAL` declarations.**  `LOCATEMAGIC` and `FOUNTAIN`
call `SAVINGTHROW`, and `CAST` calls `MONSAV`, without declaring them, so
`IMPLICIT INTEGER(A-Z)` typed them as integers.  VAX FORTRAN accepted an
integer in a logical `IF` and tested its low bit; gfortran does not.  The
declarations are added, which is what the author meant — he wrote them in
every other caller.

**Names.**  `SLEEP`, `RENAME` and `KILL` are gfortran intrinsics and
`PRINT` and `CHARACTER` are Fortran keywords, so those five routines are
prefixed (`QSLEEP`, `QRENAME`, `QKILL`, `QPRINT`, `DNDCHR`).  `DNDOP` has
its own `BUILDMAP` and `OPENDUNGEON`, which collide with `QUEST3`'s, so
they become `OPBLDMAP` and `OPENDUN0`.  After linking, `nm` confirms every
call resolves to the game's own routine and not to a compiler builtin.

**Two things behave differently, deliberately:**

* `LIB$ESTABLISH(QUEST_ERROR)` armed a condition handler whose job was to
  print "Jim, the wizard, appears before you… would you MAIL this number to
  00CKKELLEY?" and log the error.  Nothing establishes it now, so a
  run-time error aborts instead.  `QUEST_ERROR` is still compiled.
* `LIB$SCREEN_INFO` was not in the distribution — it came from
  `LIBRARY.OLB`, whose source did not survive.  `CREATE` uses only its
  second value, to choose between the ANSI and the VT52 "erase to end of
  line": `IF(B.EQ.96)`.  96 is `TT$_VT100`, so `SCRINF` in `src/vmsf.f`
  reports a VT100, which is right for the terminal this drives.

Everything else about the game — every message, every die roll, every
table, the time limits, the ageing, the permanent death — is the 1985 code.

---

## Running it

    quest.exe [options]

| | |
|---|---|
| `-f`, `--fast`      | skip the timed pauses the VAX used for pacing |
| `-s`, `--seed N`    | start the generator from N instead of the clock |
| `--freeze T`        | pin the clock, `"YYYY-MM-DD hh:mm:ss"` |

`-s` and `--freeze` together make a session reproducible, which is what the
regression test uses.  Environment variables `QUEST_DATA`,
`QUEST_USERNAME`, `QUEST_UIC`, `QUEST_SEED`, `QUEST_FREEZE` and
`QUEST_FAST` do the same jobs.

**Keys.**  Single keystrokes, no RETURN — the prompt is a single
underscore.  `H` is help everywhere.  Menu: `C` create, `R` run, `P` list,
`Y` list one user's, `F` experience table, `S` sort, `K` kill, `N` rename,
`Q` quit.  Exeter: `D` dungeon, `M` magic shop, `C` cleric, `X`/`Z`/`I`
statistics and inventory, `Q` stop.  Dungeon: `N` `E` `S` `W` to move, `F`
fight, `C` cast, `M` manipulate an item, `U` save, `A` expert map, `Q` time
stop (save and leave), `0` stay put.

If nothing is typed for 24 seconds the game acts as though the key were
`H` — that is `INCHK`'s timeout, and it is original.

**The accounts.**  QUEST asked VMS who you were.  Here the Windows user
name is used, with UIC `000100`.  Two of the sources' checks are on the
author's own account, and `QUEST_USERNAME` / `QUEST_UIC` reach them:

    QUEST_USERNAME=00CKKELLEY QUEST_UIC=065244 quest.exe

adds `O` to the main menu, which runs `DNDOP` — edit any character, edit
any dungeon square, print a dungeon (`maps/` was made this way).  Setting
`QUEST_UIC=083084` instead makes you the owner of the five 1985 characters,
so you can run them without their secret names.

**The clock.**  `ACCESS.FIL` is the one-line file that shut the game down
outside permitted hours: one digit, then two `hh:mm` times that bracket the
hours QUEST is **closed**, on every day but Saturday and Sunday.  As
shipped it reads `000:0000:00` — flag clear, and a closed window of `00:00`
to `00:00`, which the clock can never be inside — so QUEST is always open.
Writing `008:0020:00` restores the Ball State rule the game itself
describes, "available before 8:00 a.m. and after 8:00 p.m… also available
all day Saturday and Sunday"; a leading `1` shuts the gates entirely, and
the little man in the grey robes turns you away.

---

## Files

    src/          the translated FORTRAN, plus the C runtime
                  lib.f quest.f quest1.f quest2.f quest3.f dndop.f
                  qstcom.inc      the common block
                  vmsf.f          QCLEAR/QEMIT/SCRINF
                  vmsrt.c/.h      terminal, $FAO, MTH$RANDOM, $GETJPI, clock
                  keyed.c         the indexed CHARACTER.DTA
                  main.c          the image dispatcher and CHAIN
    data/         the repaired data files; the game writes character.dta here
                  character.dta.orig     the five 1985 characters, pristine
                  dungeon_ambiguous.txt  the 658 squares that cannot be proved
    maps/         all six dungeons, printed by DNDOP from the repaired data
    tools/        detab.py portify.py  regenerate src/*.f from src_original
                  dungeon.py repair_dungeon.py prepare_data.py
                  build_sources.sh
    test/         regress.sh
    build.bat  build.sh

`../src_original/` is the DECUS area exactly as downloaded, and is never
written to.
