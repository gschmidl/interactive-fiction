# Colossal Cave Adventure 751pt, 6.1/9 (PDP-10, TOPS-20) — databases lost

**ADVENTURE < 6.1/ 9>** — an edition of the 751 point Adventure that was
played on a TOPS-20 machine, apparently inside Digital, from February 1981
to October 1982.
The program survives. Its text and its world do not, so it can no longer
be played. This folder keeps what is left, what could be learned from it,
and how it differs from the edition that *is* playable,
[`..\Colossal Cave Adventure 751pt (PDP-10, TOPS-20)`](../Colossal%20Cave%20Adventure%20751pt%20%28PDP-10,%20TOPS-20%29/README.md)
(ADVENTURE 6.1/3, "the 751 port" below).

Nothing from 6.1/3 has been used to fill a gap in 6.1/9. Where the two
are compared, each side speaks only for itself.

```
archive_original\   the files, exactly as they came out of fungames.7z
recovered\          what was read out of them, and the scripts that did it
```

## Where it came from

`D:\GoogleDrive\Mainframe\fungames.7z` is a dump of a TOPS-20
`PS:<GAMES>` directory; its newest files are from mid-October 1982. The
directory's own guide to its games, `games-status.`, is signed
*Marty Brookes, MR1-2/E68, 467-6510* — an address in the style of
Digital's internal mail stops — and BROOKES is one of the people in the
game log.

Among the games are `new-adventure.exe` and everything that goes with it.
The program is bit-identical to the `NEW-ADVENTURE.EXE` that came off the
eXo TOPS-20 pack (`dump_original` of the 751 port), so the collection
already had the binary. The records, the configuration and the
billboard were not there.

The program names its own edition. RFILE and VARGET carry the release as
a floating-point 6.1 and the edit as 9, and the game log prints them as
`VER: 6.10/ 9`. The 751 port is 6.1 edit 3.

The same archive also holds `adv501.exe` — David Long's 501 point game,
the one this grew out of, identical to the copy on the eXo pack — and a
1977 core image of the 350 point original.

## What survives

| file | date | |
|---|---|---|
| `new-adventure.exe` | 13-Feb-81 | the program: 69 pages, no symbol table |
| `new-adventure.cmd` | 20-Feb-81 | a command file that points `GAMES:` at `Z:<GAMES>`, where the game looks for its files |
| `billbd.dat` | 20-Feb-81 | the billboard the game shows |
| `wizdef.dat` | 11-Jun-82 | the configuration: where the eight data sets live, then the wizards' numbers `4,31` `4,100` `4,510` |
| `winner.log` | 13-Jun-82 | the scoreboard, 14 entries |
| `gripe.log` | 2-Aug-82 | three test gripes |
| `avar.bin` | 19-Aug-82 | **0 bytes** — the world file |
| `forots.exe` | 23-Jul-82 | the FORTRAN runtime the site had; every page the game maps is identical to the pack's FOROTS |
| `games-status_` | 28-May-81 | the directory's guide, `games-status.` on TOPS-20 (Windows cannot end a name with a dot) |
| `games.log` | 13-Oct-82 | 103 finished games |

`ATXT.BIN`, the text, is not in the archive at all.

The archive keeps a 36-bit word as five bytes: four 7-bit groups, then
seven more bits and the last bit in the top of the fifth byte. A 7-bit text
file therefore reads as plain ASCII, padded with NULs to 2560 bytes.
`recovered\tools\pdp10.py` unpacks it.

The billboard, in full:

```
 The billboard reads:
 >$<
 Welcome to Adventureland!  Visit beautiful Colossal Cave!
 Open year round for your enjoyment
 This release contains a rebuilt and more powerful PARSER module.
 For a list of legal syntactical constructs, type "SYNTAX".
 Note that if objects outside of the cave occasionally seem to
 get "lost", or if worthless objects sometimes appear seemingly out of
 nowhere, it is *not* a program bug.  But what is going on, you will
 have to learn for yourself.
```

The culprit is presumably the pack rat, whose wizard trace lines the
program still carries: `*** BigRat steals` and `*** BigRat just traded`.

## Why it cannot be played

Three things. The last two were checked by running the original program
on a scratch copy of the 751 port's emulator, where it runs unchanged
once `GAMES:` counts as a disk.

**The text is gone and the world is empty.** Without `ATXT.BIN` there is
nothing to print, and `avar.bin` holds nothing.

**This build cannot rebuild its world.** It is a player's build: the
wizard routines `DIRMOD`, `INITLZ`, `MAINT`, `MAINTX`, `NEWHRS` and
`XMAP` are compiled as one-line stubs in a module called `FOO`. Asked to
start without a world file, it does this:

```
AVAR not found.  Attempting to initialize.

You are attempting an operation permitted only to wizards.

STOP
```

— and on the way it creates an empty `AVAR.BIN`: the same zero-byte file
the archive holds.

