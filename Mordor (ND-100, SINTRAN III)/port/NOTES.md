# Mordor port — notes

How `mordor.exe` came to be: where the source was found, how the original
ND-Pascal compiler turned it into a program, what the port fixed, what
SINTRAN III the program needs, and how the port was checked against SINTRAN
itself.  Octal throughout unless marked.  The ND-100 CPU, floating point and
the file layout are described in the Skattejakt port's `NOTES.md`; this source
is a copy of that emulator with the additions below.

## Where the game came from

**The source.**  The floppy image `ND-disk-00311.img` is
`LUNDIN-6` of the Swedish computer club DNF (1985-87 disks from ndlib.hackercorp.no).
Its directory is empty, but its free pages still hold a deleted file:

- page 151 keeps the deleted object entry `MORDOR-MJ:SYMB`: 36 pages,
  72,393 bytes, created 1985-09-14 12:03, last written 1985-09-29 11:24,
  index block page 86 (now zeroed);
- the text is on pages 94 down to 57, skipping 86 (its index block) and 72
  (the floppy's bit file) — 36 pages, the number the entry records;
- every one of the 35 page joins continues mid-word
  (`M|OD1`, `-C|APTURED`, `SEIZER(WHO); WRITE(' k|ills`...), and the text ends
  `BEGIN MAIN; END.` + CR LF + 0x17 exactly at byte 72,393.  What follows on the
  last page is older text.

`src_original\MORDOR-MJ.SYMB` is those 72,393 bytes, parity and all
(sha1 `ddffcbd5850c`).  The program calls itself
`Mordor: Land of evil.  v7.52 850211`.

**Other versions** (`src_original\other-versions\`, from ndlib images):

| copy | disk | what |
| --- | --- | --- |
| `MORDOR-MJ:SYMB` 59,849 B + `MORDOR-RULES-MJ:DATA` 9,653 B | DNF02 (`ND-disk-00358`) | *Mordor-mission v7.1 820901*: 32 heroes, 13 commands, Dol Amroth. Its rules name the author, **Mikael Johansson**, and thank **Magnus Domellöf**, "without whose original program this program would not have existed". |
| `MORDOR:SYMB`, `MORDOR-MD:PROG`, `MAP-MD:DATA` | MAGNUS-1 (`00377`) | Domellöf's BASIC Mordor (files dated 1980-82) |
| `NEW-MORDOR:SYMB`, `MORD-MODE:SYMB` | MAGNUS-2 (`00314`) | the same BASIC game |
| `MORD-MD:SYMB` | ERIK-1 (`00378`) | the same BASIC game |

The ndlib catalogue lists v7.52's own `MORDOR-RULES-MJ:DATA` (10,851 B, with a
72,660 B `MORDOR-MJ:SYMB`) on MIKAEL-2 (`ND-disk-00301`), but that floppy was
not preserved: only its directory listing survives.

**The instructions** in `data\MORDOR-RULES-MJ.DATA` are therefore v7.1's, from
DNF02, byte for byte (at the user's choice).  v7.52 reads them without
trouble (a line count, `'` for a blank line, `*` for a page break: 12 pages).
Where they describe v7.1 rather than v7.52: the table of heroes lists the 32
that can be chosen (v7.52 shows 8 more, who stay at their citadels) and puts
Imrahil at Dol Amroth (in v7.52 he fights at Minas Tirith); `USE`, the
Palantir, is not among the commands; the heroes are "4-12" (v7.52 takes 4 to 32).

## Two programs

| | |
| --- | --- |
| `data\MORDOR-MJ.PROG` (sha1 `7aca79f7ecaf`) | what `mordor.exe` plays: `pascal\MORDOR-MJ.SYMB`, the recovered source with the port's fixes (below), compiled and linked as described next; listing and object file in `build\` |
| `build\original\MORDOR-MJ.PROG` (sha1 `5bc03dccf0ed`) | the recovered source compiled unchanged, with its listing and object file; `mordor --prog build\original\MORDOR-MJ.PROG` plays it |

## How the programs were made

No compiled v7.52 survives whole (LUNDIN-6's free pages hold two partly
overwritten copies with no index blocks), so the programs were built from the
source with the club's own tools, on SINTRAN III VSX/500 L under RetroCore
(the eXo set-up, with user DNF added using
ndfs-py; `tools\pack_put.py`):

- **Compiler**: ND-Pascal version J 83-12-07, `PASCAL-COD-J`, `PASCAL-LIB-J`,
  `PASCAL-2LIB-J`, `PASCAL-ERR-J` from the club disk `DNF` (`ND-disk-00337`,
  user MUD).  Built as ND's installation sheet says:
  `@NRL` `IMAGE-FILE 100` `SIZE 1500` `LOAD PASCAL-COD-J PASCAL-2LIB-J`
  `DUMP "PASCAL:PROG",11,12`.
- **Licence check.**  The compiler stops with `UNAUTHORIZED USE OF PASCAL`
  (message 43 of `PASCAL-ERR`): its routine at 143046 makes MON 262 with a
  number from its caller and takes the error return to 143064 when SINTRAN
  does not recognise the machine.  At the user's request the jump at 143061
  (`JMP 143064`, 124003) was changed to `JMP *+1` (124001) in the dumped
  `PASCAL:PROG`; nothing else in the compiler was touched.
- **Compile**: `COMPILE MORDOR-MJ:SYMB,"MORDOR-MJ:LIST","MORDOR-MJ:BRF"` —
  NO ERRORS, program 034542B words, fixed data 012172B words, 32 uses of
  non-standard features.  The fixed source (on the pack as `MORDORF:SYMB`):
  NO ERRORS, program 034701B words, the same fixed data.
- **Link** (the recipe of the club's `2ARDA` mode file):
  `@NRL` `SIZE 1500` `LOAD MORDOR-MJ:BRF` `LOAD EXTRA-PAS-LIB`
  `LOAD PASCAL-LIB-J` `DUMP "MORDOR-MJ:PROG"`, with no entries left undefined.
  `EXTRA-PAS-LIB:BRF` (club library, NILSSON-3 `ND-disk-00305`, 1985-12-12)
  supplies `RAN`, `INBYTE`, `IFBYTE`, `DESCF` and `EESCF`; `PASCAL-LIB-J`
  the rest.  (`EXTRA-PASLIB-PBL` on HUMBUG is the 1988 successor.)

Both dumps are one bank, 032200-177677, started at 032211.  The runtime
library has the same MON 262 routine as the compiler, but MORDOR never calls it.

## The port's fixes

At the user's request, three bugs of v7.52 are fixed in the source
(`pascal\MORDOR-MJ.SYMB`; every change is marked `Port fix, 2026` and listed in
`pascal\fixes.diff`).  The club's compiled copies had both.

**Names.**  `LARGE` upper-cased with `CHR(ORD(CH)-30)`; v7.1 had `-32`.  So `'r'`
became `'T'`, and a name only matched as the game transformed it: Frodo was
`FTQ`, Sam `SC`, Merry `MGTT`.  Both deleted compiled copies on LUNDIN-6
contain the same `AAA -30`, so the club played it that way.  Now:

- `LARGE` subtracts 32 over `` ` ``..`~`, and turns `@` (É) into `E`;
- it is declared before `INSTR`, which passes every character typed through it,
  so commands, answers and names work in any case (`frodo`, `ne2`, `y`);
- `INNAME` compares at most 11 characters (a longer answer used to read past
  the typed string).

So *Éomer* is `eomer`, `Éomer` or `EOMER`, *Theodr`d* is `theodred`.

**The fight that never ends.**  `MEMB` counts the fellowship.  If the Ring-bearer
was captured (`RING:=CAUGHT`), later escaped by himself (`CHECK_CAPTIVES`:
"Suddenly X pops up out of nowhere!", `MEMB+1`, but `RING` stayed `CAUGHT`), and
the player then typed `ON`, `ON_RING` said *The Ringbearer flees from his
captors!* and counted him in a second time.  From then on `MEMB` was one too
high, and when the last man standing was captured in a fight,
`SELECT_FIGHTER`'s `REPEAT FIGHTER:=TRUNC(RAN*MEMBERS+1) UNTIL FELLOWS[FIGHTER,1]=IN_FELLOW`
looked for ever.  Random game 11 of `tools\portfuzz.py --original` (Damrod)
does exactly this; replayed on SINTRAN the unfixed program went silent at the
same *(F)ight or (R)un away? F*, and a memory dump of the port there shows no
hero with status 1.  Now:

- `CHECK_CAPTIVES` gives an escaped Ring-bearer the Ring back (`RING:=OFF`);
- `ON_RING` counts him in only if he is not in the fellowship already;
- `FELLOWS_LEFT` counts the fellowship from `FELLOWS`, and `FIGHT`'s
  `CHECK_FELLOWSHIP` sets `MEMB` from it, at the start of every fight and after
  every loss, so no other miscount can leave a fight without fighters.

**The hero screen.**  v7.1 had 32 heroes.  v7.52 made the arrays 40 long for
eight people who stay at home (`ORIG` rows 33-40: Hama, Ceorl and Éowyn at
Edoras, Orophin and Galadriel in Lórien, Elladan and Ellrohir at Rivendell,
Grimbeorn at Dale; status only, strength and the rest 0): the citadels count
them as defenders.  The hero screen kept v7.1's cursor (1-32, `Finished` at
33) but drew `FOR I:=1 TO MEMBERS`, so Hama was printed where `Finished.` then
overwrote him and the other seven stood after it where the cursor never went.
Now it draws 1 to 32, as v7.1 did; the program changes in one word (`SAA 50`
to `SAA 40` at 065051).

None of the fixes calls `RAN`, so everything else plays as before.

Esc still breaks a busy program: the port checks the console for Esc and
Ctrl-C every 65,536 instructions while the program runs, as SINTRAN broke at
once (`tests\consoleplay.py` tries it on the unfixed program's endless fight).

Not fixed (not asked for): `GET_RING` and `FIGHT_TRAITOR` test `FELLOWS[I,1]`
(the main program's `I`) instead of `FELLOWS[I1,1]` when the fellowship is
wiped out (the game ends there anyway); `FIGHT_TRAITOR` adds to `STRENGTH`
without setting it first; swept away at a ford,
`WHILE PLAN^[NX,NY]=RIVER DO NX:=NX+1` could run past row 127 (`T-`: unchecked).

## The club library

The routines Mikael Johansson's programs share (`EXTRA-PAS-LIB`), at their
addresses in the original program (the fixed one has them 137B words later):

| routine | at | does |
| --- | --- | --- |
| `INBYTE` | 066743 | BRKM 0, ECHOM -1, INBT from the terminal, strip parity, ECHOM 1 |
| `RAN` | 066765 | seed := (seed + low word of MON 11) × 012465 + 033031, as a real in [0,1) |
| `DESCF`, `EESCF` | 066776, 067000 | MON 71, MON 72 |
| `IFBYTE` | 067002 | **waits until MON 67 (OSIZE) says 64 bytes free**, then MON 66 (ISIZE); a byte waiting is read without echo, else 0 |

Two consequences:

- `RAN` stirs in SINTRAN's uptime (1/50 s since start) on every call, so no two
  games on the real machine were alike.  The port does the same with the host's
  tick count; under `-Z` the uptime reads 0 and a game is repeatable.
- `IFBYTE` is only called at start-up, in
  `IF (I IN [8..15]) AND (IFBYTE<>127) THEN ... HALT('You cannot play MORDOR now!')`.
  ND-Pascal evaluates both sides, so `IFBYTE` runs at any hour.  It waits for the
  output buffer to report exactly 64 free bytes: the size of a terminal buffer on
  the club's machine.  RetroCore's terminals report something else, so there
  the unpatched program hangs for ever before its title (with Esc disabled).
  The port answers 64.  DEL typed ahead gets past the lock; `--unlimited` types it.

## SINTRAN III as ND-Pascal needs it

Added to the Skattejakt/SVHA monitor calls (`src\sintran.c`):

| call | the port |
| --- | --- |
| MON 1 on device 0 | the Pascal runtime reads the rest of the command line first: it gets the command's CR, as on SINTRAN (the DEL typed after the command is still there for `IFBYTE`) |
| MON 1 on the terminal | SINTRAN echoes: Return as CR LF, printable characters as typed, nothing else; ECHOM < 0 turns it off. (SINTRAN echoes as the key is struck, the port when the program reads it; that only differs for typing ahead.) |
| MON 1 with the echo on | a line, and the delete keys rub characters out of it before the program is given them: see *Rubbing out* below |
| MON 11 TIME | host ticks since boot in 1/50 s; 0 under `-Z` |
| MON 41 ROBJE | the terminal gets `SYSTEM/TERMINAL`'s entry from the reference pack; files an entry laid out like the pack's |
| MON 50 OPEN | write access codes too; no create or truncate unless the name is quoted |
| MON 66 ISIZE, 67 OSIZE | keys waiting (console only; a pipe says 0); always 64 free |
| MON 73 SMAX | the argument is the *maximum byte pointer*, bytes − 1: the runtime passes 16383 after a 16384-byte map |
| MON 74 SETBT, 75 REABT, 120 WFILE | positioning and block writes |
| MON 117 RFILE | the last, short block of a file reads with zeros after its end (the runtime reads text files a block at a time) |
| MON 143 RSIO | interactive, terminal 1 in and out |
| MON 262 CPUST | answered as SINTRAN III VSX/500 L on an ND-100 with 48-bit floating point |

The map is a Pascal `FILE OF` a packed 128×128 array: 32 blocks of 256 words.
`(SYSTEM)PASCAL-ERR` is opened only when the runtime reports an error; the
port ships `PASCAL-ERR-J` from the club disk as `data\PASCAL-ERR.SYMB`.

## Rubbing out, and Esc

MORDOR reads a name or a command through ND-Pascal's `READ`, which has no
rubbing out of its own, and the library's `INBYTE` sets BRKM 0 ("always
break") and never puts it back — so it looked as though nothing could rub a
character out, and the port passed DEL straight into the command, invisibly
spoiling it.  The reference machine says otherwise.  Typed there, at the
game's own prompt:

    Command: MAX^P            DEL after the X: the map is drawn, so it read MAP
    Command: MA^^P            Ctrl-A twice: the command is P, "Illegal Command."
    Reenter: XYZ_             Ctrl-Q: the line is dropped and begun again

SINTRAN's terminal driver rubs the characters out of its own buffer before
the program is given them, whatever the break strategy, as long as the
program has the echo on.  Its terminals cannot back up, so it shows a
rubbed-out character as `^` and a deleted line as `_` and a new line; a
screen terminal is erased instead (BS SP BS), which is what the port does at
a console.  `src\sintran.c` holds the line and does this; with `--raw` it
echoes `^` and `_`, and then the port's output is the reference machine's,
byte for byte.  Single keystrokes — the hero screen, the DEL that gets past
the day-time lock — are read with the echo off and go straight through, as
they do there.

Esc is a user break, which MORDOR asks for itself (`EESCF`) whenever it wants
a command.  On the reference machine:

    Command: <Esc>
    USER BREAK AT    2177B
    @NONSENSE
    "NONSENSE"
    NO SUCH FILE NAME

    @CONTINUE
    <the title screen: the game starts again from the beginning>

The port gives the same break and the same `@`, but its CONTINUE goes back
into the game where it stood: there is no other way back, and nothing here to
lose it for.  LOGOUT ends the game.

## The Facit terminal

MORDOR draws for the club's Facit screens (the codes are in the club's own
`FACIT-CODES-PBL:TEXT`, NILSSON-3): `ESC Y` row+31 column+31 positions the
cursor, `ESC '` / `ESC (` start and end reverse video, `ESC )` blinking,
`ESC S @` clears the attributes, `ESC J` erases to the end of the screen, FF
clears.  The arrow keys send `ESC A`-`ESC D` and Home `ESC H`, which is what the
hero screen reads, with Esc disabled.  Anywhere else that ESC breaks the
program, and the game goes on the moment the cursor reaches `Finished` (a key
struck once too often, or held down, broke it at the next prompt).  So the
port drops the arrow and Home keys whenever Esc is enabled at the time the
program reads a key; Esc itself still breaks.  `src\term.c` turns the codes into VT sequences on a
console or a non-raw pipe; `--raw` passes the original bytes.  The text is
the Swedish 7-bit set: `@omer` is Éomer, `Theodr`d` Theodréd (sic),
`Domell|f` Domellöf.

## The reference machine

`tools\reffuzz.py` (and `tools\refcap.py` for scripted sessions) play the game
over RetroCore's telnet port as user DNF.  To make the reference repeatable the
programs on the pack are copies with three words changed:

| `MORDOR-REF:PROG` (original, sha1 `1d41dd437c63`) | `MORDORF-REF:PROG` (fixed, sha1 `60c040fd21aa`) | was | now | why |
| --- | --- | --- | --- | --- |
| 067006 | 067145 | `JAF` back to OSIZE, 131775 | `JMP *+1` 124001 | `IFBYTE` does not wait for OSIZE = 64 (RetroCore never says 64) |
| 066765 | 067124 | `MON 11` 153011 | `SAA 0` 170400 | `RAN` ignores the uptime ... |
| 066766 | 067125 | `COPY SD DA` 146115 | `SAA 0` 170400 | ... so the same typing plays the same game |

The port under `-Z` reads an uptime of 0, which is the same thing.  The
drivers wait for the game's own prompts, never for silence: Esc typed ahead on
SINTRAN breaks the program at once, where a pipe only delivers it when read.

`tests\run.py` replays every recorded session into `mordor.exe` (scratch data
directory, `--raw --unlimited -Z`) and compares byte for byte.  As of
2026-09-17: **31 of 31 identical**, 286,831 bytes (the fixed program's games re-recorded after the hero-screen fix: the same keys, 86 bytes fewer each).

- `tests\ref\` (the fixed program, `tools\refbatch.sh`): `game01`-`game12`,
  random games on a new map, typed in a mix of cases (fights and flights,
  Nazgul, Balrogs, mist, night turns, citadels falling, Gollum, the Ring on
  and off, the Palantir, bad commands and names; `game08` reads all 12 pages of
  the instructions), and `game01b-keep`-`game08b-keep`, eight more on the map
  the one before left.
- `tests\ref-original\` (the unfixed program): `fuzz01`-`fuzz06` and five
  `-keep` games, names typed shifted.

The random player is `tools\play.py`; it answers whatever the game asks, so its
games reach the fights and the name prompts.  `tools\portfuzz.py` plays such
games on the port alone: of 150 games of up to 600 commands, the unfixed
program hung in one (game 11, above) and the fixed program in none; nothing
unimplemented, no run-time errors.  `tests\consoleplay.py` plays through a
Windows pseudo console: the day-time lock, the title before any key, the Facit
hero screen with its 32 heroes, arrow keys and Home, arrow keys after
`Finished` that do not break, names in any case, the Swedish letters,
the 12 pages of instructions, the kept map, Esc as a user break at a prompt and
while a game spins.
