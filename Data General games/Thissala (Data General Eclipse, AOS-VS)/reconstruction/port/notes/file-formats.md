# THISSALA — file formats and machine notes

Source files exported from the live AOS/VS system (MV/8000 emulator) on 2026-09-07.
`src_original/` is that export, untouched.

**Read this as a log, not as a description of the finished emulator.** It is
kept in the order things were worked out, so an early section can be overturned
by a later one — that is what the `Correction:` headings are for. The two
sections that record something as unsolved both carry a **Resolved** note
pointing at where the answer ended up. For what the emulator does today, read
`../README.md` and `../NOTES.md`.

## PLOT.PR — the program

A **flat memory image**, no header:

    word address = 0x400 + file_offset/2
    byte address = file_offset + 0x800          (byte 0 is the HIGH half of a word)

63488 bytes = 31744 words = words `0x0400`..`0x7FFF`, i.e. the image ends exactly
at the top of the 32K-word address space. Page zero (words 0..0x3FF) is **not** in
the file and must be established at load or by startup code — the program calls
through it constantly (`JSR @12`, `JSR @92`, `JSR @93`, `LDA 2,32`).

Verified against the string descriptors: the literal `Game saved\n` has its record
at file `0x77FC` with byte pointer `0x8002`, and its character data at file `0x7802`
— `0x7802 + 0x800 = 0x8002`. Confirmed again with `CMD2` at file `0x8244`.

**This offset is easy to get wrong by three words.** An earlier pass used
`- 0x806` (treating the descriptor's pointer as the address of the record rather
than of its data) and every constant resolved three words low, which made the
postbox routine look like it referenced nothing.

### String descriptors

    word 0:  byte pointer to the character data (== (own_word_addr + 3) * 2)
    word 1:  length in bytes
    word 2:  length again
    word 3+: the characters, high byte first, padded to a word

The disassembler recognises this shape and prints the string inline.

### Contents

- ~`0x0400`-`0x0500`  zero
- ~`0x0500`-`0x0600`  a table of `FFFF` guards and `(7000, addr)` pairs — unidentified
- ~`0x0600`-`0x2300`  zero (overlay area and/or BSS)
- ~`0x2300`-`0x2800`  data
- ~`0x2800`-`0x4000`  zero
- ~`0x4000`-`0x8000`  the literal/constant pool: per-module constant pools and all
  string literals, interleaved. Module name literals (`CMD2`…`CMD12`, `SOVLY1`…`SOVLY6`)
  mark the pool boundaries; a module's constants follow its name literal.

There is **no executable code in PLOT.PR** except a region around `0x7C92`+ that
looks like startup/runtime glue.

## PLOT.OL — the overlays

All the game code. Load base not yet determined; disassemble with base 0 and read
addresses as file-relative until it is. Absolute operands in overlay code point
into PLOT.PR's data, so disassemble with `-p PLOT.PR 400`.

## The instruction set

16-bit Eclipse, **not** MV/8000 32-bit — every instruction decoded so far is NOVA
base plus the Eclipse extensions. Masks and matches are in `src/eclipse.h`,
transcribed from simh `NOVA/eclipse_cpu.c`.

Eclipse extensions live in the NOVA "no-load, no-skip" ALC hole
(`bit15 set && (ir & 0x08) && !(ir & 0x07)`), which is a no-op on a NOVA.

Two-word forms all have low byte `0x38`; AC in bits 12-11, index mode in bits 9-8
(0 = absolute, 1 = PC-relative, 2 = AC2, 3 = AC3):

| mnemonic | mask | match |
|---|---|---|
| ELDA / ESTA / ELEF | `0xE4FF` | `0xA438` / `0xC438` / `0xE438` |
| ELDB / ESTB | `0xE4FF` | `0x8478` / `0xA478` |
| EJMP / EJSR / EISZ / EDSZ | `0xFCFF` | `0x8438` / `0x8C38` / `0x9438` / `0x9C38` |

`ELEF ac,abs,n` does double duty: with `n` a plain number it is the load-immediate
idiom; with `n` a pool address it loads that constant's *address* for a
call-by-reference. Context decides.

## The compiler's calling convention

Compiled from an unidentified HLL. The recurring shape is:

    LDA 2,32          ; AC2 = stack pointer from page-zero 32
    ELEF 0,&arg       ; address of an argument
    PSH 0,0           ; push it
    JSR @12           ; call through a page-zero vector
    ...
    STA 2,32          ; restore the stack pointer

Range tests use `SGT` plus the `SUB ac,ac,SKP` / `SUBZL ac,ac` pair to
materialise a 0/1 boolean.

Linear disassembly still desyncs in places — some runtime calls appear to be
followed by inline argument words. Resolving this is the next task, and is the
same work as inventorying the AOS system calls for the shim.

## Worked example — the post office box (module CMD3, the PUSH verb)

At PLOT.OL word `0x3975` (file `0x72EA`):

    ELEF 2,0A96      AC2 = 2710            token for "0"
    LDA 0,10,3       AC0 = the typed noun
    SGT 2,0          \ AC2 = (noun < 2710)
    SUB 2,2,SKP      |
    SUBZL 2,2        /
    ELEF 1,0AA0      AC1 = 2720            token for "r"
    STA 2,47,3       stash the low-side flag
    SGT 0,1          \ AC2 = (noun > 2720)
    SUB 2,2,SKP      |
    SUBZL 2,2        /
    LDA 0,47,3
    IOR 0,2          out of range?
    MOV# 2,2,SNR
    JMP -> 398E      in range
    ELEF 0,4563      -> message 122  "You cannot push it."

and in range, at `0x398E`:

    ELEF 0,4564      -> message 121  "...second click, and the post office box swings open"
    LDA 0,10,3
    ELEF 1,0AA0      2720 = "r"
    SUB# 0,1,SZR     is it the Reset button?
    SUB 0,0
    ESTA 0,01E9      [01E9] := 0     the press counter — reset
    ...
    ELEF 1,4565      -> "101654"     the combination
    ...
    ELEF 0,4570      -> message 123  "Click."

`[0x01E9]` is referenced from exactly six places, all inside this one routine:
it is the count of correct presses. The combination **101654** is also spelled by
three torn paper scraps in game (messages 115 `10`, 114 `16`, 139 `54` — their
ragged edges tessellate in that order).

## Page zero — reconstructed without a dump

Page zero (words 0..0x3FF) is not in PLOT.PR, and the file is byte-complete
(AOS/VS reports exactly 63488 bytes), so nothing was lost in the export — the
program simply doesn't carry it. It can be rebuilt from the image instead.

