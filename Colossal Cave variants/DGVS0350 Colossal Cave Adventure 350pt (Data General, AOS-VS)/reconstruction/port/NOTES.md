# The AOS/VS emulators

Two programs live here, sharing what they can:

* `aosvs16.exe` — a 16-bit Data General Eclipse with an AOS/VS shim.  Runs
  **Thissala** and **Colossal Cave Adventure**.  Source in `src/`.
* `aosvs32.exe` — an ECLIPSE MV (32-bit), for **Zork** and **Ferret**.
  **Both play**, on the files exactly as they came off the tape, and both
  save and restore.  Source in `src32/`.

Everything below came out of the dump itself — the `.PR` files, and DG's own
`:UTIL` files that came with the Thissala export (`paru.16.sr`, `paru.32.sr`,
`sysid.16.sr`, `SYSID.32.SR`, `EBID.SR`, `ECID.SR`, `SKIPS.SR`, `MASM.PR`,
`SYSTEM_CALL_SAMPLES/`, and the `.ST` symbol table beside every utility).

`tools/` holds the disassembler and the symbol matcher; see `tools/README.md`.
They are not optional extras — the 32-bit work would not have been possible
without them.

## Loading a .PR, 16-bit or 32-bit

One rule covers all four programs.  From the UST template at file word 0x100
(PARU calls it `UST = 400` octal): `USTBL` at +0x0C impure blocks, `USTST` at
+0x0F the shared area's starting block, `USTSZ` at +0x13 its size, `USTPR` at
+0x14 the program type (bit 15 set = 16-bit).  Blocks are 1024 words.

* the impure image is `USTBL` blocks from **file word 0x2000**, at memory 0;
* the shared half is the **last `USTSZ` blocks of the file**, at memory block
  `USTST`;
* everything between is heap and must start as zeros — the gap blocks are in
  fact zero in all four files, so this is checkable rather than assumed;
* the UST template, file words 0x100..0x125, goes to memory 0x100.

The arithmetic closes exactly every time, on 0x8000 words for the 16-bit
programs and 0x80000 for the 32-bit ones:

| | type | USTBL | USTST | USTSZ | file |
|---|---|---|---|---|---|
| PLOT.PR (Thissala) | 16 | 2 | 12 | 20 | 31 blocks |
| ADVENTURE.PR | 16 | 10 | 12 | 20 | 40 blocks |
| ZORK.PR | 32 | 2 | 469 | 43 | 54 blocks |
| FERRET.PR | 32 | 12 | 333 | 179 | 200 blocks |

`USTSH`, "physical starting page of shared area in .PR", reads 0 in all four,
so it is the last-blocks rule that actually places the shared half.

### The entry point is in the header

**File words 0x17C-0x17D hold a ring-qualified 32-bit entry address.**

    PLOT.PR       700002F6   ->  0x02F6
    ADVENTURE.PR  70002347   ->  0x2347
    ZORK.PR       70075530   ->  0x75530
    FERRET.PR     700535E8   ->  0x535E8

Ring 7 is where AOS/VS runs user code; the offset is a word address.

### But a 32-bit program does not start there: it starts at I.INIT

That entry is the *user's* main.  Running it directly skips the runtime's
whole set-up -- the heap high-water mark and the memory cache stay at -1, so
the first allocation hands back the address 1 and the file package writes a
buffer through it into ring page zero.

The runtime's start-up routine is **`I.INIT`**, and the loader finds it in the
**doubleword at word 0x127**, just past the UST template the loader copies.
It begins `WPSH 1,1` and ends `XJMP 0,3`, and in between it sets the stack
registers, calls `I.GINIT` and `?OUTER_MAIN_INIT`, and calls the user's main.

Two things arrive with it:

* **the user's main, pushed on the stack.**  I.INIT reads it with `LDATS`,
  after a `WPSH 1,1 / LCALL I.GINIT / WPOP 3,3` sequence whose only purpose is
  to carry AC1 across the call, and later `XCALL`s through it.
* **AC1**, which is only ever tested against zero: non-zero chooses the
  stacked address, zero a pointer out of the stack base.

Every 32-bit program follows the rule.  ZORK.PR, FERRET.PR, SCOM.PR, DISCO.PR,
LFCOPY.PR and SED.PR all carry a pointer at 0x127, and in the utilities the
`.ST` symbol tables name it `I.INIT`.  Where the word is zero -- BROWSE.PR,
CPIO.PR -- the `.PR` header's entry is `I.START` itself, so there is nothing
to arrange.

`I.START` is not it, despite the name: it is a two-instruction dispatcher that
jumps to a linker-patched address and, unpatched, drops into a `?RETURN` with
an error code.

## System calls

Both machines use the same call numbers (`sysid.16.sr` and `SYSID.32.SR`
agree: ?OPEN 300, ?READ 302, ?WRITE 303, ?RETURN 310 ...), and both reach the
kernel through a `.SYSTM` thunk whose last act is **LCALL to a ring 3
address**, which the emulator intercepts.

