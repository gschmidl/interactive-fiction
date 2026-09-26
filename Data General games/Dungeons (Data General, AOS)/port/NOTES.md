# Notes — what Dungeons needed

`src/` began as a copy of the emulator from the 500-point Adventure port
(`../../../Colossal Cave variants/DGAO0500 Colossal Cave Adventure 500pt (Data General, AOS)/port`), which
already ran an original AOS program: the `.PR` as the 32K address space, the
SVC system call path, the AOS agent call numbers, overlays through `?LODO`.
`NOTES.md` there explains those.  Everything below is new with `DG.PR`.
Addresses are hex; there is no symbol table for this program.

## Layout

`DG.PR`: 9 impure blocks, shared area from block 12 for 20 blocks, overlay
directory at 0128, start address 3812 (`.MAIN` itself — see below).
`DG.OL` holds 4 overlays of 2048 words, all running at 3000; overlay *n* is
blocks 8*n*..8*n*+7.  After the directory header at 0129 the resource table is
(overlay, offset) pairs from 0131, and the word after `JSR @12` at a call
site is the address of the pair — or, when it is 0400 or more, a root
routine's own address.

## 1. Stack faults: the runtime starts on the first one

The image's page zero has SP = FP = 01A0, stack limit (042) 01A1 and the
stack fault vector (043) 7993.  `.MAIN` opens with `SAVE 1383` — which cannot
fit — and 7993 is not an error handler but the FORTRAN runtime's memory
initialiser: `?MEM`, `?GSHPT`, `?SSHPT`, `?MEM`, `?MEMI` for every free page,
then a new stack from 2058 to the top of memory less 14, a real fault handler
in 043, and back to the SAVE.

The emulator had no stack faults, so the first SAVE simply ran at 01A0 and
the stack climbed through the overlays' constant pool at 08F0.  The first
symptom was a system call numbered 013B with ASCII in the registers: the
routine at 763A issues any call by building `JSR @17 / call / ISZ @32 /
POPJ` on the stack, and its call number constant (8009, ?GTMES) at 08F2 had
already been overwritten by stack.

Implemented as the ECLIPSE programmer's references describe (015-000024
3-18..3-22, 014-000642 2-14/2-15): SAVE and MSP check before executing and
are not executed, saving their own address; PSH, PSHR, PSHJ and FPSH check
after, saving the next PC.  Overflow is SP > limit, unsigned.  The fault
clears SP's bit 0, sets the limit's bit 0, pushes AC0–AC3 and carry+PC, and
jumps through 043.  (SIMH's `eclipse_cpu.c` has five deviations from this,
so it was not used as the reference.)  Pops check for underflow as
documented.

`?GSHPT` (59) and `?SSHPT` (36) report and move the shared partition, as
first page and page count.

## 2. The Commercial Instruction Set

Why the write-up's SIMH needed a C/150: the runtime at 78E7..7910 is a row of
one-instruction stubs — `CMP`, `CMV`, then 8158/8958/9158/9958, 87A8..9FA8 and
A7A8..BFA8 — each followed by `POPJ`.  EBID.SR (AOS/VS `:UTIL`) names only the
character ones: `CMV 153650`, `CMP 157650`, `CTR 163650`, `CMT 167650`.  The
rest are the C-series commercial instructions, documented in the 1975
*Programmer's Reference Manual, ECLIPSE Line Computers* (015-000024-04,
3-54..3-58), with the FPAC in bits 3-4:

| code | | |
|---|---|---|
| 103650 | `LDI fpac` | decimal string → FPAC |
| 123650 | `STI fpac` | FPAC → decimal string |
| 100530 | `FINT fpac` | FPAC = its integer part (the MV's FINT is a different code) |
| 177650 | `LSN` | sign code of a decimal string |

AC1 is the attribute specifier: data type in bits 8-10, size in 11-15; AC3 a
byte pointer to the field.  SIMH decodes `LDI`/`STI` but leaves them
unimplemented.  The 32-bit *Principles of Operation* (014-000704) still has
the data-type table and the sign-and-digit characters (`{ A..I`, `} J..R`).
`src/cis.h` implements all eight data types; the program only ever uses
type 5, packed decimal, 8 digits.

Its random number generator is a linear congruential one done in floating
point through those conversions — X' = (X·31415933 + 14181771) mod 2²⁶ — and a
traced run matches that exactly for every step checked, which exercises
`LDI`, `STI`, `FINT` and the FPU together.

`FPSH`/`FPOP` were no-ops in the Adventure emulator.  They now push and pop
the 18-word block (FPSR, then FAC0–FAC3): a routine may change an FPAC
between them.

## 3. More instructions

- `ELDB`/`ESTB`/`DSPA` (102170/122170/142170): the 38-family layout with 78 in
  the low byte.  For the byte pair the displacement is a byte pointer and the
  index value (0, the displacement's address, AC2, AC3) is added to its
  *word* address; `DSPA` dispatches through a table with its limits in the two
  words before it.
- `CMV CMP CTR CMT`: the character instructions, signed lengths in AC0/AC1
  (negative = backwards), byte pointers in AC2/AC3; short strings padded with
  spaces.
- `DLSH`, `DHXL`, `DHXR`: 32-bit shifts of ACD:ACD+1 (AC3+1 is AC0).

## 4. System interface

- **?GTMES ?GARG returns the length in AC0 and the value in AC1.**  The
  emulator had them the other way round, and the 500-point Adventure (which
  only reads the NUL-ended string) never noticed.  DG.PR's argument routine
  (overlay 1, 3413..343C) passes AC0 to its string copy as the length, so
  "DG 3" copied three bytes — "3", NUL, junk — the parse failed, and every
  game was a level 1 dungeon.
- **Agent call 8144** is issued by the runtime's error path (76E0) with
  ?ERMSG's registers: code in AC0, buffer size and message-file channel in
  AC1, buffer pointer in AC2, length back in AC0.  The error file is not
  here, so the text is `ERROR <octal code>`.
- **?RETURN**: AC1 points at the message, AC2 holds the flags (?RFCF,
  severity, ?RFEC) and the length.  Started without a level — which the CLI
  macro never did — the program stops with " (From ERROR 1712+12)", shown on
  stderr.
- **Console**: the game opens `@CONSOLE` for binary input (?IBIN) and reads
  one byte at a time; at a real console that is `_getch` (no Enter, no echo),
  from a pipe a byte at a time.  A form feed (the first thing it writes)
  clears the screen.

## 5. DG.CLI

    [!eq,,%1%]  ... st [!r Which level do you wish to attempt? ] ...
    [!el]  st %1%  [!enD]
    x dg.pr [!st]

`dungeons.bat` does the same.  The two bytes 0254 in DG.CLI are commas with
the top bit set, so the CLI would not take them as argument separators.

## Verification

- `sh tests/run.sh`: frozen-clock replays of an introductory session (help,
  status, look, the unreadable map, a steel door, fool's gold, death), the
  welcome at levels 2-4, four driven games at level 3, and the no-argument
  exit.
- Random-key sessions — 52 games over all four levels — ran with nothing on
  stderr.