**The runtime vector table is at word `0x242E`**: 69 strictly increasing code
addresses, `6E9C` through `7C8D`. They install into **page zero words 44..112**.
The upper bound is the confirmation — 112 is exactly the highest `JSR @n` vector
the program uses anywhere, and 47..112 (every vector in the contiguous block) is
covered.

Cross-checked by disassembling two of the targets: page-zero 92 -> `0x7762` and
page-zero 93 -> `0x7788` both begin `SAVE <frame>` / `MOV 0,2`, the same routine
prologue. They are entry points.

Vectors outside the table:

| vector | status |
|---|---|
| 41 | not a vector — a scratch global. Runtime routines do `STA 3,41` to stash the return address. |
| 15 | `.SYSTM`, the system-call trap. Supplied by AOS/VS. |
| 12 | the HLL procedure call (991 sites in the overlays, 30 in PLOT.PR). No code anywhere writes it, so also loader/OS supplied. |
| 7 | one site; unresolved. |

So the shim must synthesise page zero: copy `0x242E`..`0x2472` to words 44..112,
and provide handlers for 7, 12 and 15.

## SAVE — the cause of the disassembly desync

`0xE7C8` (0163710) is **`SAVE`**, and it is a **two-word** instruction: the opcode
followed by the frame size. Every runtime routine begins with it. Reading it as one
word shifts everything after it by a word, which is what made linear disassembly
fall apart and made runtime calls look like they carried inline arguments.

The `0x?7F8` family (`87F8`, `C7F8`, `E7F8`) occupies the same corner of the
encoding and shows up at routine exits — presumed return/pop variants, not yet
confirmed.

## Page zero — solved

PLOT.PR carries its own page-zero image at word **`0x2402`**: page-zero word N
lives at `0x2402 + N`. Confirmed three independent ways:

- the runtime vector table falls at page-zero 44..112, and 112 is exactly the
  highest `JSR @n` the program uses;
- `0o40` (SP) = `0x0421`, a plausible initial stack pointer in the empty low area;
- `0o43` (stack-fault address) = `0x7CB4`, which is a code address.

`@12` and `@15` are **zero** in that image — consistent with them being supplied by
AOS/VS rather than the program, as predicted.

`src/cpu.c` builds page zero from this and reports it at startup:

    page zero built from 2402: SP=0421 FP=7CAB SL=0000 SFA=7CB4  @12=0000 @15=0000

## Overlays are position independent

Every branch inside PLOT.OL is PC-relative — there is **not one** absolute `EJMP`
or `EJSR` anywhere in the 94KB. Overlay code will therefore run correctly at any
load address, which removes a whole class of relocation work from the shim; but it
also means the load address cannot be recovered from the code, so the overlay
directory has to be found.

PLOT.OL word 0 is `SAVE 12` / `ELDA 2,01B4,PC` — a routine prologue. The file
starts straight into code, so the directory is not at its head; it is probably the
small initialised table at PLOT.PR words `0x506`..`0x529`, which is the only
non-zero content in the otherwise empty `0x0400`..`0x2402` region (that region
being, by size and emptiness, the obvious overlay load area).

## Where startup lives

Not in the root. The banner, `Initializing `, `What is your name human ? ` and the
`THISSALA.DB5` filename are all referenced from PLOT.OL words `0x90B7`..`0x92C3`.
So the root's job is to initialise, load that overlay and enter it — and the root
entry point is still unidentified.

## Correction: the page-zero image base is 0x2400, not 0x2402

The earlier base was derived circularly (chosen so the vector table would land
on 44). The independent constraint is only that the table's 69 entries must
cover every vector the program uses, 47..112 — which allows a base of 44..47.

`0x2400` is the right one, and the decisive evidence is that it fills in the two
vectors that looked OS-supplied under `0x2402`:

| | base 0x2402 | base 0x2400 |
|---|---|---|
| `@12` procedure call | 0000 | **7D15** |
| `@15` .SYSTM | 0000 | **7CBC** |
| SP / FP / SL / SFA | 0421 / 7CAB / 0000 / 7CB4 | **0404 / 0000 / 0421 / 7CAB** |

Under `0x2400` the stack words are coherent (FP zero at startup, SFA pointing at
an error stub) and both call vectors resolve to real code. The vector table then
occupies page-zero 46..114.

## The OS interface is five stubs, not a trap instruction

`0x7CBC` (page-zero 15) is a **thunk**, not the OS:

    7CBC  PSH 3,3       push the caller's return address
    7CBD  A6C9          <- entry stub
    7CBE  3000
    7CBF  0000          selector
    7CC0  0000

`A6C9` occurs exactly **five times in the whole program**, all in PLOT.PR's
runtime glue, always as the four-word block `A6C9 / 3000 / selector / 0000`
with selectors 0..4, at `7C9D`(2) `7CA1`(1) `7CA7`(3) `7CBD`(0) `7CC9`(4).
These are the five AOS entry points; everything the program asks of the
operating system goes through them. Selector 0 is `.SYSTM`.

Because the caller reached the thunk with `JSR @15`, AC3 addresses the call-code
word. Empirically the normal return is **AC3+3**: with AC3+2 the startup path
lands in the middle of a two-word error branch and dies, while AC3+3 flows
correctly on to the next stub. (The convention at other call sites, where the
error branch is a single word, still needs checking.)

## Startup found

`0x7C94` is the entry point, or close enough to it. Execution from there runs:

    7C94 -> JSR @15 -> thunk -> stub 0 -> .SYSTM 324 (0504) -> stub 1 (7CA1)

which is the process initialisation sequence. The emulator now gets this far and
stops at stub selector 1, which is the next thing to implement.

## Correction: 0x7C94 is not the entry point

It is the runtime's **fatal-error and termination machinery**. The four words at
`7C94` are handler entry points; `7CAB`/`7CAE`/`7CB1`/`7CB4` are error stubs that
load codes 18, 308, 309, 310 and fall into a common reporter at `7CB6` which
prints `FATAL RUNTIME ERROR: ` / `TERMINATING` (the plain text sits at `7CDE`+)
and exits via `.SYSTM 200 (0310)`. Page-zero 35 (`0o43`, the stack-fault vector)
points at `7CAB`, which is why that path is reachable at all.