* 16-bit: `JSR @15`; the thunk is at the address in page-zero word 15.
* 32-bit: `XJSR @6` (`C619 8006`); the thunk is at the doubleword in words
  6-7.  In ZORK.PR that is 0x7007FBD0:

        WPSH 3,3
        LCALL 30000000,0        ; the gate
        WBR  +2                 ; <- the error return lands here
        ISZTS                   ;    and steps the caller past the call number
        ISZTS                   ; success lands here and steps past both
        LDAFP 3                 ; AC3 = the frame pointer, by convention
        WPOPJ

  So the gate returns to **at+4 on an error and at+5 on success**, and the
  caller's own layout is `XJSR @6 / .word <call number> / <error return> /
  <success continues>`.  AC3 is the frame pointer after every system call.

**The runtime replaces that thunk.**  `I.GINIT` writes its own address over
words 6-7 -- 7DF34 in FERRET.PR, installed at 7DF60 -- and that thunk range
checks its caller and the call number with two `WCLM`s before passing anything
it does not want on to the original.  Watch words 6-7 if system calls suddenly
go somewhere unexpected; it is the program doing it, not a bug.

The I/O packet differs, and its two pointers are 32-bit:

    16-bit  ICH 0  ISTI 1  ISTO 2  IBAD 3   IRCL 5  IRLR 6  IRNH 7  IRNL 8
            IFNP 9  IMRS 10
    32-bit  ICH 0  ISTI 1  ISTO 2  IMRS 3   IBAD 4-5  IRCL 7  IRLR 8
            IRNH 10  IRNL 11  IFNP 12-13

Calls the two 32-bit games make: ?CREATE ?MEM ?MEMI ?GTOD ?GDAY ?SOPEN
?SCLOSE ?RNGPR ?OPEN ?CLOSE ?READ ?WRITE ?TERM ?RETURN ?ERMSG ?UIDSTAT
?GTNAM ?IFPU.

Zork builds its system calls **on the stack**: it WBLMs an eight-word stub
into a frame local, patches the call number into it, and XPSHJs to it.  So the
emulator has to execute code out of the wide stack.

### Two shim rules that took finding

* **?GCHR must not fill all fifteen PARU words.**  PLOT.PR passes a buffer at
  FP+13 with only eight words before the top of its stack, and the thunk has
  already pushed its return address just past that.
* **A pending ?SPOS decides where a fixed record goes**; without one the
  record number in the packet addresses it directly.  Thissala relies on the
  second (its first read on a channel is record 3), ADVENTURE.PR on the first.
  PARU's ?IPST is clear in both, so the flag is not what distinguishes them.

### Flush the console before blocking on the keyboard

Both shims now `fflush(stdout)` before a console `?READ`.  Without it whatever
the game has just written sits in stdio's buffer until something else forces it
out, so interactively you type **blind** — Thissala's room description does not
appear until after you have answered it.  The test that shows it: feed the
emulator from `( sleep 8; printf '...' )` and look at the output file after five
seconds.  Fixed, the banner and first room are already there; unfixed, the file
is still empty.  A piped run never shows the difference, which is why it lasted
so long.

**This is not what puts Ferret's `->` after the line you typed.**  That one is
the game, not the shim, and it is worth writing down because it looks exactly
like a flushing bug:

       [write ch=1 <...There appear to be no exits from this room.>]
       ?READ  [read ch=2 n=6 <stand\n>]        <- reads the line, no prompt first
       [write ch=1 <-> CR LF>]                 <- writes "-> " AFTER reading
       [write ch=1 <As you attempt to stand up...>]

FERRET.PR never writes a prompt before a read.  It reads the line and *then*
writes `-> ` followed by CR LF, on a line of its own — a separator
acknowledging the command, not a prompt inviting one.  So

    stand
    ->
    As you attempt to stand up, ...

is authentic.  Zork does prompt, with a bare `>` and no newline, and looks
conventional.

## The 16-bit CPU

Beyond what Thissala already needed: `SZBO` (the atomic test-and-set), the
whole **Eclipse floating point unit** (ADVENTURE.PR's FORTRAN runtime formats
numbers with it), and one decode fix that matters everywhere —

**HLV, XCT and MSP are a .DIAC group.**  `MSP = 0103370`, `XCT = 0123370`,
`HLV = 0143370` share their low eleven bits, so a mask of 0x87FF cannot tell
them apart; bits 14-13 pick which and the accumulator is in bits 12-11.
Decoding HLV as MSP turns `AC = AC/2` into `SP += AC`, which walks the stack
out of the unshared area and into read-only shared memory some thousands of
instructions later, a long way from the mistake.

**The indirect bit applies in all four index modes, in the two-word form too.**
Bit 15 of the displacement word of `ELDA`/`ESTA`/`ELEF`/`EJMP`/`EJSR` means
indirect, exactly as bit 10 does in the one-word form.  This path honoured it
only for absolute addressing:

    if (w2 & 0x8000 && ix == 0) a = indirect(a);   /* wrong */

so `ELDA 1,@00C9,3` loaded the pointer instead of what it points at.  Thissala
never uses the indexed indirect form — 0 executions in a full session, against
100 in a few turns of Adventure — which is why the emulator was developed
against Thissala for months with this wrong.  In Adventure it is how the game
reaches `PLACE(obj)`, so every object was "not here" and none could be named.
Fixed 2026-09-10; Thissala's transcripts are byte-identical either side of it.

The displacement needs no separate sign handling: `AMASK` is 15 bits, so the
indexed arithmetic is modulo 32768 and bit 15 contributes nothing to it.

The FPU operand encoding is not in EBID.SR, only the opcodes, so it was read
out of ADVENTURE.PR itself; see the comment at the top of `src/fpu.h`.  The
one that is genuinely counter-intuitive is **FFAS**, which puts the fixed
integer in the accumulator named by bits 14-13 while taking the FPAC from bits
12-11 — the opposite way round from FLAS.

## The MV (32-bit) CPU

### The instruction table is extracted, not guessed

`MASM.PR`, the 32-bit macro assembler in `:UTIL`, carries its mnemonic table
in the clear.  Each entry is 24 bytes:

    [opcode: 2][0x0001: 2][format type: 2][0x001B: 2][name length: 2][name to 16]

**The opcode comes BEFORE the name.**  Reading it the other way round shifts
every mnemonic one record along and silently renames half the instruction set
— XJSR becomes XJMP, XPEF becomes XJSR — after which compiled code
disassembles into plausible-looking nonsense.  The parse is checked against
EBID.SR: 50 Eclipse opcodes spot-checked, all exact.  897 records in all;
`src32/mvops.h` holds the 395 in the extension space.

**Three opcodes carry two records each.**  The table is in file order and the
MV's additions sit far below the Eclipse set, so where they collide the later
record is the one these programs mean:

    8018   XOP  (Eclipse)  ->  XNADD
    8038   XOP1 (Eclipse)  ->  WBR
    8748   SYC             ->  SVC

Taking the first record hides **WBR**, and since 0x8038 also reads as a
harmless no-load ALC it then executes as a no-op — every short branch in every
32-bit program quietly falls through.  See below.

The wide instructions live in the hole the Eclipse left — **bit 15 set and a
low nibble of 9**.  Nibble 8 is the older extension space, shared between the
Eclipse instructions and the MV's X and L form loads and stores.

The **length** column is derived from the format type, not carried by MASM.
Types 06 and 07 (the Eclipse floating memory references, and FSST/FLST) and
0A (CIOI) are two-word instructions; XNDO and XWDO (type 2A) are three.
Getting a length wrong makes a skip over that instruction land on its operand.

### WBR, the short branch

Opcode base 0x8038, one word, mask `(ir & 0x843F) == 0x8038` — that is, the
no-load ALC space with a carry field of 3, a skip field of 0 and bit 10 clear.
The displacement is **eight bits signed, in bits 14-11, then 9-8, then 7-6**,
relative to the instruction's own address.

It was solved from a loop that appears identically in both games — the copy
loop inside `I.DISPLA`, at 7EB4B in ZORK.PR and 7EED0 in FERRET.PR:

    WSBI 1,1 / WSGT 1,1 / 81F8 / XWLDA / XWSTA / WADI 2,2 / FA38

`81F8` has to be +7, out of the loop to the LDAFP that follows, and `FA38`
-8, back to the WSBI.  Read that way, 7059 of the 7108 WBRs in the utilities
land on an instruction boundary; read with bit 10 included, only 5009 do.

The idioms it appears in are unmistakable once it decodes:

* a conditional branch is `<skip> / WBR / XVCT`, so most displacements are one
  more than the length of the instruction after them;
* an alternate entry point is a bare WBR, e.g. `R.GOTO: WBR +3` sitting one
  word above `I.GOTO` and jumping past its first two instructions, and
  `I.FREEB: WSSVS 0 / WBR +8` jumping into `I.FREE`'s body.

### The skips compare against zero when they name one accumulator twice

The type-10 skips (WSEQ, WSNE, WSGT, WSGE, WSLT, WSLE, WUSGT, WUSGE) take ACS
in bits 14-13 and ACD in bits 12-11 and compare **ACS against ACD** — except
that naming the same accumulator twice compares it against **zero**.  DG say
so themselves, in `:UTIL`'s SKIPS.SR:

    .MACRO WGTZ  **  WSGT ^1,^1  %      ;skip if AC > 0
    .MACRO WEQZ  **  WSEQ ^1,^1  %      ;skip if AC = 0

Over half of the type-10 instructions in these programs name one accumulator
twice, so reading them as AC-against-AC turns every test against zero into "is
X greater than itself", which is never, and the branch goes the wrong way
every time.  The range check at 77091 in DISCO.PR is the clearest case:
`WSGTI 0,32 / WSGT 0,0 / XVCT` reads as "if it is above 32, or not above 0,
complain".  The direction — ACS against ACD, not the reverse — comes from the
guard on ZORK.PR's case-folding loop at 7D2BE, `WSLE 1,0` with AC1 = 1 and AC0
the string length, which has to enter the loop when the string is not empty.

### Operand placement

By MASM format type:

* `0F`,`10` — two accumulators, bits 14-13 and 12-11.
* `08`,`0B`,`0C`,`11`,`12` — one accumulator in bits 12-11, with an immediate
  of increasing width following.
* `09` — accumulator in bits 12-11, an immediate of 1..4 in bits 14-13.
* `05` — a frame size word follows.  **It counts doublewords, not words.**
* `2B`,`2C`,`18`,`19` — X and L forms with no accumulator: the index mode is
  bits 12-11.
* `26`,`27`,`28`,`2E`,`13`,`14`,`15`,`1A` — X and L forms with one: the
  accumulator is bits 12-11 and the index mode bits 14-13.
* `2A` (XNDO, XWDO) — the accumulator is bits 14-13 and the index mode 12-11,
  the other way round from `2E`.
* `16` LCALL — a 32-bit target then an argument count; `29` XCALL is the same
  with a 16-bit displacement.
* `24` (WSKBO, WSKBZ) — no accumulator field; a five-bit bit number split
  across bits 14-12 and 5-4, numbered DG's way from the high end of AC0.
  Needs its own mask (0x8FCF), or 0xDF49 — skip on bit 20 — collapses onto
  0xC749, which is FXTE.

X form displacements are 15 bits signed with bit 15 meaning indirect, and
PC-relative counts from the displacement word.  **The instructions whose names
end in B take a byte displacement from a byte-pointer base** — ZORK.PR builds
its two file names with `XLEFB 3,-26(PC)` and `XLEFB 3,-92(PC)`, and read as
word counts they miss by half the distance.

**LPEFB and XPEFB take their index mode from bits 12-11**, not 14-13, even
though the rest of their format class uses the high field: like LPEF and XPEF
they push an address rather than load one into an accumulator, so there is no
accumulator field to displace the index.  ZORK.PR at 7D3B8 pushes the byte
pointer to its heap header with `LPEFB 0,[0394]` -- 0394 is 01CA doubled --
and reading bits 14-13 as the index makes that 2, so the pointer comes out
measured from AC2 and the heap is read into the middle of the shared area
instead of into the buffer the version check reads.

Bits 5-4 are a **carry field** on the ALC-space instructions the MV inherited,
so `mvfind` retries with them masked out.

### Calling, frames and the stack

DG's own sample programs in `:UTIL/SYSTEM_CALL_SAMPLES` settle most of this.
`INRING.SR` is worth reading in full; it ends

    LDAFP   3        ; Frame pointer in AC3.
    XWISZ   0, 3     ; Increment return address for good return to LCALLer.
    WRTN             ; Return to caller.
    ...
    ERTN:   LDAFP   3
            LWSTA   0, ?OAC0, 3 ; error code into the saved frame's AC0
            WRTN

so ?ORTN really is the doubleword at frame pointer +0, ?OAC0 at -8, and
stepping ?ORTN by one word is how a routine takes the caller's *good* return.
Its prologue comment — `WSAVR 0 ; Save frame (4 ACs, PC in AC3)` — says the
block is AC0, AC1, AC2 and AC3-as-return, plus the caller's frame pointer:
five doublewords, frame pointer at the last word of the block.

* `LCALL target,count` pushes the count as a doubleword of its own and leaves
  the return address in AC3.  Without that extra doubleword between the save
  block and the pushed arguments, every argument the callee reads at WFP-12,
  -14, -16 is off by two — which is exactly how the first ?OPEN came through
  with a call number of 0 (?CREATE) instead of 192.
* **WSAV and WSSV are not interchangeable.**  WSAV is the prologue of a
  routine reached by LCALL, which has already pushed the argument count; WSSV
  is the prologue of one reached by LJSR, which has not, so it pushes a count
  of its own.  The programs never mix them up: across seven utilities, all
  1188 LCALL targets begin with WSAVS or WSAVR and all 20 LJSR targets with
  WSSVS.
* **WRTN clears the argument list.**  It pops the frame block, then the
  argument count at WFP-10 and the arguments below it.  Nothing at the call
  site adjusts the stack — 7E664 in FERRET.PR calls and pushes its next
  argument straight away — so leaving the count behind leaks two words per
  call, and the first WPOPJ after a few of them jumps into page zero.
* `WRTN` leaves the carry the routine set rather than restoring the caller's:
  call sites test it immediately, so the carry is the return status.
* **The stack and frame pointers name the ADDRESS of the top doubleword** --
  its low word -- not its last word.  That is a one-word distinction and it
  stays invisible until something treats the two as interchangeable.  `O.ON`
  in ZORK.PR does: at 7F084 it relocates its own frame block up the stack with
  WBLM and then does `LDASP 3 / STAFP 3 / XWLDA 2,-2,3` -- the new frame
  pointer is simply the stack pointer, and the next instruction reads ?OFP
  through it.  The copy leaves ?ORTN's two words at 6FD-6FE, so only a stack
  pointer of 6FD makes that read fetch the caller's frame pointer rather than
  half of one field and half of the next.  Everything else -- the frame
  offsets, `LDATS`, `ISZTS`, the argument list -- shifts with it.
* **WPOP undoes WPSH.**  The push runs from ACS *up* to ACD with wraparound,
  so the pop runs from ACS *down* to ACD.  Popping the other way round looks
  right for the adjacent pairs the compiler uses most (WPSH 0,1 / WPOP 1,0)
  and is wrong for the wrapping ones: the storage allocator saves a pointer
  with `WPSH 2,0`, pushing AC2, AC3 and AC0, and restores it with `WPOP 0,2`,
  which must fill AC0, AC3 and AC2 in that order.  Filling AC2, AC1, AC0
  instead leaves AC3 holding whatever the routine last used it as a counter
  for, and the allocator then writes through it.

**Watch out for macros with side effects.**  `SETDW(WFP_A, wpopdw())` with
SETDW as a macro pops twice and shifts the whole frame restore by one
doubleword.  It is a function now.

### LDSP, the dispatch

The Eclipse's DSPA, widened.  The effective address is the first table entry;
the two doublewords in front of it are the low and high bounds, and an index
outside them falls through to the next instruction.  **Each entry is a word
displacement from its own address**, the same convention the X and L forms use
for PC-relative operands.

FERRET.PR at 7E4D6 dispatches AC1 over a table of four with bounds 1 and 4
whose entries read 8, 11, 13, 13; taken from each entry's own address those
land on 7E4E6, 7E4EB, 7E4EF and 7E4F1, all instruction boundaries and all four
arms of the storage allocator — and 3, the value the caller passes, selects
the same entry point `I?SALLOC` jumps to directly.  Taken from the table base
instead, one of the four lands mid-instruction.

### The DO loop

`XNDO ac,index(idx),end` is the loop header.  The index variable is a word
(XNDO) or a doubleword (XWDO) in memory; the limit arrives in the accumulator,
which the loop reloads before every pass because the header overwrites it with
the index.  That reload is why the branch closing the loop lands two or three
words *before* the header: at 7D2E0 in ZORK.PR a WBR of -31 goes back to
7D2C1, an XWLDA of the limit, and only then to the XNDO at 7D2C3.  The exit
displacement is relative to the first displacement word.

### Floating point

The MV keeps the Eclipse FPU unchanged — same four FPACs, same hexadecimal
format, same opcodes — and adds X (type 2E, two words) and L (type 1A, three
words) addressed forms of every memory reference.  `src32/mvfpu.h` is the
16-bit `src/fpuexec.h` with 32-bit addresses.

Which arithmetic and whether it is single or double comes from the **mnemonic**,
not from the opcode bits: bit 6 marks a double in the Eclipse block (FAMS
8228, FAMD 8268) but is already set in the L block for a single (LFAMS 80C9),
where bit 4 is the one that moves.  `WFLAD` and `WFFAD` are the wide float and
fix, the counterparts of FLAS and FFAS.

### Instructions whose operands took finding

* **WCLM** names the same accumulator twice when its two limits follow inline
  as doublewords -- the same "against itself" convention the skips use -- and
  is then five words long, not one.  Every same-accumulator instance in the
  utilities is followed by a plausible pair: 61/7A for a-z, 30/39 for 0-9,
  C4/15A for the system call numbers the PL/I runtime's thunk intercepts.  It
  skips when the value is inside them.
* **WCLM** takes the value in ACS and the address of the two limits in ACD,
  not the other way round.  The runtime's storage freer does `LLEF 0,[0168]`
  to point AC0 at the heap's low and high bounds -- 0168 and 016A really do
  hold the two ends of the heap -- and then `WCLM 2,0` to ask whether the
  block in AC2 is one of its own.  Read the other way the limits come out of
  the block being freed, which is a string, and every free is refused.
* **ENQH, ENQT and DEQUE** are the queue instructions the free list is built
  on, and they never appear in a disassembly: the runtime loads the opcode
  into an accumulator with `NLDAI 3,0C7E9` and runs it with `XCT 3`.  The
  header is two doublewords at the address in AC0, the first and last
  elements, and an empty queue has -1 in the first -- the free routine at
  7ED39 in ZORK.PR loads that word and compares it with -1 to choose between
  the head and the tail.  The element is in AC2; DEQUE takes the head off and
  hands it back in AC1.
* **WBLM**'s count is signed, and a negative one runs the move downwards from
  the addresses given.  Counting an unsigned AC1 down to zero instead takes
  four billion steps that wrap the address space and leave the stack registers
  in page zero overwritten -- ZORK.PR does exactly that at 7F08D with -12.
* **WCTR** takes its translate table from the stack, not an accumulator:
  FERRET.PR pushes it with LPEFB, loads WSP into AC0, runs WCTR and pops it
  again with WPOP 0,0.
* **WCST** is the one that decides whether ZORK.PR can parse a command.  Its
  count is in AC1, not AC0, and comes back as the number of bytes it did not
  reach, the matching one included; zero means it ran off the end.  The
  string is in AC3 and the table is in **AC0** -- and the table is addressed
  by *word*, not by byte: the program hands it over with LLEF, and it is
  sixteen words holding 256 bits numbered DG's way, bit 0 at the top, so
  character c is word c/16 and mask 8000 shifted right c mod 16.  ZORK.PR's
  tokeniser proves both halves: one of its two tables is a lone 8000 in the
  first word, "stop at a null", and the other is every bit but the first of
  the third word, "stop at anything that is not a blank".  The caller works
  out where the scan stopped by priming AC2 with the count plus one and
  subtracting what AC1 has left.  Read the count out of AC0 instead and AC1
  comes back untouched, the word before the end of the line measures one byte
  long, and Zork answers every command with `I don't know the word ''.`
