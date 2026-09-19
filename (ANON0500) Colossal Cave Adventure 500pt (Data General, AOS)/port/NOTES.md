# Notes — running an original AOS program

`ADVENTURE.PR` is the first program in this collection built for **AOS**, the
16-bit Eclipse system that came before AOS/VS.  The emulator in `src/` started
as a copy of the AOS/VS 16-bit one used by Thissala and the 350-point AOS/VS
Adventure; everything below is what had to change or be added.  Addresses
are hex, and names come from the linker symbol table (`notes/ADVENTURE.sym`,
read with `tools/st16.py`).

## 1. The program file is the address space

An AOS `.PR` (file type ?FPRG, 65) has none of the AOS/VS layout — no header
blocks, no separately placed shared half.  It is exactly 0x8000 words and
**file word N is memory word N**: page zero is already live (M[15] = 7C81,
the .SYSTM thunk), the overlay area is present but zero, and the UST at 0x100
is the older, contiguous table:

| UST | field | value |
|---|---|---|
| +0B | tasks | 1 |
| +0C | address of the .SYSTM context block | 0113 |
| +0F | impure blocks | 3 |
| +10 | overlay directory | 0128 |
| +11 | shared start block | 16 |
| +12 | shared blocks | 16 |
| +1D | start address | 02C6 = `.F5INIT` |

Each field checks against something else: 3 impure blocks end where the zeros
start at 0B6D; 16+16 blocks close exactly on 0x8000; the directory names 13
overlays of 1024 words at 6800, which is the `.OL`'s 26624 bytes and the
address the symbol table gives every overlaid routine.  `load_pr` recognises
the format by its length and the missing AOS/VS program-type bit (UST+14).

Two slots moved: the overlay directory (UST+10, not +0D) and the impure block
count (UST+0F, not +0C).  `?MEMI` still wrote the AOS/VS slot, UST+0C — which
in AOS is the context block pointer — and every later system call saved its
registers at word 4.

## 2. How a system call reaches the system

`JSR @17` enters the runtime's thunk at 7C81, which files the caller's state in
the context block (address in UST+0C) and traps with **SVC (8748)**, AC0 =
the block:

    +1  bit 15 set while in a system call     +6/+7/+8  AC0 AC1 AC2
    +2  SP   +3 FP   +4 SL                    +9  FP
    +10 the success return (error = one less) +15 call number
    +19 the original call word, for agent calls

A call word with bit 15 set is an **agent** call.  The thunk (7C99 `MOVL#
1,1,SNC`) sends it as call 50 with the word at +19, after setting UST+7 bit
0x4000 busy; nothing in the program clears that bit or block+1 bit 15, so the
system must.  Left set, the runtime's scheduler `SCHED` (7C48) sees a pending
reschedule and traps with SVC 3.

### The AOS agent's call numbers

