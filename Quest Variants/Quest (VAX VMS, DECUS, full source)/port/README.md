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

`test/regress.sh` runs 40 scripted cases against the built game.

---

## What survived, and what it is

The DECUS area <https://www.digiater.nl/openvms/decus/vax85a/quest/> holds
28 files: the complete FORTRAN sources, four MACRO-32 modules, the five
linked VMS images, the object library, and the data files.

There are two copies of those 28 files.  `../src_original/` is the web
download, converted to plain stream files on the way.  `../quest/` is the
same area **as RMS stored it on the VAX disk**: variable-length records
still carrying their length words, relative and indexed files block for
block, and the 1984-85 file dates.  The conversion damaged `DUNGEON.DTA`
and cut `CHARACTER.DTA` down to five records, so `data/` is built from the
VMS copy.  `tools/verify_web_copy.py` re-derives every byte of all 28 web files
from the VMS copy, so the relationship between the two is exact, not
assumed.

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

`character.dta` still contains the characters that were live on the Ball
State VAX when the tape was cut: **55 of them, on 39 accounts**, recovered
from the half of the indexed file that survives (see below).  ISMEL, a
level 13 cleric with 1,027,376 experience points, is still flagged as
adventuring, four levels down in Thorlyn's Manor, where he has been since
1985.  `Ahman`, a level 8 cleric, is on the author's own account, and has
the same name as the resident cleric of the temple of Ra.  `P` at the main
menu lists them all.

---

## DUNGEON.DTA: damaged on the web, whole on the VAX disk

**The web copy is damaged, and the game cannot run on it.**  An independent
restorer (*El Explorador de RPG*) hit the same thing and had to rebuild the
file too.  The VMS copy in `../quest/` is undamaged, and it shows exactly
what went wrong.

