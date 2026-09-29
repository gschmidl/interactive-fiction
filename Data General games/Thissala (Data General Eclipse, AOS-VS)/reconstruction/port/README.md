# THISSALA — native port

THISSALA is a 1985 text adventure for Data General AOS/VS, written by David
Auerbach, Paul Chiasson and Peter Macaulay of Data General's Corporate
Consulting Group. This is revision 0.60, the AOS/VS rehost.

`thissala.exe` runs **the original, unmodified binaries**. Nothing was
reimplemented and nothing was patched: `PLOT.PR` and `PLOT.OL` are loaded
exactly as AOS/VS loaded them and executed on a 16-bit Data General Eclipse
emulator, with a shim underneath that answers the system calls the program
makes. The game's own runtime does the parsing, the text formatting and the
overlay swapping, just as it did on the MV/8000.

    thissala.exe

That is all. The game files live in `data/`; the emulator also looks in the
current directory, beside the executable, and in `../src_original`.

    -d <dir>   directory holding PLOT.PR, PLOT.OL, THISSALA.DB1..DB8
    -s <dir>   where SAVE writes and RECALL looks (default: the current one)
    -g         debug: the game's own ASSIST/EXPRESS/MTBL, plus # verbs
    -v         trace system calls, overlay loads and file I/O
    -h         usage, including the diagnostic options

## Playing

`HELP` prints the authors' own release notes and command summary. The commands
that are not obvious:

    RECALL     restore a saved game (SAVE / RECALL, not SAVE / RESTORE)
    STATUS     power, coordination, magic, wisdom, knowledge
    RTOD       real time of day        TIME   the game's internal clock
    THISSALA   the authors' description of the game
    RECOMMEND  suggest a word for the vocabulary
    BRIEF / VERBOSE / SCORE / INVENT / LOOK IN

`data/DRAW.TH` is a saved game that came off the tape with everything else — the
authors' own, 110 moves in on the Major mountain ledge. `RECALL` it with the
name `DRAW` (the game appends `.TH` itself).

Saves are written to the current directory by default, never into `data/`. The
shipped files are opened read-only.

## Debug (-g)

    thissala.exe -g

### The game's own debug suite

THISSALA ships with one, written by its authors: a 29-item `ASSIST` menu plus
the `EXPRESS` and `MTBL` verbs. It is gated on the word at `0x01C7`, which node
0 overlay 0 sets during startup and then clears again unless the AOS/VS user
name is `$$DAVE`, `$PAUL` or `$PETER` — David Auerbach, Paul Chiasson and Peter
Macaulay. With the flag clear the parser answers those verbs with "I don't
understand the word", which is why they look as though they are not there.

`-g` re-asserts the flag before every console read. Nothing is patched: this is
the value the authors' own logins produced.

    ASSIST            print the 29-item menu
    ASSIST:<n>        run option n
    EXPRESS <s>/<r>   teleport to section s, room r  (":" works too)
    MTBL              the move table for the current room

The menu:

     1 this display              16 move an object into the room
     2 GTEXT array               17 VS+EL railroad information
     3 MTEXT array               18 display monster information
     4 DTEXT array               19 highest room number
     5 MTBL                      20 set STATUS value
     6 information on an object  21 display IVAL/VECT
     7 MODA array                22 display contents array
     8 VRES <xxx>                23 set/reset input timeout mode
     9 move table size           24 simulate NASTY function
    10 current location/section  25 not currently used
    11 SECTION array             26 display GSIZE for an object
    12 available memory          27 monster tracing
    13 enter/leave DEBUG mode    28 set up for end game
    14 express to a room         29 display XC/YC
    15 preset score

`ASSIST:13` turns on the authors' DEBUG mode, which changes the prompt to
`<section/room=value`. `ASSIST:28` drops the end-game objects into the current
room — the shortest route to the ending the 1985 players never reached.
The room number is relative to its section: absolute = SECTION[s] + room, and
`ASSIST:11` prints the SECTION array (bases 0, 125, 292, 382, 510, 650 for the
six sections; 710 rooms in all). `notes/rooms.md` lists every one of them with
its EXPRESS code, an index by name, and the codes for the end-game chain.

### Emulator commands