* **WMULS** and **WDIVS** name no accumulators -- MASM's table has them among
  the no-operand instructions, at the full words E759 and E769 -- and use the
  fixed pair AC0:AC1 the way the Eclipse's MULS and DIVS use their sixteen-bit
  halves.  ZORK.PR's buffer-number hash asks for a remainder with the standard
  preamble `WSUB 0,0 / WSGE 1,1 / WADC 0,0 / WDIVS`, which is only a
  sixty-four bit dividend if the quotient comes back in AC1 and the remainder
  in AC0.
* **FXTD** and **FXTE**, at A779 and C749, sit in that same no-operand block
  and bracket a stretch of integer arithmetic -- ZORK.PR wraps FXTD ... FXTE
  round the one add in that hash.  Their neighbours in MASM's table are the
  Eclipse's FTD and FTE, floating-point trap disable and enable, and SNOVR,
  skip on no overflow, so these are the fixed-point pair: turn the overflow
  trap off, and on again.  Nothing here traps on integer overflow, so the
  emulator keeps the flag and does not act on it.
* **LLDB** and **LSTB** are the L-form byte load and store, at 84C9 and 84D9,
  and take a 32-bit *byte* address inline.  Nothing reached them until Ferret
  got past its file package.
* **WMESS** skips when it succeeds; the word after it in I.GINIT is a branch
  to the runtime's "cannot initialise" exit.