`DNDOP` writes the dungeon with

    WRITE(21'N,3) K
    3  FORMAT(I4)

into a relative file of `RECL=4`, so every record is four characters, right
justified.  Whatever converted the file for distribution treated the first
character of each record as a FORTRAN carriage-control byte and **applied**
it: the character was consumed and replaced by the control it stands for.

    ' '       ->  LF (0x0A)     value recoverable, it was a blank
    '1'       ->  FF (0x0C)     value recoverable, it was a 1
    '2'..'9'  ->  LF (0x0A)     leading digit LOST

It also wrote out only the records that exist.  On disk the file is 106
blocks: a prolog, then 102 cells a block, each a control byte (`08` = the
record exists) and four characters.  There are 10203 record numbers, and
**records 520-549 were never written**; their control byte is 0.  Dropping
them moved every level after dungeon 1 level 3 thirty records down, so the
web copy's pointer table no longer matches its data.  The gap is exactly the
size of one more column of that level: dungeon 1 level 3 is 30x6 and fills
records 334-519, but the table starts level 4 at 550, which leaves room for
30x7.

`data/dungeon.dta` is the VMS file record for record, with the author's own
pointer table.  The 30 unwritten cells are copied as they are, four NUL
bytes each: nothing reads them, and if anything did, gfortran would refuse
NULs in an `I4` field just as VMS refused a read of a record that does not
exist.  `tools/prepare_data.py` checks the file the way `GETDUNGEON` reads
it: the pointer table leads to 48 levels, every length and width is within
`MAP(40,21)`, every level's stairs-up square holds object 16 or 18 and its
stairs-down square 17 or 19 (level 8, which has no way down, gives `(0,0)`),
no two levels overlap, and every written record belongs to one of them.

### What the earlier repair got right, and wrong

Before the VMS copy turned up, this port ran on a file rebuilt from the web
copy alone.  `tools/verify_web_copy.py` now scores that repair against the
real file:

* **9419 level records** it decoded as proven, including the 144 whose lost
  leading `2` was proved by a surviving `0` (`I4` never writes a leading
  zero): **all correct**.
* **The pointer table.**  The repair called the table "stale" and rebuilt it
  from the chain of levels.  The rebuilt table was right for the file it had,
  so the game ran correctly, but the diagnosis was wrong.  The table was never
  stale; the conversion had removed the 30 unwritten records underneath it.
* **658 ambiguous squares.**  An LF followed by `1`..`7` is either a blank
  (objects 1-7: fountain, the three teleporters, throne, pool, pit) or a lost
  `2` (objects 21-27: 50% treasure, never meet a monster, never find
  treasure, unused, magic three times as often, no magic, dragon).  The
  repair wrote the low object each time.  **503 were right and 155 were
  wrong**: 30 x object 21, 45 x 22, 16 x 23, 44 x 25, 16 x 26 and 4 x 27.
  All 155 are in dungeons 3-6.  The two beginner dungeons contain none of
  objects 21-27, which is why the low reading looked so plausible there.
  The group-size argument made for it (no ambiguous group looked twice the
  size of the `4` group, which must be all low) could not see high objects
  spread thinly over six groups.

**The four dragons** are the squares that mattered most.  `SUBROUTINE
DRAGON` gives dungeons 3-6 one each (`J = DUNGEON-2`, monsters 116-119),
and all four are on level 8, the deepest level:

| dungeon | level | X | Y | dragon | breath |
|---|---|---|---|---|---|
| 3 Gheldron's Caverns | 8 |  1 |  8 | Red    | fire |
| 4 Fallhaven Tunnels  | 8 |  3 |  9 | Blue   | lightning |
| 5 Thorlyn's Manor    | 8 | 10 | 17 | Gold   | poison gas |
| 6 Tombs of Tarasar   | 8 | 34 | 10 | Silver | frost |

Dungeon names are the game's own, from `DUNNAM.DTA`; `DNDOP`'s map printer
calls 3 and 5 "Gheldrons Passages" and "Thorlyn's Maze", which is where the
file names in `maps/` come from.  X and Y as `DNDOP` numbers them.  A
character above level 5 meets the dragon on arrival; `test/regress.sh`
visits all four.  `maps/` was redrawn from the real file and shows `DRA` on
each.

### The other data files

`MAGIC.DTA` and `MORAL.DTA` are relative files (`RECL` 54 and 80) with no
gaps, and `MON.DTA`, `DUNNAM.DTA` and `ACCESS.FIL` are variable-length
sequential files.  The web conversion only added an LF to each record, so
they decode to the same content from either copy.

## CHARACTER.DTA: the whole roster, half of it

`CHARACTER.DTA` is an RMS indexed file (prolog 3, two-block buckets, key and
record compression) keyed on the character name and the owner's user name.
The VMS copy is 75 blocks, but the file's own area descriptor says **149
blocks were in use**, so the copy stops halfway.  The name index names 66
data buckets.  The 31 in blocks 4-66 survive, and the 35 in blocks 76-148 do
not.

`tools/vmsfile.py` decodes the surviving buckets: **55 live characters on
39 accounts**, plus 114 deleted ones, of which RMS keeps only the name.  The
user-name index is a check on that decoding.  Three of its five buckets
survive, every recovered character whose account falls in one of those
three has a live pointer there to its own record ID, and the other 16 fall
in the two lost buckets.  The surviving user-name buckets also point at 47
more live characters whose data buckets are gone.  So **at least 102
characters were live**, on at least 67 accounts.

The web copy's five characters are what a converter recovers by following
the data buckets' next-bucket links from the first bucket.  Block 4 holds
ISMEL, MOTO and NACERIMA, block 8 PARDUE1 and ZACK, and block 10 only
deleted records.  Block 10's link then points at block 138, past the end of
the file, and the converter stops there.

`data/character.dta.orig` is the 55 recovered characters, in key order.  A
new `data/character.dta` starts as a copy of it, and the regression test
works on a scratch copy of both.

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
any dungeon square, print a dungeon (`maps/` was made this way).  The UIC
is also what makes a character yours: each record holds its owner's six
digits right after the user name, and setting `QUEST_UIC` to them — for
instance `083084`, the account that owns ISMEL and the other four in the
web copy — lets you run those characters without their secret names.
`065244` is the author's, and the sources let it run anyone's.

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
    data/         built from ../quest/ by prepare_data.py; the game writes
                  character.dta here
                  character.dta.orig     the 55 recovered 1985 characters
    maps/         all six dungeons, printed by DNDOP from data/dungeon.dta
    tools/        detab.py portify.py  regenerate src/*.f from src_original
                  build_sources.sh
                  vmsfile.py         RMS variable-length, relative, indexed
                  prepare_data.py    ../quest/ -> data/, with structure checks
                  verify_web_copy.py every web file from its VMS original,
                                     and the score of the earlier repair
                  dungeon.py         decoder for the web copy's DUNGEON.DTA
    test/         regress.sh
    build.bat  build.sh

`../src_original/` is the DECUS area exactly as downloaded, and
`../quest/` the same area as it lay on the VMS disk.  Neither is ever
written to.