The real HLL startup is the runtime's stack initialisation around `27D0`-`27E3`
(`STA 1,33` zeroes FP, `STA 3,34` sets the stack limit), but the address AOS/VS
actually enters at is still unknown — and since nothing in the image copies the
page-zero image into page zero either, both are almost certainly held in the
program file's AOS/VS metadata rather than in its 63488 bytes.

**This does not block the port**, because overlay code is position independent:
an overlay can be loaded anywhere and entered at its head, bypassing the root's
entry entirely.

## The instruction set, completed and sourced

Fixed full-word opcodes (all confirmed against simh `eclipse_cpu.c`):

| opcode | octal | instruction |
|---|---|---|
| `AFC8` | 0127710 | RTN |
| `9FC8` | 0117710 | POPJ |
| `8FC8` | 0107710 | POPB |
| `87C8` | 0103710 | PSHR |
| `E7C8` | 0163710 | SAVE (two words) |
| `EFC8` | 0167710 | RSTR |
| `B7C8` / `97C8` / `BFC8` | 0133710 / 0113710 / 0137710 | BLM / BAM / DIVX |

Two-word immediate family, mask `0xE7FF`, bits 12-11 select the destination
accumulator and the second word is a full 16-bit immediate — 360 sites:

| first word | octal | instruction |
|---|---|---|
| `87F8` | 0103770 | IORI |
| `A7F8` | 0123770 | XORI |
| `C7F8` | 0143770 | ANDI |
| `E7F8` | 0163770 | ADDI |

**Frame layout confirmed.** `SAVE` pushes AC0, AC1, AC2, FP, then carry|AC3,
sets FP to the new stack top and leaves AC3 = FP. That puts the saved AC0 at
FP-4 — exactly how the runtime addresses it (`LDA 0,-4,3`), which is independent
confirmation the stack model is right.

## Overlays are called through page-zero 12

`JSR @12` -> `7D15` -> `.SYSTM 186 (0272)`. The generic procedure-call vector
issues a system call, which is why it accounts for 991 of the call sites in the
overlays: **every inter-module call goes through the overlay loader.** So
`.SYSTM 186` is the overlay load, and implementing it is the next milestone.

Overlays appear to be fixed 2048-word blocks (`SAVE` prologues sit at every
`0x800`-word boundary through file word `0x9000`), so overlay N should be file
word N*0x800 — derivable without the directory. The startup overlay, which
references the banner, `Initializing `, `What is your name human ? ` and the
database filenames, is the one at file word `0x9000`.

## JSR @12 carries one inline argument — the callee id

The overlay procedure-call vector at `7D15` reads the word at the caller's
return address (`LDA 2,0,3` / `LDA 2,0,2`), passes it to `.SYSTM 186`, and then
computes the return as `(saved AC3 & 0x7FFF) - AC0 + 1` so that control resumes
**past** the inline word.

So every `JSR @12` is a two-word construct: the call, then a 16-bit callee id.
Teaching the disassembler this resynchronises it across the whole program — it
was the last cause of drift. `src/dis.c` now decodes it.

There are **58 distinct callee ids** in the overlays. The most used:

| id | calls | identified as |
|---|---|---|
| `5D26` | 177 | print a message (called with the address of a message number) |
| `5DBE` | 93 | |
| `6024` | 63 | |
| `5674` | 56 | |
| `5FFA` | 45 | |
| `540B` | 35 | |

`5D26` is confirmed: in the PUSH handler it is called twice in a row, first with
the address of message 122 and then with the address of message 121.

## Artifacts

- `notes/plot-ol.dis` — full disassembly of the game code, 29,978 lines
- `notes/plot-pr.dis` — the runtime and literal pool, 21,862 lines
- `notes/decoded/` — rooms, objects, messages, vocabulary in plain text
- `dis.exe` — the disassembler; `thissala.exe` — the CPU

## Status of the emulator

Working: the full 16-bit Eclipse instruction set as used by this program, the
Eclipse stack and frame model (validated against the runtime's own `FP-4`
addressing), page-zero reconstruction, overlay loading at an arbitrary base, and
the five AOS entry stubs. It executes real overlay code and reaches the overlay
loader.

Blocked on: the semantics of the 32 `.SYSTM` call codes. No AOS/VS system-call
reference could be found online, and the codes cannot be inferred safely — that
is exactly the kind of guessing that would make the port quietly unfaithful.
What would unblock it: the AOS or AOS/VS Programmer's Reference call-code table
(bitsavers is the likely source), or the program file's AOS/VS metadata, which
would also supply the entry point and the overlay directory.

## System calls — resolved from :UTIL:SYSID.16.SR

`SYSID.16.SR` defines `?SYST = JSR @17` (octal 17 = decimal 15, matching the
`JSR @15` in the program) and declares each call with `?SCL1`/`?SCL2 <name>
<octal code>`. 260 calls defined; 25 of the 30 this program uses:

| code | call | | code | call |
|---|---|---|---|---|
| 0300 | ?OPEN | | 0310 | ?RETURN |
| 0301 | ?CLOSE | | 0311 | ?ERMSG |
| 0302 | ?READ | | 0312 | ?GCHR |
| 0303 | ?WRITE | | 0313 | ?SCHR |
| 0003 | ?MEM | | 0316 | ?SEND |
| 0014 | ?MEMI | | 0326 | ?PROC |
| 0015 | ?DELAY | | 0504 | ?KILL |
| 0036 | ?GTOD | | 0521 | ?MYTID |
| 0041 | ?GDAY | | 0542 | ?IFPU |
| 0143 | ?BRKFL | | 0543 | ?GCRB |

`?RETURN` and `?KILL` land exactly where expected — the fatal-error path and the
termination dispatch. Everything above is implemented in `src/cpu.c`, with the
general user I/O packet from PARU (`?ICH` 0, `?IBAD` 3, `?IRCL` 5, `?IRLR` 6,
`?IRNH`/`?IRNL` 7-8, `?IFNP` 9, `?IMRS` 10) and the `?SCL2` convention of
passing the packet address in AC2.

The five that remain — **0270 through 0274** — SYSID marks explicitly as
`RESERVED BY AGENT`, so they are the AOS/VS agent's private calls and are not
documented anywhere in PARU or SYSID. `0272` is the one the call trampoline at
`7D15` uses on every inter-module call.

## The UST, found via PARU

PARU documents the User Status Table, which let me locate it by signature at
word `0x500` — the table noticed in the first hour and never identified:

    USTDA=FFFF(-1)  USTTC=0001(1 task)  USTBL=0002  USTOD=0127
    USTST=000C      USTIT=FFFF(-1)      USTSZ=0014  USTPR=8000 (UST16)