* **WSTI**, **WLDI** and **WEDIT**, the commercial decimal instructions, are
  in `src32/mvdec.h` with the evidence for each; `X.DC` -- the runtime's
  number formatter -- is built on them, and the three edit sub-programs it
  chooses between are decoded there against ECID.SR's opcode list.

### ?IPST decides whether a record number is absolute or relative

`?ISTI` bit 2 -- PARU calls it `?IPST`, "record positioning type (1 =
absolute)" -- says whether the record number in the packet is an absolute
record or a displacement from where the channel already is.  ZORK.PR reads its
whole heap with the bit clear and the number zero, meaning "the next record",
over and over; treating that as absolute re-reads the first 512 bytes for ever
and the heap never loads.  With the rule in, it reads 261 records.

The 16-bit shim's rule -- a pending ?SPOS decides, otherwise the record number
addresses directly -- is what Thissala and ADVENTURE.PR want and is unchanged;
?IPST is clear in both of those too, but neither ever asks for a relative
record.

A ?IBAD or ?IRCL still holding the fill pattern is a *request*, not a
mistake; see *What ?IBAD and ?IRCL of -1 mean*.  Only when there is no
channel buffer to fall back on does the shim refuse the transfer, rather than
writing eight kilobytes through the top of memory and hiding the real
fault.