`-g` also makes any line starting with `#` an emulator command, answered by the
shim and never seen by the game:

    ENHOOK                hook the curtains (the game's HOOK verb is broken)
    #room                 show the current room number
    #goto <n>             set the room number within the current section
    #where <obj>          show where object obj is
    #move <obj> <n>       put object obj in room n
    #bring <obj>          put object obj in the current room
    #state <obj> [n]      show or set an object's state
    #peek <addr> [words]  dump memory (addresses are hex: 0x1D5)
    #poke <addr> <value>  store one word
    #regs                 accumulators, carry, PC, stack
    #dump <file>          write the 32K memory image out
    #set room|obj <addr>  retarget the two tables
    #help                 the list

Numbers are decimal, `0x` for hex. Prefer `EXPRESS` over `#goto` for moving
around — `#goto` writes the room word (`0x01D5`) without touching the section,
so it can only reach rooms in the section you are already in. `#peek`, `#poke`,
`#dump` and `#regs` are for looking at the machine rather than the game.

The object arrays are the ones `ASSIST:6` prints as `OBJECT[o,1..3]`, one word
per object indexed by the object number itself: `0x27EC + o` is the room it is
in (`0` carried, `-1` not in play) and `0x29EC + o` is its flags, with the
state in the top three bits. Objects run 1..211. Take object numbers from
`ASSIST:6` — arithmetic on THISSALA.DB6 string numbers comes out 17 too high,
because DB6's stale record tails hide strings.

### ENHOOK

The game's own `HOOK` verb (225) exists, and so does its message — *"The
curtain is now hooked to the hook, revealing a passageway north"* — but the
word cannot be typed. `hook` resolves to the noun; the vocabulary entry that
would give it its verb reading was never added, and `light` (verb 222) is
broken the same way. `unhook curtains from hooks` parses and answers *"The
curtain is not hooked to the hook"*, so the undo works and the do does not.

`-g` supplies `ENHOOK`, which sets the curtains (object 32) to state 1 exactly
as the real verb would. Stand in the Library (`EXPRESS 2/13`) and type it:

    ENHOOK
    LOOK      The curtains are drawn back, hooked to the curtain hooks on
              each side.  There is a passage north behind the curtains.
    NORTH     Low stone crawl

That room is unreachable in the game as shipped. The game prints its own text
throughout and its own `UNHOOK` still undoes it; nothing is patched.

## What is in here

    thissala.exe            the game
    dis.exe                 Eclipse disassembler used to work the binaries out
    data/                   the original AOS/VS files, untouched
    src/cpu.c               the emulator and the AOS/VS shim
    src/debug.h             the -g debug verbs
    src/dis.c               the disassembler
    src/eclipse.h           instruction decode tables shared by both
    notes/rooms.md          every room with its EXPRESS code, indexed by name
    notes/objects.md        all 211 objects and where they start
    notes/file-formats.md   how all of this was worked out
    notes/plot-pr.dis       disassembly of the root
    notes/plot-ol.dis       disassembly of the overlays
    dump.img               a 32K memory image taken from the live MV/8000

Build with `make` (gcc, C99, no dependencies).

## Accuracy

The emulator was checked against the real machine rather than against
expectations. Two breakpoints the original AOS/VS system reported at `0x7D15`
reproduce register for register from a cold start, and the runtime's own
88-word control block at `0x2FA8` matches the live memory dump word for word.

`notes/file-formats.md` records the whole derivation, including the three
instruction-set bugs (`BLM`'s operands, `RTN` restoring AC3, `DIVX`) and the
overlay-restore rule that the AOS/VS resource-call protocol depends on.

## Rebuilt 2026-09-09 on the shared AOS/VS emulator

`thissala.exe` is now built from the same 16-bit Eclipse core that runs
Colossal Cave; an identical copy of those sources is in `src/`.  Four CPU
and shim bugs were
found while getting that game up, and all four were live here too:

* **HLV was decoding as MSP.**  `HLV` (0143370) and `MSP` (0103370) share
  their low eleven bits, so `AC = AC/2` was executing as `SP += AC`.  Object
  handling was wrong because of it: the parking lot listed the rope instead of
  the square dowel, and `take dowel` did not work.  Both are right now.
* **?GCHR filled all fifteen PARU words** into a buffer the program only
  leaves eight words for, overwriting the return address the .SYSTM thunk had
  just pushed.  It answers four fields and clears through ?CH8 now.
* **?RCALL did not return AC0 = 0**, so the trampoline's procedure-variable
  marker ended up in bit 15 of the saved return address.
* **?SPOS now decides where a fixed record goes** when one is pending;
  without one the record number in the packet still addresses it directly,
  which is what this game relies on.

The entry point no longer has to be supplied by hand either: it is a
ring-qualified address at file word 0x17C of the .PR header, and for PLOT.PR
it reads 700002F6 — exactly the 02F6 that had to be found by trial before.

**Known regression:** the game's own author verbs (`ASSIST`, `EXPRESS`,
`MTBL`) are refused again.  `-g` still pins the gate word at 0x01C7, and the
parser does read it as 1, but the words no longer reach the vocabulary — the
gate was worked out against the emulator as it behaved before the HLV fix and
needs re-deriving.  The emulator's own `#` verbs (`#help`, `#goto`, `#where`,
`#bring`, `#state`, ...) and `ENHOOK` are unaffected and still work under `-g`.
