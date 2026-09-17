# Legend port — notes

How LEGEND was recovered and rebuilt, what was wrong with it and with its
libraries, what SINTRAN III the program needs, and how the port was checked
against SINTRAN III itself.

## Where the game came from

Six NDFS floppies of the Swedish computer club DNF, from ndlib.hackercorp.no:
LUNDIN-1, -2, -4, -5, -6 and -8 (`ND-disk-00369`, `00310`, `00344`, `00370`,
`00311`, `00345`).  Magnus Lundin's `22-BAK-LEGEND` mode file copies the game
to two floppies called `L-1` and `L-2`: LUNDIN-1 and LUNDIN-2 are that
backup set.

| file | floppy | what |
| --- | --- | --- |
| `LEGEND-LU:ZYMB` | LUNDIN-4 | **v10.0 of 22 May 1987**, the newest; `2LEGEND-LU:SYMB` beside it compiles it |
| `LEGEND-LU:SYMB` | LUNDIN-1 | v10.0 of 19 May: reads eight player fields with `ASC()` and none of the teleport positions, so it cannot read what it writes |
| `LEGEND-LU:SYMB` | LUNDIN-2 | v9.11 of 29 August 1986 |
| `RUMFIL-0..8-LU:DATA` | LUNDIN-1 | the rooms; RUMFIL-5..7 are empty on the disk too |
| `SPELARE-1..12-LU:DATA` | LUNDIN-2 | the players of the twelve worlds, as v9.11 wrote them |
| `SAKKARE-1..12-LU:DATA` | LUNDIN-1 | the things in them: none (every file says 0) |
| `INSTR-LEGEND`, `HELPFIL`, `BROAD-LEGEND`, `BORT`, `VEMFIL` | LUNDIN-2 | instructions, help, the games master's message, the abort and who-plays files |
| mode files, `CHK-LEGEND`, `NY-VARIABEL`, `KOLLA-RUM`, ... | LUNDIN-2, -4 | the author's tools; `..\src_original\mode-files\` |

The source says "(C) A Hedström & M Lundin 84-87"; the instructions call the
game "stulet från Anders Hedström, sysop på databasen RatHole i Skellefteå".
No compiled LEGEND survives: the mode file dumps it to user DRAGON, which is
on none of the floppies.

**The world.**  Rooms 801-900 (RUMFIL-8) are the village: a hundred rooms,
every exit leading to one of them, all reachable from the market street
(839).  RUMFIL-0..4 hold an unfinished wilderness (rooms 1-422) that nothing
leads to; its records have errors (room 13 has seven exits, 131 five, 184 no
exit line) that would stop the game's reader, and its exits lead to rooms
423-799 that do not exist.  Only the SuperUser, by typing a room number,
could go there.