`USTPR = 1B0` confirms a 16-bit program. This also reconciles the load base:
PARU puts the UST at `0400` octal, and `0x400 + 0x100 = 0x500`, so the origin
really is word `0x400` as the string descriptors indicated. **`USTOD` is the
overlay directory address** — 0x127 from the origin.

## The overlay mechanism is AOS/VS "resource calls"

This is the structure behind `JSR @12`, and it is fully documented across the
:UTIL files.

SYSID.16.SR defines three resource calls: **`?KCALL` (0), `?RCALL` (1),
`?RCHAIN` (2)**. URT16.LB contains their implementations — its symbol table holds
`??KCA?`, `??RCA`, `??RCH?`, `K?CALR`, `R?CAL`, `R?CHAK`. The Link editor
resolves resource calls at link time and can convert them to direct `EJSR`
(`/NRC`, `/NRP`); in this program they were left as runtime-resolved calls, which
is why they all funnel through the trampoline at `7D15`.

PARU gives the stack layout, and it matches the trampoline instruction for
instruction:

| offset from FP | PARU name | meaning |
|---|---|---|
| -4 | `?OAC0` | caller's AC0 |
| -3 | `?OAC1` | caller's AC1 |
| -2 | `?OAC2` | caller's AC2 |
| -1 | `?OFP` | caller's frame pointer |
| 0 | `?ORTN` | carry + return address |
| +1 | `?DESC` | caller's resource description |
| +2 | `?VRTN` | caller's virtual return |

`?RSAVE` is `SAVE 2+n` — routines making resource calls allocate two extra frame
words for `?DESC` and `?VRTN`. The trampoline's `STA 2,1,3` and `STA 2,2,3` are
writing exactly those two fields into the caller's frame.

`?DESC` encodes what the target is:

    0            no resource
    >= ?USTART   in the root  (?USTART is Link-defined; here the origin, 0x400)
    bit 15 set   shared library
    bit 15 clear overlay

So `.SYSTM 0272` is the agent resolving a resource id into a `?DESC`. For targets
in the root it is the identity — which is consistent with every inline argument
in this program pointing at a `SAVE` prologue in PLOT.PR. Overlay targets would
come back with bit 15 clear and require a load.

## URT16.LB confirms the library

Extracted from the :UTIL dump (3191 bytes). Its symbols name the fault handlers
this program uses — `.BOMB`/`?BOMB`, `.KILL`, `.UKIL`, `.UTSK`, `WALKB`/`.WALK`
(walkback), `UNWIN`/`.UNWI` (unwind), `IXIT`, `GATED` — and it contains the
literal strings `FATAL RUNTIME ERROR`, `TERMINATION`, which are exactly the text
at PLOT.PR `0x7CDE`. That identifies the four error stubs found earlier:

| address | stub | error code |
|---|---|---|
| `7CAB` | one of `.BOMB`/`.KILL`/`.UKIL`/`.UTSK` | 18 |
| `7CAE` | " | 308 |
| `7CB1` | " | 309 |
| `7CB4` | " | 310 |

## Remaining defect

The emulator implements all 25 identified system calls and returns identity for
`0272`, which the documentation says is correct for root targets. Execution
still diverges inside the trampoline after the call — the descriptor does not
reach the dispatch at `7D3C` (`POP 3,3` / `JSR 0,3`). That is now an
implementation bug in the stack model, most likely `SAVE`/`RTN` detail, rather
than a gap in understanding. Everything needed to fix it is in this document.

> **Resolved.** The guess held: it was the stack model, not the understanding.
> See *"The resource call returns its answer in AC1"* and *"The resource return
> has to restore the caller's overlay"* below for the two pieces, and
> *"System calls implemented"* for where it finished. The emulator now cold
> starts from `PLOT.PR` and plays the game through.

## System call return convention — two families

Documented calls: **error returns at AC3+1, normal at AC3+2** (AC3 addresses the
call-code word). `?MEMI` at `0x2731` shows it plainly — `0x2733` is an indirect
jump to an error handler, `0x2734` the normal continuation.

The agent-reserved family **0270-0274 resumes at AC3+1** instead. The resource
trampoline at `7D15` proves it: its `LDA 3,33` at `7D23` must execute, or AC3
still holds the syscall return address and the frame walk reads garbage.

## The resource call returns its answer in AC1

`.SYSTM 0272` takes the resource id in AC2 and returns:

- **AC2** = `?DESC`, stored by the trampoline into the caller's frame at FP+1;
- **AC1** = the entry address to jump to. The trampoline writes AC1 into
  `M[FP-5]` at `7D33`, and the dispatch at `7D3C` (`POP 3,3` / `JSR 0,3`) pops
  exactly that word.

For a target in the root both are the resource id itself. Setting only AC2 was
why the dispatch jumped into the literal pool.

With these two fixes the emulator executes the whole resource-call path and runs
on into the runtime's memory manager.

## What is still missing: the entry point

Entering at the startup overlay skips the root's initialisation, so the runtime's
free list is never built and the allocator at `78FA`-`790B` walks a zero-filled
list forever. Sweeping every plausible address in the root finds only individual
runtime routines; the initialisation sequence (memory setup near `2690`-`2800`,
stack setup at `27D0`-`27E3`) has no discoverable single entry.

The UST records addresses at UST+21..+24 (`7000 7C94 7000 7C92`) but `7C94` is
the termination dispatch — running it calls `?KILL` immediately.

So the entry point is held in the program file's AOS/VS metadata, outside the
63488 bytes, exactly as suspected. **A break file would supply it**, along with
page zero and a loaded overlay at its real address: `?BRKFL` is system call 0143
in SYSID, so AOS/VS can produce one.

> **Resolved — no break file was needed.** The guess in the last paragraph was
> right about *where* and wrong about *how hard*: the entry point is in the
> file's metadata, but it is simply a ring-qualified 32-bit address at **file
> word `0x17C`**, just past the UST template. `PLOT.PR` holds `700002F6`, and
> `0x02F6` is exactly the address later found by hand — see
> *"Entry point: 0x02F6"* below, and *"The program load, finally"* for the three
> load steps that go with it. The same word works for every AOS/VS program in
> the collection, 16- and 32-bit alike: `ADVENTURE.PR` `70002347`, `ZORK.PR`
> `70075530`, `FERRET.PR` `700535E8`. `load_pr()` in `src/cpu.c` reads it
> directly, so cold start needs neither a dump nor a break file.