**The one 751 point text that survives is the wrong edit.** RFILE works
out 10000 × 6.1 + 9 = 61009 and insists that the text file begin with that
number. The 1982 `ADVTXT.BIN` from the 751 port begins with 61003.
Offered the 1982 files under 6.1/9's names, the program loads the world,
opens the text and stops:

```
ATXT opened in connected directory.

** FATAL PROGRAM ERROR (ERRCODE= 25); Wizard has been notified. **
```

Nothing else in `D:\GoogleDrive\Mainframe` can stand in either.
`srinic.7z`'s `<GAMES>` is 6.1/3 again — its program, text and world are
bit-identical to the eXo pack's — and no other archive there holds a file
named ATXT, AVAR or WIZDEF.

## What was recovered

```
recovered\modules.txt          the routines of both builds, side by side
recovered\strings-6.1-9.txt    every piece of text compiled into 6.1/9, by routine
recovered\strings-6.1-3.txt    the same for 6.1/3
recovered\strings-compared.txt wording one program has and the other lacks
recovered\games-log.tsv        the 103 games as a table
recovered\logs.txt             the records summarised, and every text file made readable
recovered\tools\               the Python 3 scripts that write all of the above
```

Only the program's own text is recoverable — its FORMAT statements, its
character constants and its word tables. The rooms, objects and most
messages were in `ATXT.BIN`. The one room description that survives is
the first, quoted by the three gripes: *You are standing at the end of a
road before a small brick building...*

**The players.** 103 games between 25-Feb-81 and 13-Oct-82, under 13 login
names. SHOOP played 43 of them and reached 369 points; the best game was
KARLSON's 463 on 10-Jul-82. 66 games ended in QUIT, 24 were paused, and
13 ended with the player killed. Three games reached the closed cave, the
endgame — SHOOP twice on 8-May-81, KARLSON on 10-Jul-82 — and all three
players died there. No game ended in an escape.

**What the log records.** After the score, ENVIRN writes two more lines.
The first lists objects for which VALUE reports the full value held: the
treasures scored in full. Only four ever appear: objects 55, 58, 62 and
167. (6.1/3 makes the same call to a routine its symbols name
`OBJVAL(OBJ, FULVAL, CURVAL)`.) The second lists events, from a table of
thirty compiled into the program:

```
SNK DOG BLU CONCH SAFE COMBO WMP CHAIN ANVIL SWD PYR EMR JOSH RUG CHEST
QSAND VINE BUSH ROSE ELF WELL COURT CASTLE KEEP COPTER CTL RM FLOOD
MINE COLLAPSED FALCON VAULT GARDEN
```

Every one of them was reached by somebody except ELF and MINE COLLAPSED.
Where 6.1/3's symbols name them, the events are object states: the
snake's for SNK, the poster's for SAFE, the book's for COMBO, the wumpus's
for WMP, the wall's for JOSH, the dragon's for RUG.

## How 6.1/9 differs from 6.1/3

### The two editions

| | 6.1/9 (this folder) | 6.1/3 (the 751 port) |
|---|---|---|
| edition compiled in | release 6.1, edit 9 | release 6.1, edit 3 |
| text file key RFILE demands | 61009 | 61003 |
| dates | program 13-Feb-81, played to Oct-82 | text 14-Jan-82, program 17-Jun-84 |
| site | apparently a Digital TOPS-20 machine | set up for Stanford's LOTS, by its own messages; home directory `SRC:<GAMES>` |
| build | player build, wizard routines stubbed | full build, wizard routines present |
| size | 69 pages, no symbols | 144 pages, including LINK's full symbol table (35,090 words) |

The two are not simply older and newer: edit 9 was running a year before
edit 3's text was built, and each has code the other lacks.

### Files and the operating system

- **Configuration.** 6.1/9 reads `WIZDEF.DAT`, naming `GAMES:ATXT.BIN`,
  `AVAR.BIN`, `GRIPE.LOG`, `GAMES.LOG`, `BILLBD.DAT` and `WINNER.LOG`.
  6.1/3 reads `ADVWIZ.DAT`, naming `ADVEN:ADVTXT.BIN`, `ADVVAR.BIN`,
  `ADVGRP.LOG`, `ADVBBD.DAT` and `ADVWIN.LOG`, and leaves the game-log slot
  empty.
- **Finding its files.** 6.1/9 relies on a `GAMES:` logical name defined
  outside the program; `new-adventure.cmd` points it at `Z:<GAMES>`. 6.1/3 defines
  its own `ADVEN:` with `CRLNM%` for `SRC:<GAMES>`. If that fails, it says
  *Oh NO! ADVENTURE logical name cannot be defined. Tell a wizard!*
- **System calls.** Both use GJINF, DIRST, STPPN, RUNTM, DISMS, ERSTR and
  HALTF. 6.1/3 adds five, all in ADVMAC:
  - CRLNM, to define `ADVEN:`;
  - RPCAP and EPCAP, to recognise a WHEEL or OPERATOR as a wizard
    (*You are a wheel. You are now a wizard.*);
  - GETOK, from a routine called GETLOD;
  - PSOUT, for the *Oh NO!* message.