### The shim has to return ring-qualified addresses

`?MEM` and `?MEMI` hand back addresses, and the runtime compares what it gets
against pointers it already holds: `I.GINIT` at 7E058 in FERRET.PR does
`WSGE 0,2` with the heap start in AC2.  A bare offset always compares low, so
the runtime concludes it has no room and gives up.  This is worth checking for
every call the shim answers with an address.

### A ?ORVR file keeps a header in front of every record

This is what stood between ZORK.PR and its own database, and it is worth
stating plainly because nothing in the packet says it.

Zork opens both its heaps with `?ISTI = 4014`: `?ICRF` set, record format 4,
which PARU calls **`?ORVR`**, variable length.  Such a file does not hold its
records end to end the way a fixed-length one does.  Each is stored with a
four-byte header, and the header is the record's whole stored length written
as four ASCII digits.  Both heaps are made entirely of 512-byte records, so
every header in them reads **"0516"** -- four for itself and 512 of data.

Read flat, those four bytes look like a magic number sitting in front of the
header the program wants, the version check reads "05" instead of 100, and
Zork writes `Heap version not compatible` and returns.  It is a very
convincing wrong answer: the version it wants really is at byte 4.  The
arithmetic is what settles it -- `0516` occurs every 516 bytes from offset
zero, 227 times in `ZORK_RO_HEAP` and 34 times in `ZORK_RW_HEAP`, and
227 x 516 and 34 x 516 are the two file lengths exactly.  261 records, which
is the number of reads the program does.