## Verified against the real machine

The AOS/VS debugger (`DEBUG PLOT`, ESC-B breakpoints, ESC-R start) confirmed the
two foundational assumptions:

- **Address model.** Reads of `76222/`..`76227/` returned `102670 000105 000404
  000405 000417 000657`, matching the image exactly. The origin is word `0x400`
  and PARU's `UST=400` is relative to it.
- **Page zero.** Locations 0-45 octal match the image block at word `0x2400`
  word for word, including `14/ 076425` (resource vector), `17/ 076274`
  (`.SYSTM`), `40/ 002004` (SP), `42/ 002041` (SL), `43/ 076253` (SFA).

At entry all four accumulators and carry are zero.

**Bug found by this:** low memory is 1024 words, not one 256-word page. The
emulator was copying only 256, so everything from address `0x100` up — including
the overlay directory at `USTOD` = `0x127` — was missing.

## The overlay directory, decoded

`low[0x126]` is `?NDNUM` = 2 nodes; `USTOD` points at their descriptors:

| node | descriptor | overlays | size | file block | file word | area base |
|---|---|---|---|---|---|---|
| 0 | `0x129` | 18 | 2048 words | 0 | `0x0000` | **`0x3000`** |
| 1 | `0x131` | 11 | 1024 words | 144 | `0x9000` | **`0x3800`** |

A block is 256 words (node 1's block 144 x 256 = file word `0x9000`, exactly
where the `SAVE`-prologue stride changes from 2048 to 1024 words). Both areas
are marked `7FC0` (`?AREPY`, empty) at load, so overlays come in on demand.
`?NDFAR = 5` is an offset definition, not a field: area descriptors start at
node+5, and `?ARBAS` at node+6 gives the load address.

## Resource ids below 0x400 are overlays

The first resource call the program makes is `JSR @12` with inline arg `0x0179`
— below `?USTART`, so it is an overlay descriptor rather than a root address:

    low[0x179] = ?OVEDS 0x0200   low[0x17A] = ?OVEOF 0x0000

`?OVEDS` bits 14-6 give the overlay number (8) and the low bits the node (0), so
this loads node 0 overlay 8 — file word `0x4000`, which is a `SAVE` prologue —
at `0x3000`, entering at offset 0.

`src/cpu.c` implements this and it works:

    [overlay node 0 ovl 0: file word 00000, 2048 words -> 3000]

## LEF mode — the "I/O" instructions are Load Effective Address

Every `IO 6xxx` in the disassembly is really **LEF**. Eclipse reinterprets the
I/O opcode space (opcodes 12-15) as Load Effective Address when LEF mode is on,
which is why SYSID has `?LEFE` (0265, enable LEF mode). The instruction is
decoded exactly like a memory reference, but the *address* is loaded into
AC(op-12) instead of the word at it. Implemented in both `cpu.c` and `dis.c`.

## Booting from a live dump

The AOS/VS debugger's `ESC W` writes a dump (`*.MDM`). Its layout: an 8192-word
header, then **the complete 16-bit address space** — 16-bit address A is at MDM
word A + 0x2000. `port/dump.img` is that image extracted.

It is a post-initialisation snapshot, and the proof is in page zero: the stack
pointer has moved from `0x404` to `0x279` and the stack limit from `0x421` to
`0x1425`, so the runtime's memory manager has run. Booting from it sidesteps the
entry-point problem entirely — `thissala.exe -D dump.img -e 7D15 -a <acs>`.

Note that at runtime the UST at `0x500` reads as zero and addresses around
`0x2700` are not mapped at all (the debugger rejects them). Those regions of the
file are loader *input* — the low-memory image the loader copies to `0`-`0x3FF`
— not part of the running address space. So the overlay directory must be read
from low memory at `0x127`, not through `USTOD` in the UST.

## Current state: it runs and produces output

From the live dump the emulator now:

1. takes the resource call with `AC2 = 0x0179`, an overlay descriptor;
2. loads node 0 overlay 8 from file word `0x4000` to `0x3000` correctly;
3. takes a second resource call to a root routine at `0x5DBE`;
4. reaches the runtime's character output routine at `7BD1`-`7BDC` and issues
   `?WRITE` calls that reach stdout.

`?READ`/`?WRITE` are **counted**, not NUL-terminated — the packet's `?IRCL`
gives the length and `?IRLR` returns it. Using a NUL-terminated copy dropped
every other byte.

It currently loops emitting newlines: the routine at `7BCE` fetches its
character from the fixed location `M[0x7BAD]`, so this is the "write newline"
path being called repeatedly from somewhere upstream. That upstream loop is the
next thing to chase.

## The runtime UST lives at address 0x100

Diffing the live dump's low memory against the image block at word `0x2400`
shows only 100 of 1024 words changed by initialisation — and among them, the
**UST appears at low address `0x100`**, exactly where PARU says (`UST = 0400`
octal). The loader copies it down from the file's copy at word `0x500`:

    low[0x10A] USTTC = 0001    low[0x10D] USTOD = 0127
    low[0x114] USTPR = 8000    low[0x107] USTDA = FFFF

So `USTOD` must be read from `0x10D` at runtime, not from the file's UST.

Low `0x11B`-`0x123` is the saved register block at the break:
AC0 `3C1B`, AC1 `0227`, AC2 `0279`, AC3 `4D7C`, flags `F000`, PC `7D15` —
which confirms the register state used to resume.

The other changed words are the stack registers (`0o40`-`0o54`) and the
allocator's free-list heads at `0x77`-`0x7A` (`2FD0 2FE0 2FC0 2FF0`, pointing
into a heap around `0x2E00`-`0x3000`). Those are **set up by the AOS/VS loader,
not by the program** — which is why running the pristine image from the entry
stub spins in the allocator: it walks a null free list forever.

## Where it runs to

Resuming from the dump at `7D15` with the confirmed registers, the emulator:

1. resolves the resource call and loads node 0 overlay 8 correctly;
2. executes the overlay at `0x3000` and returns into root routines;
3. resolves a second resource call to the root routine at `5DBE`;
4. opens `@OUTPUT`, and drives the runtime's real character-output path to
   produce correctly formatted text on stdout.

The output it produces is the runtime's own diagnostic:

    FATAL TO TASK ERROR 30003  ILLEGAL NUMBER CONVERSION
      REPORTED FROM 67473  ROUTINE CALLED FROM 56777