**The players.**  v10.0 stores class and brotherhood as numbers where v9.11
had words, and adds three teleport positions; LUNDIN-4's source reads that.
`tools\convert_players.py` rewrites the nine worlds v10.0 offers (1-9) the
way v10.0 writes them — the author added fields to these files the same way
(`NY-VARIABEL-LU`).  One record needed mending: v9.11 wrote an extra empty
line after *Namnlös* for a player who gave no name.  `tools\make_data.py`
builds `data\` from the floppy files.

## The toolchain

`2LEGEND-LU:SYMB` (LUNDIN-2):

    @RECOVER BASIC-H00
    DEFAULT-INTEGER
    TABLE-SIZES 1500,35000
    COMPILE LEGEND-LU:ZYMB,0,(SCRATCH)USER-STAT:SYMB
    @NRL
    SIZE 2700
    LOAD (SCRATCH)USER-STAT:SYMB
    LOAD (DNF)LIBRARY-MJ:BRF
    LOAD (SYSTEM)BASLIBR-H00:BRF
    DUMP (DRAGON)LEGEND-LU:PROG

- **The compiler**: `BASIC:PROG` on LUNDIN-8, "BASIC COMPILER, JANUARY 85" —
  byte for byte the `SYSTEM/BASIC:PROG` of the SINTRAN III VSX/500 L pack the
  reference machine runs (sha1 `1fc9e7681958`).
- **The club library** `LIBRARY-MJ:BRF`: NILSSON-3 (`ND-disk-00305`),
  1985-10-19, Mikael Johansson's; 95 units (terminal, monitor calls, strings).
- **ND BASIC's run-time library** `BASLIBR-H00:BRF`: HUMBUG (`ND-disk-00437`
  and `-00437c`, the same file in both), 1988-05-27.  **Damaged** — see below.

Built on the reference machine by `tools\rebuild.py` (commands in
`tools\build.cmd`, output in `build\build.log`):

| program | from | statements | sha1 |
| --- | --- | --- | --- |
| `data\LEGEND-LU.PROG` | `basic\LEGEND-LU.ZYMB` + `build\LIBRARY-MJ.BRF` + `build\BASLIBR-H00.BRF` | 1151 | `0085918b2d11` |
| `build\original\LEGEND-LU.PROG` | the ZYMB as recovered + LIBRARY-MJ as recovered + `build\BASLIBR-H00.BRF` | 1122 | `099a64b1506c` |

Both are one bank, from 037200, started at MAIN (037200).  NRL leaves no
entry undefined.

## BASLIBR-H00: the lost sector

Loading the library, NRL stops: `ILLEGAL BRF-CONTROL NO.` (at 000276).  In
both images of HUMBUG the 512 bytes at file offset 2048-2559 — one floppy
sector — are garbage followed by zeros; everything before and after is sound.
Seven entries LEGEND needs were in it or cut off by it.

**The format.**  The record types are in the table "Binary Relocatable Format
Code" of the SINTRAN II Operator's Guide (ND-60.044.01): a control byte and
16-bit parameters — LF (01) a word, LR (02) a word plus the program base,
SFL/AFL/SFR (08-0a) the location counter, LBR (0d) a library entry, ENTR (0e)
an entry here, BEG (0f), REF (10) a word holding an external address, END (11)
with a checksum, EOF (13), LNF (14) a block of words.  Found by taking the 77
sound units apart (`tools\brf.py`):

- a name is two words of five 6-bit characters (ASCII and 077), right-aligned:
  `7INIT` is 0x3724E254, `7ASI` 0x00DC14C9; after control byte 1a (LIBRARY-MJ
  uses it) names take three words;
- END's word makes the 16-bit sum of every control byte and parameter word of
  the unit, END's own included, come to zero;
- a unit's first word lands at PB+1.

**What the sector held.**  The BASIC compiler has the library linked into it,
in the library's order (`tools\place_units.py` finds 58 units there at their
exact relocated values).  Between unit 7INIT and unit 7INNN/7UTTT/7RANT it
holds: the rest of 7INIT, then 7GOP (17 words), 7FNP (129), and the unit
7IOIN/7INB/7OUTB whose end survived after the hole.  NO-GA/BACKGAMMON:PROG on
the SINTRAN pack, a BASIC program linked with the same library at another
address, shows the rest: three one-word units 7EXCB, 7DBUG, 7PBAS (which the
compiler supplies itself); 7GOP and 7FNP are stacks — one word 1, the rest
only reserved, which is why BACKGAMMON's copy holds leftover code there and
the compiler's its own data; 7INIT ends in 129 zero words and a reference to
77ERR; the addresses 7INIT refers to (7GOP, 7FNP, 7IOIN, 7EXCB, 7DBUG, 7PBAS)
and the compiler's 7BIO (7INB, 7OUTB) place the rest.

`tools\rebuild_baslibr.py` writes those units back.  Of the ways the unknown
parts could be encoded, one comes to exactly the 512 bytes of the sector:
7INIT's zero words as a block, the one-word units each a block, the stacks
LF 1 and SFR.  It agrees with every byte that survived on either side, and
**the checksum of unit 7IOIN — written in 1985, surviving after the hole —
comes out right**.  258 bytes differ from the damaged file, all inside the
sector.  The only thing that cannot be recovered is the order of the three
library names at the head of 7IOIN's unit (the checksum does not depend on
it); they are written in address order.

With the rebuilt library (sha1 `f58d8e5d05cd`) NRL loads what LEGEND needs without
complaint and LEGEND runs, on SINTRAN and on the port.

## LIBRARY-MJ: GUSNA

The club library's GUSNA:

    SWAP SA DB; STA SAVE; LDA I 0,B; LDX I 0,B; MON 214 ...

MON 214 (GetUserName) wants A pointing at a 16-byte buffer and X the user
index.  LEGEND calls `CALL GUSNA(USER,WHERE(UR(0)))` — the index from RSIO
first, the buffer second — so with this library SINTRAN writes the name at
the address USER and LEGEND reads an empty name.  LEGEND lets its guests, ten
signatures including the author's own `LU`, play only from user DNF: with an
empty name every one of them is thrown out ("Är du insläppt på DNF kan inte
köra Legend från !").  The copy the club linked with in 1987 must have taken
the buffer from the second argument; `tools\fix_library_mj.py` changes that
one word to `LDA I 1,B` (045400 to 045401) and the unit's checksum
(`build\LIBRARY-MJ.BRF`, sha1 `614db2b405cf`).  The port answers MON 214 with
DNF.

## The port's fixes

At the user's direction (as for Mordor: fix what a player cannot be expected
to understand), in `basic\LEGEND-LU.ZYMB`, each marked `Port fix 2026`
(`basic\fixes.diff`).  Most are v10.0 regressions — the version was three
days old, a rewrite of v9.11's command interpreter and player records:

| lines | bug | since |
| --- | --- | --- |
| 15610 | a dead player's SOMN saved as N$ (NUL); `INPUT#` cannot read it back: error 271 on the next start, and the error routine then saves the half-read world | v10.0 |
| 9940 | two-word commands split at I (the verb loop) instead of I19 (the space) | v10.0 |
| 10080-10087, 6454, 6325 | BLIXT=1 set for every two-word command but BLIXT, never for BLIXT; the class check read X before setting it; BLIXT alone threw nothing; a failed BLIXT now does not carry over | v10.0 |
| 6431, 6451 | the verb search stopped at 94 of 96 words (POSITION, POS); `TEMP=I/16+1` put the 16th, 32nd, ... word (SLU, HEL, PO, TIME, STOP) between the ON lists: "mystifikuskt fel" | v10.0 |
| 7395 | POSITION printed its question instead of asking it | v10.0 |
| 10230, 10236 | VÄLJ switched to NYSPE (past the last player) instead of NYOK | v10.0 |
| 4730-4732 | the menu's status tested SORT (class) for 3 and 0 where GOD (brotherhood) was meant: every Magiker was shown as Laglös | v10.0 |
| 4910, 4922 | a wrong class number asked again from 4880, which adds a player each time | v10.0 |
| 6251 | promotion at level 11 added 3 to the class, not 4 (a Slagskämpe became a Präst) | v10.0 |
| 2120-2210 | the player arrays were 0-20, but a signature with no player in a full world gets index 21: error 332 on quitting | v10.0 |
| 7140, 8400, 8420 | TAPPA compared M$ (Ta Mig Med Snabeln) with MEDL$ (TA MIG MED SNABELN), so nothing was dropped, and lowered CARRY anyway | v9.11 |
| 5990 | a carried body followed you only if it was not player 1 (`>1`) | v9.11 |
| 8680-9034 | the shop compared typed amounts as strings: "2" > "10", "95" > "9" | v9.11 |

