# Skattejakt port — notes

How the ND-100 and SINTRAN III were worked out, what the program turned out
to be, and what is still guessed.  Octal throughout, as on the ND.

## Where it came from

`E:\EXO\Skatte Jagt (19xx)\ND-100\SMD0.IMG` is a 75 MB SMD pack, directory
`PACK-ONE`, SINTRAN III VSX/500 L "generated 16 December 1988".  eXo runs it
with RetroCore.  The game is one file:

    GAMES/SKATTEJAKT:PROG   65 pages, 131584 bytes, created 1995-07-01 14:21
    sha1 50600f2aff2f6614de62024ef924ae3dd90bbb06

There is no source, no data file and no other copy on the pack; the
`note.txt` beside it says to type `skat` as user GAMES.  (The SVHA Adventure
pack in `E:\EXO\SVHA Adventure (1979)` is a different pack and has no
Skattejakt.)  `../src_original/SKATTEJAKT.PROG` is the file as extracted.

The pack was read with a small NDFS reader written for this
(`tools/ndfs_ls.py`) from the format description in RetroCoreLabs'
norskdata-ndfs (`docs/NDFS-FORMAT.md`, MIT): master block at 07740 in page
0, object and user files as index blocks, 64-byte object entries, 2048-byte
pages.

## The :PROG file

Block 0 starts with the 7-word header `@RECOVER` reads (NDInsight
`SINTRAN/File-Formats/prog-fileformat.md`, from RP-P2-MONCALLS.NPL):

| word | value | meaning |
| --- | --- | --- |
| 0 | 0 | start address |
| 1 | 0 | restart address |
| 2 | 0 | bank 1 first address |
| 3 | 177777 | bank 1 last address |
| 4 | 177777 | bank 2 first: none |
| 5 | 0 | bank 2 last |
| 6 | 0 | no data-bank copy |

The whole 64K address space follows at byte 01000 (0x200).  Words 10, 12
and 16 of block 0 hold 101, 13772 and 7, then `FTNLIBR'` — unexplained and
not needed.

It is a *dump*: the image was saved after the program had initialised
itself, Woods-style.  Its output buffer still holds `Husk å dumpe ut
memory...` ("remember to dump memory") and its random number state is set.
So a new game is always the same game for the same commands, and the clock
plays no part — which is what made byte-for-byte comparison possible.

## The CPU (src/cpu.c, src/fpu.c)