That is a *secondary* failure. The primary one is a `?READ` from `6F24` whose
packet carries record number `0x4588` with a 2048-byte record size — an offset
far beyond the end of any file here. A literal-pool address is being used as a
record number, so a value is being carried wrongly through the second resource
call. That is the next thing to chase: it is an emulation defect on a known
path, not an unknown.

## Entry point

The file's entry stub is at word `0x265C` — `EJSR 4C71` (main) then `EJSR 6E84`.
But that region is **not mapped at runtime** (the debugger rejects addresses
around `0x2700`, and the dump has only 43 non-zero words in `0x2000`-`0x2C00`):
it is loader input, consumed when the low-memory image is copied down. So the
running process never executes the stub, and a clean cold start would need the
loader's heap setup reproduced. Resuming from a dump avoids that entirely.
---

# It runs

Cold start from `PLOT.PR`, no dump, no hand-set registers. Everything below was
settled by reading the AOS/VS sources in `:UTIL` and by diffing the emulator's
memory against the `.MDM` dump word for word.

## The program load, finally

Three steps, in this order:

1. **The whole file maps at `0x0400`.** `PLOT.PR` is 0x7C00 words, so it lands at
   `0x0400`..`0x7FFF`. The shared (read-only) half is `0x3000`..`0x7FFF` —
   `USTST` = 12 blocks in, `USTSZ` = 20 blocks long, 1024 words to the block.
2. **The impure image is copied down.** File words `0x2000`.. — memory `0x2400`
   after step 1 — go to `0x0000`..`0x07FF`. That is `USTBL` = 2 blocks, and it
   carries page zero, the runtime's variables, its startup code and the overlay
   database.
3. **The UST template is copied to `0x0100`.** File words `0x0100`..`0x0125`.
   PARU puts `UST` at 400 octal = `0x100`.

The earlier note that the entry stub "is not mapped at runtime" was wrong: it is
mapped, at `0x265C`, but the *runtime* start is in low memory and reaches it
through step 2. `M[0x3BA]` holds `0x025C` and `0x3AD` jumps through it.

Low memory extends past `0x3FF` — `M[0x400]`, `M[0x402]`, `M[0x403]` are runtime
variables that come from the impure image, which is why the copy is 0x800 words
and not 0x400. `M[0x400]` = `0x0020` is the address-space size in 1024-word
pages, and the startup code takes `min` of it and what `?MEM` reports.

## Entry point: 0x02F6

Not `0x7C92`/`0x7C94` (those two UST doublewords are the *termination* vector —
`0x7C94` is `JMP 4,PC` to a `.SYSTM ?KILL`). The runtime's own start is at
`0x02F6`, inside the impure image:

    02F6  STA 2,38,PC      save AC2
    02F7  LDA 1,-2,PC      AC1 = 0x03BB
    02F8  STA 1,41         page-zero 41 = the "print and die" routine
    02F9..0308             channel-count check, MUL/DIV presence test
    0309  .SYSTM ?MEM
    0331  .SYSTM ?MEMI     ask for 10 more pages, ending up with 12
    035E  .SYSTM ?GHRZ
    0383  .SYSTM ?LEFE     turn the I/O opcodes into LEF
    038D  .SYSTM ?IFPU
    03AD  JMP @13,PC       through 0x3BA to 0x25C
    025C  EJSR 4C71        the game's own init
    025E  EJSR 6E84        the main loop

After that runs, 15 of the 17 low-memory words that the live machine's dump
pins down match exactly, and the runtime's 88-word control block at `0x2FA8`
matches all 88. (The two that differ are written later in the run than the
point the emulator was stopped at for the comparison.)

## What ?MEM and ?MEMI mean

- `?MEM` returns AC0 = unshared pages still available, AC1 = pages currently
  allocated, AC2 = the highest allocated address. Here that is 10, 2, `0x07FF`.
- `?MEMI` takes a page delta in AC0 and returns the new highest address in AC1.
  The startup asks for 10 and gets `0x2FFF`, which is exactly `USTBL` = 12 in
  the live machine's UST.

`?GHRZ` returns a small index into a five-group table at `0x362`; the live
machine ends up with (100, 10, 600) at `0x2FAC`, which is group **1**.

## Three instruction bugs the dump caught

- **`BLM`** takes its word count in **AC1**, not AC0, and moves from AC2 to AC3.
  The runtime fills its 88-word control block by storing -1 at `0x2FA8` and
  moving 87 words from there to `0x2FA9`; it also relies on AC1 coming back as
  zero for the four stores that follow. Getting this wrong copied the page-zero
  vector table over the channel tables.
- **`RTN`/`POPB`** set **AC3 = FP** after popping the return block. Callers use
  AC3 as the frame pointer immediately after a call returns without reloading
  it from page-zero 41.
- **`DIVX`** sign-extends AC1 into AC0 and then divides as `DIVS`.

`FCLE`, `FTE`, `FTD`, `FPSH`, `FPOP` come from `EBID.SR` (Data General's own
Eclipse instruction definitions, found in `:UTIL`); the runtime clears and
disables the FPU and the game never does floating point.

## The .SYSTM thunk does its own bookkeeping

`JSR @15` reaches `0x7CBC`, which pushes AC3 and traps. The **thunk**, not the
system, computes the return:

    7CC1  JMP 7CC7     error:  bump the pushed return address once
    7CC2  LDA 3,32     normal: bump it twice
    7CC3  ISZ 0,3
    7CC4  ISZ 0,3
    7CC5  LDA 3,33     AC3 <- FP
    7CC6  POPJ

So the emulator resumes at the word after the four-word trap block for an error
and one further for success, and leaves the pushed AC3 alone. Returning straight
to AC3+1/AC3+2 skips the `LDA 3,33` and leaves AC3 pointing into the runtime —
which is what made `?OPEN` file its channel number through a garbage frame
pointer and report "channel already open".

Carry still belongs to the caller: error is signalled by *which* return is taken.

## Record formats