Not changed: typing a letter where most other numbers are asked still ends
the game through the error routine (saving first); monsters never throw
lightning (13391 cannot be true); ÅTERUPPLIVA works for anyone carrying a
body (v9.11 kept it for the SuperUser); the SuperUser's BROAD never stops
asking for lines.

The fix at 6451 is written `TEMP=(I+15)/16`, not the `(I-1)/16+1` it started
as, and the reference machine is the reason.  With the first verb in the
table, NORR, `(I-1)` is 0, and on the ND-100 dividing zero is an error:

    $NORR
    ** Error  315
    Kontakta The Boss!

Error 315 is "Overflow in division.  Result set to zero".  A floating zero's
exponent field is 0, so the divide subtracts the divisor's exponent from it
and borrows out of the 15-bit field, which the hardware reports in the error
indicator Z; the run-time clears Z, divides, and tests it (`BSET ZRO SSZ` /
`FDV 0,X` / `BSKP ONE SSZ` at 145033, the one place it checks Z after a
floating operation), so every `NORR` typed in full ended in the error trap.
`(I+15)/16` is the same number for every I >= 1 and divides no zero.

The port's `FDV` did not set Z there and so played on: three of the recorded
sessions were the only difference left between the port and SINTRAN III, and
they are what turned this up.  `src\fpu.c` now flags a result whose exponent
field cannot hold it, and the ports of Skattejakt, SVHA and Mordor have the
same fix (SIMH and nd100x flag division by zero only, so this is not
something another emulator would have told us).

## SINTRAN III as ND BASIC needs it

Added to the Skattejakt/SVHA/Mordor monitor calls (`src\sintran.c`), each
checked on the reference machine:

| | the port |
| --- | --- |
| terminal input | **with even parity**: ND BASIC's line input ends at 0215, rubs out at 0377 or 0201, clears the line at 021 |
| `@TERMINAL-MODE Y,...` (COMND) | capital letters: `a`-`z` and `{ \| }` (ä ö å) typed become capitals, echo included; `~` and `` ` `` stay as typed |
| MON 3 ECHOM 1 | echo everything but control characters: Return is not echoed (the program starts the new line) |
| output NUL | not sent: the runtime writes one after every prompt, and the reference terminal shows none |
| MON 13 CIBUF | forgets the console's typed-ahead keys (a pipe's bytes are kept) |
| MON 117 RFILE past the end of a file open for writing | zeros: ND BASIC reads a block of a virtual array (`DIM #`) before it first writes it |
| MON 143 RSIO | the terminal's logical device number, 1 or `--terminal`; LEGEND shows it (VEM, HJÄLP) |
| MON 214 GUSNA | DNF |
| MON 16 MGTTY | at a console, a screen that can back up (0166006): ND BASIC then rubs out a character with BS SP BS instead of printing `^`; the reference's telnet terminals say 0 |

The program's own start-up (`CALL SLOW`) waits until MON 67 OSIZE reports 64
free bytes, like Mordor's IFBYTE: the port answers 64.  `CALL HOLD` pauses
are kept (`--no-hold` skips them).

## The club's names

Every player record holds two names: the character's (MEDL$, what the game
shows in a room and what DÖDA takes) and the name of the member who made him
(NAM$, asked as "Vad heter du?" and printed by VILKA).  Some twenty of the
club's members wrote their real names there in 1986-87.  Those are not ours
to publish, so `tools\blank_names.py` has put LEGEND's own word for a player
who gave no name, `Namnlös`, in every NAM$ of the recovered files, and two
character names that were a member's full name keep only the first name.
The signatures (up to four letters) stay, and so do the authors' credits in
the program and the instructions — "(C) A Hedström & M Lundin 84-87", and
the game's own joke about "Eru Iluvatar alias Magnus Lundin, creator av
detta spel".

The tool also drops what stood in these files beyond the player count: the
game writes a file from the beginning and leaves the rest of it standing, so
SPELARE-1 still held six deleted players, and SPELARE-2 one more whose name
is nowhere in the live records.  What the members actually typed is on the
author's floppies, which this repository does not publish.

## The reference machine

SINTRAN III VSX/500 L under RetroCore, user DNF,
reached over telnet; `tools\rebuild.py` puts the sources, libraries and the
game's files on the pack (`tools\pack_put.py`), with pristine copies of the
files a game changes under user LEGENDORIG.

To make it repeatable the programs there are copies with three words changed
(`LEGENDF-REF:PROG` for the port's program, `LEGEND-REF:PROG` for the
original):

| port | original | was | now | why |
| --- | --- | --- | --- | --- |
| 131705 | 131442 | `JAF` back to OSIZE, 131775 | `JMP *+1`, 124001 | SLOW does not wait for OSIZE = 64 (RetroCore never says 64) |
| 144174 | 143731 | `MON 11`, 153011 | `SAA 0`, 170400 | RANDOM ignores the uptime ... |
| 144175 | 143732 | `COPY SD DA`, 146115 | `SAA 0`, 170400 | ... so the same typing plays the same game |

The port under `-Z` reads an uptime of 0, which is the same thing.
`tools\reffuzz.py` logs in, puts the game's files back, and plays a game with
the random player (`tools\play.py`), typing only once the game has printed its
prompt — LEGEND clears the input buffer before most prompts, so keys typed
ahead would be lost on SINTRAN and kept by a pipe.  The title shows the time,
and VEM the terminal number: the log keeps both, and `tests\run.py` replays
each session into `legend.exe --raw --no-hold -Z ... --terminal ...` on a
fresh copy of `data\` and compares every byte.

## What the tests say

| | |
| --- | --- |
| `python tests\run.py` | **18 of 18 sessions identical**, 122637 bytes: twelve games of the port's program recorded on SINTRAN III and six of the program as recovered (`build\original\`), replayed byte for byte |
| `python tools\portfuzz.py 60 200` | **0 of 60 games had trouble**, 3349 commands: no hang, no unimplemented call or instruction, no run-time error, no prompt the random player did not know |
| `python tests\consoleplay.py` | 7 checks at a real console (ConPTY): the title, the signature and the menu, a game played, the Swedish letters, the keys |

The sessions are recorded with `sh tools\refbatch.sh` and re-recorded whenever
the program changes; the last difference between the port and the reference
machine was the one that the ND-100's floating divide turned out to explain
(see *The port's fixes*).