- **The game log.** 6.1/9 logs every game, including the line of treasures
  scored in full. 6.1/3's ENVIRN writes events only.

### The world file

Both keep the world in the same COMMON blocks, and all but two have the
same size in both: DWFCOM 30 words, FLGCOM 100, HNTCOM 200, LOCCOM 1000,
OBJCOM 2300, ADJCOM 310, LFXCOM 1300, LIQCOM 20, MNECOM 250, OFXCOM 2000,
PRPCOM 400, TRVCOM 1800, TXTCOM 1200, VERCOM 10 and VOCCOM 3010. The
differences:

- **EVNCOM**, 200 words, exists only in 6.1/9 — the state of its event
  scheduler.
- **WIZCOM** is 25 words in 6.1/9, 60 in 6.1/3.
- 6.1/9's VARGET reads the release word and the blocks one by one, 14,156
  words. 6.1/3 reads the release word and two sweeps of 4,000 and 11,000
  words, 15,001 in all. The files are laid out differently and cannot be
  swapped. Both programs carry the same check against a saved game from
  another edit (*You saved this set of ADVENTURE variables under
  release...*).

### The program

- **Only in 6.1/9:**
  - a parser of 14 routines, roughly 2,500 words: PARSER with PARADJ,
    PARALL, PARALT, PARALX, PARBAD, PARDBG, PAREOC, PAREOL, PARNP, PARPRO,
    PARTAD, ASTACK and RSTACK. 6.1/3 has one PARSER of about 1,500 words.
  - verbs in routines of their own: BURNIT, EATIT, FEEDIT, LOCKIT, MOVEIT,
    PLAYIT, POURIT, READIT, VIEW.
  - an event scheduler: EVENT, SKEDUL, CEASE, CANCEL, RESKED and ENVCLR.
  - DWFSEE, FIN (the final score), GETALT, ROOM, VALUE, and the stub
    module FOO.
- **Only in 6.1/3:** ADVMAC, a MACRO module (ADVLOG, ADVPPN, ADVTIM, AQUIT,
  CRLNAM, COPY, ISWHL, LENGTH, FIXFOR, GETLOD), plus MOTD, NEWHRX and OBJVAL.
  The verbs' messages and the final score live in its main program.
- **Wizard tools.** 6.1/3 has them all: initialisation (*Initialization
  complete.*), maintenance (*New VAR file has been saved.*, the
  short-game length and restart latency), prime-time hours, the treasure
  list (TLIST, which reports the *Maximum score*) and a map dump to
  `ADVMAP.TXT`. In 6.1/9 all of these are stubs. Its TLIST, which nothing
  calls, returns the constant 751.
- **Debugging output only 6.1/9 kept:** the BigRat trace, a dump of *Dwarf
  locs / Pirate / BigRat loc / RATFLG*, and PARDBG's word, object, verb and
  indirect-object lists.

### The parser, from the outside

The billboard offers a `SYNTAX` command with the new parser. 6.1/3
does not know the word:

```
syntax

I don't understand the word SYNTAX.
```

The 6.1/3 parser takes `TAKE ALL`, `TAKE LAMP AND MATCHBOX` and `IT`, but
not `DROP ALL BUT LAMP` (*I don't understand the word BUT.*). What the
rebuilt parser in 6.1/9 accepted went with its text. Going by their names,
its routines deal with adjectives, ALL, pronouns, noun phrases, and the
ends of commands and lines. The parser's built-in word lists differ only
slightly between the two: 6.1/3 has ETHEM, and 6.1/9 has `7/22/34` beside
the `7-22-34` that both carry.

### The wording

`recovered\strings-compared.txt` has the full list. The notable parts:

- **Suspending.** 6.1/9 warns that you *will have to wait at least N
  minutes before continuing*. 6.1/3 just says you can *resume later*.
- **Stanford.** Only 6.1/3 carries LOTS's policing: *NOTE: Open/Closed
  hours are disabled at LOTS.* There is a cave-in when the system load is
  high (*Maybe you should tread on Colossal Cave's grounds when the load
  is not so high*), and a queue of knife-throwing dwarves when the system
  is crowded. Dial-in players hear *Don't even THINK about playing
  adventure on a dial-in! If you want to explore, come to campus.*, and
  PTY users *PTYs are not fit for the likes of Colossal Cave.*
- **Small messages.** Only 6.1/9 has *I have extinguished the...*,
  *There is nothing else I can tell you about the...*, *I'm not sure what
  you want to do with the...* and *? Are you kidding me*. Where 6.1/9
  says *I'm afraid it's full.*, 6.1/3 says *...is full.*
- **Shared.** Both have the health rating out of 100, the lamp's *dwergs
  of power*, the scoring and rating messages, and the gripe and log
  layouts.

## Rebuilding recovered\

```
cd recovered\tools
python modules.py
python strings.py
python logs.py
```

The scripts read `archive_original\` and, for the comparison, the 751
port's `dump_original\ADVENTURE.EXE.36`.