The shim therefore frames a `?ORVR` channel before its first read: walk the
file from the start, take each four-digit header as the stored length, and
build a table of where each record's data begins and how long it is.  The
walk has to prove itself -- every header four digits, the last record ending
exactly at the end of the file -- and a channel that fails the test keeps the
flat reading.  FERRET.PR's heaps need that fallback; they are `?ORFX`, fixed,
and have no headers.

A record length of zero in `?IRCL` is a legal empty record, not "unspecified".
Zork writes a nought-byte one when it does not recognise a word.

### The bit instructions address a bit, and one accumulator can hold it all

`WBTO`, `WBTZ`, `WSZB`, `WSNB` and `WSZBO` take a **bit** address, not a word
one: ACS is a word address and ACD a bit offset from it, sixteen bits to the
word, numbered DG's way with bit 0 at the top.  Naming one accumulator twice
is the same convention the skips and `WCLM` use -- there is no separate base
then, and the accumulator holds the whole bit address by itself.

FERRET.PR's world model is a bit array reached exactly that way.  At 54D92 it
works out

    index * 5 * 16 + 08B54

-- five words per entry, sixteen bits to the word, so that is bit 4 of word
08B5 -- and asks `WSZB 1,1` whether the bit is set.  Read as base plus offset
the address comes out as AC1 + AC1/16, hundreds of words away, and every flag
in the game answers with whatever happens to be lying there.  The symptom is
almost comic: Ferret decides it is too dark to do anything, prints a generic
"You are in a very dark room" instead of the room it is actually in, and
refuses every verb with "It's a bit dark to do that isn't it?".  With the bit
address right it opens where it should:

    Dark Room
    You appear to be lying in an exceedingly small dark room and you feel as
    if you have been sleeping for ages. You are very drowsy, your body
    appears to be quite heavy and feels partially numbed. There appear to be
    no exits from this room.
    -> stand
    As you attempt to stand up, the lid of your room bounces up due to the
    impact of your head

This one was caught by comparing against the 2022 PL/I port of the same game,
which is a direct port of the original source and opens with the same room --
see *Checking against another port of the same game* below.

### The carry is how a system call says it failed

This one held Ferret up for a long time and it is worth stating first,
because everything downstream of it looked like a different bug.

A return address on this machine carries the carry in its top bit -- DG
numbers that bit 0 -- and `WPOPJ` puts it back when it jumps.  That is the
whole error-signalling path out of a system call.  The PL/I runtime's
trampoline at 7EF8C in FERRET.PR shows it plainly:

    CRYTO                  ; carry := 1
    XPSHJ 2,3              ; call the .SYSTM stub
    CRYTZ                  ; carry := 0   -- only the SUCCESS return lands here
    ...
    SUBCR 0,0 / SEX 0,0    ; turn the carry into the function's result

The error return skips that `CRYTZ` by bumping the stacked return address --
that is what the `ISZTS` pairs in the stub and in the ring-3 gate are for --
so the carry is still 1 when the caller reads it.  Three things have to be
right for that to work, and none of them is obvious:

* **XJSR and XPSHJ must fold the carry into bit 31** of the return address
  they build.  A ring 7 address is 7xxxxxxx, so with the carry it is
  Fxxxxxxx; everything masks the address anyway.
* **WCLM must leave the carry alone.**  The .SYSTM thunk uses two of them to
  decide whether the call number is one it handles itself, and a WCLM that
  writes the carry wipes the CRYTO before the gate is even reached.
* **The shim must not clear the carry either.**  It used to, on the way out
  of every call.

Without all three, every system call reports success.  FERRET.PR asks
`?GTMES` for the argument it was started with, is told there is none, reads
the answer as a success, takes the error code out of AC0 as the length of the
argument, and asks the player to keep their filename under 33 characters --
about as far from the real fault as a symptom can get.

### WMOVR shifts, and that is what makes Ferret's parser work

`WMOVR` at E699 is "move right": the accumulator shifted down one place, the
wide relative of the Eclipse's MOVR.  Its neighbours in MASM's table are
`WHLV`, which shifts down one arithmetically, and `CVWN`, which narrows -- so
a third spelling of CVWN is exactly what it is not, and that is what it had
been implemented as.

The PL/I file package settles it.  At 7C462 in FERRET.PR it loads the
caller's buffer, which arrives as a byte pointer, does `WMOVR` on it, and
files the answer where everything afterwards treats it as a word address --
once as the target of an indirect store and once as the index of an `XLEFB`
that shifts it back up.  Only a shift down turns E0006010 into 70003008 and
makes both readings agree.  Sign-extending the low half instead leaves 6010,
and the file package writes the line it has just read a whole segment away
from the variable the game handed it.  Ferret then answers every command with
`I'm sorry?`, because the line it parses is empty.

### What ?IBAD and ?IRCL of -1 mean

A packet field left as the template's fill pattern is not garbage; it is a
request:

* **`?IBAD` of -1 means "the channel's own buffer"** -- the one the program
  named in the `?OPEN`.  FERRET.PR's file package allocates a buffer at open,
  hands the address to `?OPEN`, and then, whenever the caller wants less than
  a whole record, issues the read with the packet's `?IBAD` still -1 and
  copies out of that same buffer afterwards.  Without the rule the read has
  nowhere to go, and this is what made Ferret look for years as if its
  allocator were broken.
* **`?IRCL` of -1 means "one whole record"** -- the length the channel was
  opened with.
* **`?IRCL` of nought is a real, empty record.**  ZORK.PR writes one when it
  does not recognise a word.

An `?OPEN` for output creates the file if it is not there; a save is the
first thing either game writes.

### The two ways a program ends

Zork leaves through **?RETURN**, which the shim answers by halting.  Ferret
does not: its PL/I runtime's exit path ends at `7DF4B` with

    7DF4B  BEC9 0000 0000 0000    LCALL 00000,0

— a call whose 32-bit target is literally zero, reached immediately after the
farewell score is written.  On a real AOS/VS that is a call through location 0,
where the OS has left its own process-exit trampoline.  Nothing puts anything
there here, and word 0 reads as `0000`, which the narrow decoder takes as
`JMP 0`: Ferret printed its score and then span on that single instruction for
ever.

`BEC9` is `LCALL` — it matches `A6C9` once the acd field is masked out, so both
the disassembler and the emulator decode it correctly; the target really is
zero in the file.

So **a transfer of control to address 0 is now treated as process exit** and
halts the emulator.  Nothing legitimate executes at 0 — page zero is where SP
and FP live.

This only ever showed up interactively.  A piped run ends anyway: EOF on the
next console read halts the emulator, which is why every scripted transcript
here looked fine and only a typed `QUIT` hung.

### C719, the one instruction MASM cannot assemble

`I?SALLOC` and `I.FREE` -- both named by the symbol matcher -- each contain
one instruction MASM's mnemonic table does not have:

    7EC10  C719 001C      in I?SALLOC
    7ED40  C719 001A      in I.FREE

The table is complete enough to be trusted about this: 897 records, one
contiguous run, and it is the only one in the file.  Every PL/I program in
the dump carries exactly these two, with exactly these two operands, which is
what you would expect of two hand-assembled words in one copy of the runtime.

**It skips nought, one or two**, and all three landings are used.  It is a
step of a walk along the free-storage queue: nought is "the queue is
exhausted" -- the allocator then extends memory, the freer puts the block at
the head of the list; one is "not this one, try the next"; two is "this one"
-- the allocator dequeues it, the freer puts the block at the tail instead.
The state it is given, measured in ZORK.PR at 7EC10: **AC0** the requested
size, **AC1** the first element of the free queue, **AC2** the address of the
queue header at 0160, **AC3** -2, which is also the displacement of a block's
size word.  What the 001C and 001A operands select is still unknown.

It answered **"exhausted" every time** until 2026-09-10 — safe, because an
allocator that never finds a free block takes fresh memory instead and a freer
that thinks the queue ran out puts the block at the head of it; both are
correct, only wasteful.

**The allocator's 001C is now implemented.**  Three things settled it, none of
them a guess about the operand:

*The loop shape.*  The landings at 7EC10 are

    7EC10  C719 001C     one step of the scan
    7EC12  89B8          skip 0 -- exhausted
    7EC13  FB78          skip 1 -- WBR -3, straight back to 7EC10
    7EC14  C379 E7C9     skip 2 -- WMOV 2,0 / DEQUE, take this block

"Try the next" branches back to the instruction with nothing in between, so
**the instruction advances the walk itself**.  And what follows the DEQUE loads
the block's size, subtracts the request and compares the remainder with 8 —
a first-fit allocator deciding whether to split what it just found.

*A measurement.*  At 7EC10 during ZORK.PR's SAVE: AC0=8 words wanted,
AC1=700753E8 the block, AC2=70000160 the header, AC3=-2.  That block's size
word held **24** and its link word **FFFFFFFF** — so a block that fits, and the
old answer walked past it.

*DEQUE, which was already implemented and working.*  It takes the cell in AC0,
follows what it points at to that element's own first word, and treats
FFFFFFFF as the end.  That fixes the layout — link at offset 0, -1 ends the
chain — and, because DEQUE unlinks whatever the cell in AC0 points at rather
than searching for it, **AC2 has to walk along with AC1 as the predecessor's
link cell**.  That is what makes the `WMOV 2,0 / DEQUE` on the skip-2 landing
unlink the block that was found and not the one at the head.

So: exhausted when AC1 is -1 or null; skip 2 when `size >= request`; otherwise
step AC2 to the current element, AC1 to its link, and skip 1.

**The freer's 001A is left alone**, still answering "exhausted".  Only the
allocator's site has ever been observed to execute: a Zork session with two
saves and two restores runs 001C four times and never reaches 001A at all, and
FERRET.PR executes no C719 whatever.  Implementing a test that has never run,
for an operand whose meaning is still unknown, would be the guess this note
originally warned against.

Either side of the change ZORK.PR's transcript is byte-identical, saves and
restores included; the four scans that used to answer "exhausted" now find
their block, so the allocator stops extending memory to serve a request the
free list could already meet.

### Where each game gets to

**Both play.**  Both read the files exactly as they came off the tape --
nothing in either `data/` was edited -- and both save and restore.

    ./aosvs32.exe data/ZORK.PR

    West of house
    You are in an open field west of a big white house with a boarded
    front door.
    A rubber mat saying 'Welcome to Zork!' lies by the door.
    There is a small mailbox here.
    >open mailbox
    Opening the mailbox reveals a leaflet.

    ./aosvs32.exe data/FERRET.PR

    Dark Room
    You appear to be lying in an exceedingly small dark room and you feel as
    if you have been sleeping for ages. You are very drowsy, your body
    appears to be quite heavy and feels partially numbed. There appear to be
    no exits from this room.
    -> stand
    As you attempt to stand up, the lid of your room bounces up due to the
    impact of your head
    -> lift lid
    There is an ominous creaking sound followed by a clunk.  Light encircles
    your body temporarily blinding you.
    Twilight Room
    Your eyes appear to have adjusted to the light. Beyond your shell-like
    cover you can see a number of machines, dotted with switches and
    readouts.