User-mode ND-100/CX with the 48-bit floating point processor, one 64K bank,
no paging.  Written from ND-06.029.1 (ND-110 Instruction Set; the OCR of its
octal-code appendix swaps a few codes — RDIV shows RCLR's, RSUB shows
RINC's), with the semantics of nd100x's microcode-validated implementation
(GPL, read, not copied) as the tie-breaker.  Points that matter:

- Effective address, mode bits ,X I ,B = 4 2 1:
  P+d, B+d, (P+d), (B+d), X+d, B+d+X, (P+d)+X, (B+d)+X.  P is the address of
  the instruction itself; d is 8 bits signed.
- JPL sets L = P+1 except in mode ,X ,B, where the microcode shares JMP's
  path and does not touch L.
- ADD/SUB/RADD/AAA...: C = carry out (SUB is A + ~m + 1, so C = no borrow),
  O sticky, Q = this operation's overflow.  COPY is RADD CLD and does set C.
- Register field 0 reads 0 and ignores writes, except in bit instructions,
  where 0 is the status register (PTM TG K Z Q O C M = bits 0-7).
- SKP GEQ and LSS look only at the sign of the difference; GRE and LST are
  the overflow-corrected signed compares; MGRE/MLST unsigned.
- Shifts: 6-bit signed count with a 5-bit counter (count -32 is no shift),
  the last bit out goes to M.
- Floating point: T,A,D = sign+exponent (bias 040000), 32-bit mantissa
  normalised to [0.5, 1).  The integer algorithms (guard bit on alignment,
  one renormalising shift, divide rounds up on a remainder) are the ones
  SIMH's ND-100 simulator and nd100x use.

Implemented but never reached by this game (so not verified against the
real machine): MOVB, MOVBF, BFILL, INIT/ENTR/LEAVE/ELEAV, TSET, RDUS, MIX3,
RMPY.  Decimal instructions and MOVEW trap as unimplemented; privileged
instructions and IOX trap.

The game itself uses floating point for words: the parser keeps a typed word
as 6 characters in 3 words and compares it with the word table by LDF + FSB.
That is presumably why a Norwegian Adventure is remembered as needing a 48-bit
floating point machine (spillhistorie.no, "Norges første eventyrspill").

## SINTRAN III (src/sintran.c)

The game makes six monitor calls:

| call | use |
| --- | --- |
| MON 3 ECHOM | A = -1 once at start: SINTRAN echo off; the runtime echoes |
| MON 70 COMND | `  SABLE-ESCAPE-FUNCTION,` and `CC <SKATTEJAKT> CC` at start, `ENABLE-ESC-FUN,` and `CC <EDITOR> CC` at the end |
| MON 113 CLOCK | at start (the suspend check) and for `ÅPENT`; A points at the address of 7 words: ticks, s, min, h, day, month, year |
| MON 1 INBT, MON 2 OUTBT | T = 1, the terminal; skip return on success |
| MON 0 LEAVE | end |

A raw scan of the image finds more MON words (40, 50, 60, 64, 65, 77, 107,
117, 120, 141, 142, 210, 266, 277, 360, 365): FORTRAN library code for files
and error messages that this program never reaches, or data.  Anything not
implemented stops the port with a message naming the call.

The same source runs SVHA Adventure (`GAME=1`), which needed files (OPEN,
RFILE, CLOSE, SETBS), HOLD, BRKM and MGTTY as well; those are described in
the SVHA port's `NOTES.md` and do not touch Skattejakt, whose 15 recorded
sessions still replay byte for byte.

**The mangled command.**  The program's first COMND is meant to be
`DISABLE-ESCAPE-FUNCTION`, but the string in the image begins with two blanks
where `DI` should be.  Typed at `@` on the real system, `  SABLE-ESCAPE-FUNCTION`
does nothing and says nothing, and Esc then breaks the running game with
`USER BREAK AT   53031B` (53031 is the INBT in the runtime's line reader).
The port ignores any command it does not know in the same way.

**Clock.**  RetroCore's SINTRAN runs 28 years behind (17 September 1998 on
17 September 2026); the calendar repeats in 28-year steps, so weekdays
agree.  The port does the same; `-Z` fixes the host time.

## The terminal

Measured on the reference with raw captures (`tools/refkeys.py`):

- The game's first output is LF and EM (031) — the EM shows nothing on the
  RetroCore terminal and is dropped on the console.
- Echo is the runtime's: CR is echoed as CR LF after a word, as CR BEL for an
  empty or blank line, which the game refuses and waits on.
- Ctrl-A deletes a character (echo BS SPACE BS); Ctrl-Q deletes the line
  (echo EOT, 004); DEL and BS are refused with BEL.  So the console's
  Backspace key is sent as Ctrl-A.
- Lower case is accepted.
- National characters are 7-bit NS 4551: [ \ ] { | } = Æ Ø Å æ ø å.  An 8-bit
  byte (E5, å in Latin-1) typed over RetroCore's telnet arrives as `?` —
  RetroCore's doing; an ND terminal sent `}`.
- When the program leaves, SINTRAN writes CR LF before its `@`.

## Suspend and resume

Woods' `SETUP` is the word at **071144**: 2 in the image as dumped, 3 after a
game has ended, -1 after `SPAR`/`UTSETT`/`PAUSE`.  The port offers to write a
dump when the program leaves with -1 there *and* the program read the
terminal while it was not -1 — so a resume the game refuses ("utsatt for kun
10 minutter siden") does not ask again.  The dump is a one-bank :PROG with
start and restart 0, like `@DUMP name,0,0`.  Started again, the program
compares the clock with the time it saved:

- under 30 minutes: *Selv trollmenn må vente lenger enn dette!*, stop;
- 30 to 90: *Er du en trollmann?* — `JA`, then the magic word (read without
  echo) `LURVEN`, then *Vet du hva jeg trodde det var?* — `NEI` prints a
  five-letter challenge (e.g. `RSOTS`) whose answer comes from MAGNUM
  (11111 here, Woods' default; seen as the dividend of the RDIV-by-10 loop at
  034331) and the time;
- 90 and over: the game continues.

LURVEN was found by tracing the compare at 034016 (`FSB I 034143`); the word
at 071135 decodes with the vocabulary key below.

## Vocabulary

Woods' ATAB/KTAB: the parser loop at 031403 walks KTAB through the pointer at
031511 (value = type*1000 + meaning) and ATAB through 031515, 3 words per
entry, 400 entries (limit word via 031517).  Each ATAB word is 6 characters
XORed with **020040 045440 020040**.  `notes/vocabulary.txt` is the decoded
list.

## Checking it

`tests/run.py` replays sessions typed on SINTRAN III under RetroCore and
insists on identical bytes.  They were captured with `tools/refcap.py` (one
command per line, wait for 0.8-1.5 s of silence) and `tools/refkeys.py` over
RetroCore's terminal port (TCP 9000: Esc, `GAMES`, empty password, `SKAT`),
running a copy of the eXo pack, never the original.

- `first_session`, `walk1`: typed and scripted play into the cave; walk1
  ends in a dwarf's knife and a refused reincarnation.
- `keys_*`: the editing keys above.
- `fuzz1`...: 300 random commands from the game's own vocabulary
  (`tools/fuzzgen.py SEED 300`), fuzz1-3 after walking into the cave.

`tests/consoleplay.py` drives the port through a Windows pseudo console:
letters with Ø and Å both ways, Backspace, `SPAR` and the file prompt,
resume, Esc.

## Tools

| tool | what |
| --- | --- |
| `tools/ndfs_ls.py` | list and extract files from a SINTRAN NDFS pack |
| `tools/nddis.py` | disassemble a :PROG (`nddis.py FILE START END`, octal) |
| `tools/ndtel.py` | talk to a RetroCore SINTRAN terminal |
| `tools/refcap.py`, `refkeys.py` | capture reference sessions |
| `tools/emucmp.py`, `mktests.py` | compare a script with a capture; make tests |
| `tools/fuzzgen.py` | random command scripts from the vocabulary |