AOS/VS kept the agent calls (SYSID.16.SR: 0300 "AGENT OPEN", 0301 "AGENT
CLOSE"…) but not their AOS order, so each number is pinned by evidence rather
than assumed:

| AOS | call | AOS/VS | evidence |
|---|---|---|---|
| 8000–8003 | ?OPEN ?CLOSE ?READ ?WRITE | 0300–0303 | the packets: `.F5INIT` opens the console, `RDSEQ` reads, `WRSEQ` writes |
| 8009 | ?GTMES | 0307 | **RUNAWAY**, an AOS assembly utility on the NADGUG tape (BJ_BB/AOS/PERFORM) that survives as source and program: its source issues ?GTMES ?OPEN ?WRITE … ?RETURN and its `.PR` has 8009 8000 8003 … 800A |
| 800A | ?RETURN | 0310 | the same; also `?BOMB`, `SFALT` |
| 800B | ?ERMSG | 0311 | follows ?RETURN; `F.CLOSE`'s error path |
| 8016 | ?SPOS | 0322 | the routine at 752C files record − 1 in ?IRNH/?IRNL and calls 8016 with the packet |
| 8017 | overlay open | ?OVOPN 0323 | `?LODO` asks for the channel its overlays come from |

So AOS had two more calls than AOS/VS before ?GTMES and four more before
?SPOS.  Anything else stops the emulator and names itself.  (`?LODO` also
holds an 813D, used instead of ?SPAGE when UST+7 bit 0x1000 is set; it never
is here.)

## 3. PSHJ

`85B8 0079` is **PSHJ** (EBID.SR 102270): push the return address and jump,
index mode in bits 9-8 like the other extended memory references.  Its low byte
is B8, not 38, so it never reached that decoder, and it had been mistaken for
XOP1.  The AOS FORTRAN 5 runtime calls its own internals with it (`R?CAL` →
`?RSRE`, `?RSLO`); neither AOS/VS program had used it.

## 4. Overlays and shared pages

`?LODO` gets the overlay channel from agent call 8017, then loads an overlay
with ?SPAGE (48) or ?RDB (7): channel in AC1, the 16-bit block packet in AC2
(block count in ?PSTI's right byte, address in ?PCAD, block number in
?PRNH/?PRNL, 512-byte blocks).  Overlay n is blocks 4n..4n+3 at 6800.

The game opens `ADVENTURE.DATA` with **?SOPEN** (51; AC0 the name, AC1 the
channel wanted, channel back in AC1) and maps its blocks 8..31 at 4800 with
?SPAGE — the travel table, text pointers and vocabulary COMMONs (`TRVCOM`,
`TXTCOM`, `LTXCOM`, `VOCCOM`).  Those mappings are remembered and compared
with the file at ?FLUSH, ?SCLOSE and exit.  A normal game never changes them,
so the original is opened read-only; only a changed page would make a copy in
the save directory.

## 5. The bug that emptied the unit numbers

The first runs died with a FORTRAN traceback:

    **ERROR** REPORTED BY (112)
      CALLED AT (106)+64
      CALLED AT (54)+11
      CALLED AT 0+64402

The numbers are octal page-zero entries: `.FOP` (0x4A) failing under `.IFILE`
(0x46) under `.IFWR` (0x2C), from `SPEAK` in overlay 0.  `SPEAK` writes to the
unit held in `CHNCOM`, which was 0, so the runtime opened the default file
`00.F5`.  `CHNCOM` had been read from `ADVENTURE.DATA` — as zeros, because
every one of the twelve start-up reads returned the file's first record.

`RDSEQ` reads with ?ISTI = 5001 (?ICRF, ?IBIN, dynamic) and ?IRNH/?IRNL = 0.
PARU: **?IPST (1B2 of ?ISTI) set means the record number is absolute; clear,
it counts on from where the channel stands** — 0 is the next record.  The
inherited code looked for that flag in ?IRES (packet word 4), always found it
clear, and so treated every record number as absolute.  Thissala's reads
happened to work that way; sequential FORTRAN reads cannot.  `rec_seek` now
reads the flag where PARU puts it, for ?READ and ?WRITE alike.

## 6. The rest of the system interface

- **?GTMES** answers from the emulator's command line: `adventure MYGAME`
  gives ?GCNT = 1 and ?GARG 1 = `MYGAME`, in capitals as the CLI passed them.
  The game asks for the count at start-up and restores from the named file.
- **?OPEN** now follows PARU's ?OFCR/?OFCE: SAVE opens its file for input and
  output, and on "does not exist" (ERFDE, 025) opens it again with ?OFCE,
  which creates it.  Output goes only to the save directory.
- **?DELETE** — the game deletes a suspended game when it resumes it.
- **Console input** is folded to capitals (a console without `/ULC`); the game
  answers a lower-case "no" with "Please answer the question."
- **Console output**: a formatted record is NEW LINE, text, RETURN.  The RETURN
  is held back and dropped before a NEW LINE, so transcripts are plain text;
  NULs (the resume message pads the file name with them) are not shown.
- `-Z` freezes ?GTOD/?GDAY at 1981-06-15 12:00 for the tests.

## 7. The other Adventures on the AOS tapes

- *Games 1* `ADVENTURE/` is the 350-point AOS/VS program already ported in
  `Colossal Cave Adventure 350pt (Data General, AOS-VS)` (same `.PR`).
- *Games 1* `AOS.ADVENTURE/` is a 1977 **AOS** build of the 350-point game
  (bound 9/10/77 per its `ADVENTURE.MAP`), with the source database
  `ADVENTURE.DB` and a `SAVEADVENTURE`.  It also runs on this emulator: it
  builds its tables ("Table space used: 584 of 1000 Message Pointers…") and
  then enforces Woods' cave hours — weekdays 8:00–18:00 are wizards only.
  Not ported.
- *Games 2* `GAMES/BASIC/ADVENTURE.CI` is not an adventure: it is a DG BASIC
  Star Trek under that name.  The BASIC menu `ADV_MENU.LS` runs this game.

## 8. Emulator updated with Dungeons (2026-09-16)

`src/` is now the same emulator as the Dungeons port
(`../../Dungeons (Data General, AOS)/port`), whose `NOTES.md` has the
details: Eclipse stack faults, the C-series commercial instructions
(`src/cis.h`), `ELDB`/`ESTB`/`DSPA`, the character instructions, the double
shifts, real `FPSH`/`FPOP`, one-key console input, and two corrections that
matter here too — **?GTMES ?GARG answers the length in AC0 and the value in
AC1** (this game only reads the NUL-ended name, so it worked either way), and
**?RETURN takes its message pointer in AC1 and flags + length in AC2**.  All
four recorded transcripts in `tests/` are unchanged.