Zork was reached first, and needed the `?ORVR` framing, `WCST`, `WDIVS`,
`FXTD`/`FXTE` and `WCMV`'s operand pairing.  Ferret needed the carry
convention above, `WMOVR`, the bit-address rule, the two packet rules,
`?GTMES` and `?RNGPR` answered from PARU's own packet definitions,
`LLDB`/`LSTB`, and an open for output that creates.  ZORK.PR has no
same-accumulator bit instruction anywhere, which is why it played for hours
with that rule wrong; FERRET.PR has 348 of them.  Every one of them was found the same way: run the game,
read the trace at the point it goes wrong, and work back.  That really is the
method here.

The instructions still implemented from inference rather than from evidence,
and so the first things to doubt:

* `WSKBO`/`WSKBZ` — the five-bit split of the bit number is inferred, but it
  is now cornered rather than guessed.  Of the 120 orderings of those five
  bits, eight give Ferret's heaps a record format other than the fixed one
  its file package cannot serve, and four of those also keep every WSKBO and
  WSKBZ in the two programs testing a bit in 16..31.  All four move Zork's
  heaps out of `?ORVR`, and the record framing that makes 227 x 516 come out
  exact then stops applying, so all four are wrong.  The reading in use here
  is the only one left.
* `NSALA`/`NSALM`/`NSANA`/`NSANM` and their W forms — the all/none polarity of
  the four spellings is a guess.
* `XNDO`'s exact step: the index is stepped before the test and both the
  accumulator and the memory copy hold the stepped value, which is what the
  case-folding loop needs.
* `WSTI`/`WLDI`'s zoned digit format is a choice, not a finding, and `WEDIT`
  knows only the seven sub-program opcodes X.DCA uses out of ECID.SR's
  twenty-four -- it stops on any other rather than running on.
* `WMESS` reports success and does nothing else.
* `WMOVR` shifts down one place; whether the bit shifted out reaches the
  carry, and whether the carry comes in at the top the way a rotate
  would, has had no test.  The carry is left untouched.
* `C719`'s **operand** -- what 001C and 001A actually select is still unknown.
  The allocator's 001C is now implemented from its loop shape, a measurement
  and DEQUE's own layout (above); the freer's 001A is still answered
  conservatively, and has never been seen to execute.
* `WCMP`'s operand *pairing* is now known -- it is WCMV's, AC0 with AC2 and
  AC1 with AC3 -- but which of the two strings the -1 and the 1 are about
  has had no test.

### One thing that is understood but not explained

Both runtimes, in the routine that extends memory, do

    WADC 1,1                  ; -1
    XWSTA 1,0,3               ; store it at the frame pointer + 0

with AC3 holding the frame pointer, and then return through that frame with
WRTN.  ?ORTN is certainly at +0 — DG's own sample proves it — so the machine
must do something with a -1 there that this emulator does not know about,
most likely the "virtual return" the parameter file mentions two words further
up (`?DESC = 2`, `?VRTN = 4`, "for user runtime resource management"), which
would be a gate into ring 3 like the one LCALL uses.

The reason this is no longer fatal is that fixing WPOP means AC3 is the
allocator's own saved pointer at that point, not the frame pointer — but it is
worth knowing that the sequence exists and that both games run through it.

## Naming a stripped program

Neither game shipped a symbol table, but both are linked against the same
AOS/VS runtime as the `:UTIL` programs, which did.  `tools/symmatch.py` takes
the twelve words at every symbol in every donor `.ST` and looks for the same
twelve words in the target; a unique hit names the routine.  144 routines in
FERRET.PR and 54 in ZORK.PR, which is how the runtime in both turned out to be
**PL/I** — `P?PUT`, `P?GET_LINE`, `P?OPEN`, `I.START`, `O.SIGNAL`, `I?SALLOC`,
`I.DISPLA`, `X.DC` — and how the code could be read as what it is.

### Asking a running program a what-if question

`-P <pc> <addr> <val>` sets a word every time the PC reaches an address, up to
eight of them at once.  It is not a patch and nothing is written to disk: it
is how the Ferret finding above was made, by holding one field at a different
value and watching how far the program then got.  When a program stops on
something that looks like a decision made much earlier, this answers "would it
have worked?" in one run, without guessing at an instruction's meaning first.

### One thing about this machine, not that one

Piping a script into the emulator's standard input on Git Bash is
intermittently unreliable — a run will occasionally stall with the pipe half
read, and the same script redirected from a *file* always works.  It is the
shell's pipe handling, not the emulator: `< script.txt` rather than
`cat script.txt |`.  Interactive play is unaffected.

## Building

    make            # both emulators and the disassembler
    make aosvs16.exe
    make aosvs32.exe

`aosvs16.exe` finds its program automatically when exactly one `.PR` sits
beside it or in `./data`; otherwise name it on the command line.  `-h` lists
the options.  Both take `-v` to trace system calls and `-t` to trace
instructions; the 16-bit one adds `-w lo hi` to watch a range of memory and
`-F` to trace floating point, the 32-bit one `-D lo hi` to dump memory at the
end, `-W lo hi` to report every write to a range with the instruction that
made it, and `-F` for floating point.

`-W` is worth reaching for early.  A frame quietly overwritten from somewhere
else is the hardest kind of bug to find by reading a trace, and it found both
of the last two — the leaked argument counts and the WCMV over page zero — in
one run each.