`?ISTI` carries `?ICRF` plus three format bits (PARU "LOGICAL RECORD FORMAT
TYPES"). Without `?ICRF` the file's own format applies; the only channel that
ever arrives without one is the console, and that is data sensitive.

- **Data sensitive (2)** — delimiters are NUL, NEW LINE and FORM FEED.
  Carriage return is *not* one: the game's own line records end CR LF, and
  treating the CR as the end of the record splits every line in two.
  On output a NUL just ends the transfer with nothing appended — `Initializing `,
  `.`, `.`, `.` and ` Initialized` + NL are five records that make one line. Only
  a record that runs to `?IRCL` with no delimiter gets a NEW LINE supplied.
  On input the delimiter is part of the record: it goes in the buffer and counts
  towards `?IRLR`. Leave it out and the game trims a character of its own, so
  `look` arrives as `loo`.
- **Fixed (3)** — `?IRNH`/`?IRNL` address the record absolutely, on both read
  and write, with `?IRCL` as the record size (`?IMRS` holds junk in these
  packets). THISSALA.DB6 record 36 is read straight after DB5 records 0..3, and
  a save file written sequentially instead comes out 5120 bytes instead of 3584.

## The resource return has to restore the caller's overlay

This was the last real bug. `JSR @12` to `0x7D15` to `.SYSTM 0272` resolves the
callee, and the trampoline stores whatever comes back in AC2 at *callerFP+1*.
On the way out, `0x7D42` reads that word back and passes it to `.SYSTM 0273`.

That round trip exists for one reason: loading the callee can evict the
**caller's** overlay. So `0272` works out which area the caller's return address
lies in, notes what is resident there, and hands `0273` a token for it; `0273`
reloads it if it is gone.

Without that, a routine in node 0 overlay 5 issues `JSR @12` at `0x303B` and
resumes at `0x303D` on top of overlay 2, reading its own locals out of another
routine's code. The symptom was

    NON-FATAL ERROR 30006  UNALIGNED STRING ADDRESS

after SAVE, RECALL and SCORE, plus a line of raw memory printed as text.

## System calls implemented

`?CREATE`, `?DELETE`, `?MEM`, `?MEMI`, `?DELAY`, `?CTYPE`, `?GHRZ`, `?GTOD`,
`?GDAY`, `?LEFE`, `?LEFD`, `?LEFS`, `?OPEN`, `?CLOSE`, `?READ`, `?WRITE`,
`?RETURN`, `?ERMSG`, `?GCHR`, `?SCHR`, `?GTMES`, `?KILL`, `?IFPU`, `?STOM`,
and the agent family `0270`-`0274`.

`?STOM` is the one that is only a stub. SYSID calls `0400` "SET TIME"; the game
issues it once while setting up `@CONSOLE`, between `?GCHR` and `?SCHR`, and
never looks at what comes back — the wrapper at `0x6E8F` only tests the carry
the return path leaves.

## Files are never written

The shipped game files are opened read-only. Reads look in the save directory
first (so `RECALL` finds a saved game) and fall back to the data directory;
creates, writes and deletes only ever touch the save directory. `md5sum` over
`PLOT.*` and `THISSALA.DB*` is unchanged after a full session including SAVE.

## Verified against the real machine

Both breakpoint hits the user took on the MV/8000 reproduce exactly, from a cold
start with no registers supplied:

    0B 76425 (0x7D15) hit 1   real  AC0=3C1B AC1=0227 AC2=0279 AC3=4D7C C=1
                              mine  AC0=3C1B AC1=0227 AC2=0279 AC3=4D7C C=1
    0B 76425 (0x7D15) hit 2   real  AC0=486C AC1=486B AC2=02A5 AC3=387E C=1
                              mine  AC0=486C AC1=486B AC2=02A5 AC3=387E C=1

(The earlier reading of those octal words as `0x4C6C`/`0x38FE` was an arithmetic
slip; octal `044154` is `0x486C`.)

Independently, `RECALL` loads `DRAW.TH` — the authors' own saved game, exported
with the rest — and restores it correctly: Major mountain ledge with the
drawbridge closed, eight carried objects, a miner's helmet worn, 20 of 750
points, 110 moves.

## Debug verbs, and the two variables they need

`-g` makes any console line starting with `#` an emulator command: it is
answered by the shim and never reaches the game's parser. That keeps the
binary untouched — unfinished rooms and objects are reached by writing the
game's own variables, which is the whole point of doing this at the emulator
level instead of patching PLOT.OL.

Both addresses were found by diffing memory across a turn rather than by
reading code.

**Current room — `0x01D5`.** Dump at the start, go north, dump, come back,
dump, go northeast, dump; then look for a word that is equal in the two
"parking lot middle" dumps and different in the other two. Three words survive:
`0x01D5`, `0x01D6` and `0x02D1`. `0x02D1` is on the stack and `0x01D6` shadows
`0x01D5`; writing `0x01D5` and typing LOOK moves the player, so that is the one.
Room 1 is the middle of the parking lot, 5 the north-east corner, 6 the north
edge, 50 the bend in the river, 300 the city's periphery.

**Object locations — `0x27DC`, one word per object.** Dump, TAKE the dowel,
dump, DROP it, dump; exactly three words change and come back, and only
`0x2884` holds a room number (1 on the floor, 0 carried). THISSALA.DB6 gives
"a square dowel" as string 841, and DB6 stores five strings per object, so the
dowel is object 169 — which puts the base at `0x2884 - 168` = `0x27DC`. The
table runs to `0x28BF`, 228 entries; `-1` means not in play. Checked by putting
object 225 (the turquoise charm, string 1121) into room 1 and letting the game
describe it there itself.

`#bring` is preferable to poking a carried flag: it drops the object at the
player's feet so the game's own TAKE runs with all its side effects.

## The authors' own debug suite, and the flag that hides it

THISSALA has a debug suite built in: `ASSIST` (a 29-item menu), `ASSIST:<n>`,
`EXPRESS <section>/<room>` and `MTBL`. All four words are in THISSALA.DB3 with
system-verb codes 105, 123 and 124 — they were never removed. The parser
answers them with "I don't understand the word" only because the dispatcher
checks a flag first.

The gate is the word at **`0x01C7`**. Node 0 overlay 5 tests it twice, at
`0x32C3` and `0x32E6`:

    32C3  ELDA 0,01C7
    32C6  MOV# 0,0,SZR        skip if zero
    32C7  JMP 23,PC -> 32DE   non-zero: run the real handler
    32C8..32DD                zero: "I don't understand the word '...'"

and node 0 overlay 0 is where it gets its value, in the clear:

    32F8  ELDA 0,3F2A         0
    32FC  ESTA 0,01C7         flag := 0
    3302  ELEF 0,3F2C         ":MAIL:$$PETER_FROM_$DAVE"
    3307  JSR @12 / 7001      open it
    330A  ELDA 0,3F3C         1
    330F  ESTA 0,01C7         flag := 1
    3324  ELEF 0,3F3F  "$$DAVE"   compare against the user name
    3329  ELEF 0,3F46  "$PAUL"
    332D  ELEF 0,3F4C  "$PETER"
    3331  ELDA 2,3F2A         0
    3333  ESTA 2,01C7         flag := 0    (none of the three matched)

So the suite was for David Auerbach, Paul Chiasson and Peter Macaulay, keyed on
their AOS/VS logins. The `:MAIL:$$PETER_FROM_$DAVE` open that shows up in the
system-call trace at startup is part of the same block.

`-g` re-asserts `M[0x01C7] = 1` before every console read. That is after the
game's own init has run, it is idempotent, and it patches nothing — it is the
value the authors' logins produced.

### What the options do

`ASSIST` alone prints the menu. Options take a colon: `ASSIST:12` reports
"Available memory is 4349 words", `ASSIST:19` "Highest room is 710",
`ASSIST:13` toggles DEBUG mode (the prompt becomes `<section/room=value`),
`ASSIST:15` prompts for a score to preset, `ASSIST:28` sets up for the end game
by dropping the end-game objects into the current room.

`EXPRESS` takes two numbers separated by `/` or `:` — section then room.
Section 1 is the surface, 2 the caves, 3 the city. `EXPRESS 1/66` is the Major
mountain ledge, `EXPRESS 3/1` the Stone Ring. `MTBL` prints the current room's
move table, e.g. from the middle of the parking lot: North 6, Northeast 5,
East 4, Southeast 3, South 2, Southwest 9, West 8, Northwest 7.

The emulator's own `#goto` writes the room word at `0x01D5` without touching
the section, so `EXPRESS` is the better tool; `#goto` is only useful for moving
within the section you are in.

## The object arrays, and two things the authors never finished

Corrected from an earlier pass in these notes: the object numbers there were
derived by arithmetic on THISSALA.DB6 string numbers and came out **17 too
high** — DB6's stale record tails swallow strings, so the count per object is
not a reliable 5. Take object numbers from `ASSIST:6` instead. The square dowel
is object 152, not 169; the post office box is 84; the turquoise charm is 208.

Three arrays, one word per object, indexed by the object number itself
(not o-1), each 0x200 words apart — the three fields `ASSIST:6` prints:

    27EC + o   OBJECT[o,1]   the room the object is in
                             0 = carried, -1 = not in play
    29EC + o   OBJECT[o,2]   flags; the object's state is bits 13-15,
                             so state = (word >> 13) & 7
    2BEC + o   OBJECT[o,3]   (string base)

Objects run **1..211**. The state field was found by opening the post office
box: `0x2A40` (= `29EC + 84`) went `1280` → `22B0`, and in the authors' own
saved game every state-1 object has `2000` set and every state-0 object does
not.

### The curtain cannot be hooked

`HOOK` is verb 225 and `unhook` is verb 226. The message exists:

    [143] The curtain is now hooked to the hook, revealing a passageway north.

but the word `hook` cannot be typed as a verb. Words that are both a noun and a
verb are handled with a three-part vocabulary entry — a `5000`-series
disambiguation code, a `2800` "ambiguous" placeholder, and an UPPERCASE pair
giving both readings. `up` 5000, `down` 5001, `in` 5002 and `board` 5003 all
have the full set and all work. `hook` has only the `2800` placeholder at
`0xCB8` and the pair `HOOK 225` / `HOOK 1534` at `0x26D0`; the `5000`-series
entry was never added. `light` (verb 222) has the identical defect.

Proved by the asymmetry, since UNHOOK has a plain unambiguous entry:

    hook curtains on hooks      → Sorry ... I could not parse that
    unhook curtains from hooks  → The curtain is not hooked to the hook.

Splicing entries into THISSALA.DB3 does not fix it. `hook`=5004 was rejected
(only 5000-5003 exist, so the code indexes a fixed four-entry table), and
`enhook`=225 written over an existing, definitely-scanned entry at `0x0CB0`
was not found either — even though overwriting that slot demonstrably removed
the word that used to be there. So the lookup is not a plain scan of the
ENDVOC-terminated lists; the 88-word index at the head of DB3 selects which
part gets searched, and its key is not the first letter, the length, or any
simple sum or xor of the characters (all tested against the 496 entries).
Decoding that index is the outstanding piece.

`-g` therefore supplies **ENHOOK** at the shim level: it sets object 32 to
state 1, which is what verb 225 would have done. The game then prints its own
state-1 text and `NORTH` reaches **Low stone crawl** — a room otherwise
unreachable. `UNHOOK` still undoes it.

### Something should be in the post office box

The combination is **101654** and it works, typed as `PUSH "1"`, `PUSH "0"`,
`PUSH "1"`, `PUSH "6"`, `PUSH "5"`, `PUSH "4"` — the buttons are vocabulary
nouns 2710..2719 spelled `"0"`..`"9"`, quote marks included, with 2720 `"r"`
resetting. The box swings open and scores 10 points. And it is empty.

That is not how it was meant to be. `ASSIST:22` prints the CONTENTS table,
which is also the tail of THISSALA.DB2, as `container / flag / capacity`:

    85  / 1 / 4    a veneer box with a slot in the top (room 39)
    40  / 0 / 4    the dumbwaiter (room 136)
    84  / 1 / 2    the post office box (room 32)
    166 / 0 / 5    a canvas sack (room 399)
    167 / 1 / 3    a large glass display case (room 458)
    108 / 1 / 1    an envelope with a rare stamp (room 293)

Every container with the flag set starts with exactly one object inside it,
recorded by setting bit 15 of that object's location word:

    85  holds object 11   a CHEVY key
    108 holds object 161  a withdrawl slip
    167 holds object 168  a geological hammer
    84  holds nothing

The two flagged 0 are empty as intended. So the post office box is the single
container marked as starting occupied with nothing assigned — its content was
never placed. (The nest, object 209, holds the hen without appearing in the
table, so contents can also be set outside it.)

What was meant to be there is not recorded anywhere, so this is inference and
not a finding. Forty-four objects have location -1, never placed at all, and
the mail-shaped ones among them are the two remaining combination scraps:

    object 70  a small piece of paper    reads "1 0", clean left edge
    object 71  a medium piece of paper   reads "1 6", ragged both sides   room 265
    object 72  a large piece of paper    reads "5 4", clean right edge

Only 71 is in the world. The box's capacity is exactly 2, which is suggestive,
but putting the other two scraps inside it would be circular — you would need
the combination to reach the combination. More likely they were meant to be
scattered the way 71 is, and the box held one of the other unplaced objects.
Either way the box, and two thirds of the clue that opens it, are unfinished.
