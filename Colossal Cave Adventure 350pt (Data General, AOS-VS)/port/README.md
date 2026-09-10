# Colossal Cave Adventure — Data General AOS/VS

The original `ADVENTURE.PR` from the MV/8000 dump, run unmodified on a 16-bit
Eclipse emulator with an AOS/VS system-call shim.  Nothing is patched and no
memory image is pre-loaded: the program is cold-started from its own `.PR`.

    adventure.exe                 # plays; data/ is found automatically
    adventure.exe -s <savedir>    # where saves are written

This is the stock **350-point Crowther & Woods** game — `ADVENTURE.DB` is an
ordinary `ADVENT.DAT` with the usual twelve sections and the 349/9999 class
table.  It calls itself "XYZZY Adventure" in its welcome line.

## What works

Boots, prints the welcome and the full instructions text, moves, describes
rooms, runs the whole verb set (including the verb-implies-object forms —
`eat` finds the food, `light` finds the lamp), keeps score, kills you, offers
reincarnation, and quits with a correct score line.

Naming objects works too — `take lamp`, `unlock grate`, `drop lamp`, `xyzzy`.

## The object-name bug, and what it was

Until 2026-09-10 naming an object did nothing: `take lamp` answered "I don't
understand that!" and a bare `lamp` answered "I see no LAMP here" with the lamp
in the room.  It was **the emulator, in the two-word memory reference**:

```c
if (w2 & 0x8000 && ix == 0) a = indirect(a);   /* wrong */
if (w2 & 0x8000)            a = indirect(a);   /* right */
```

Bit 15 of the displacement word is the indirect bit and applies in all four
index modes.  Restricting it to absolute mode left the indexed forms loading
the pointer instead of what it points at.  The one-word `ea()` had always
applied `MR_IND` in every mode; the two-word path simply had not caught up.

The game reaches an object's location with

    4002  D738 00C9   ESTA 2,00C9,3     store &PLACE(obj)
    4005  AF38 80C9   ELDA 1,@00C9,3    load PLACE(obj) through it
    4009  CD0C        SUB# 2,1,SZR      compare with the room number

so AC1 came back as `104C`, the address of `PLACE(2)`, and the game compared
that against the room number.  It always differed, and every object in the game
was therefore "not here".

The vocabulary was never at fault.  It is a character trie of 890 three-word
nodes at `0x023D` — child is the next node, `+2` is the sibling, and the value
sits in `+0` of the node after the last letter, so `KEYS` yields 1001 and
`LAMP` 1002, exactly stock.  `0x0FE5` holds the **verb**, not the object.

**Why Thissala never showed it:** Thissala does not use the indexed indirect
form at all.  Counting the two-word indirect operands actually executed, a few
turns of Adventure use it 100 times; a full Thissala session uses it 0 times.
Thissala's transcripts are byte-identical either side of the fix.

## Files

`data/ADVENTURE.PR` the program, `data/ADVENTURE.TXT` the message file (874
fixed 72-byte records; the first two characters of each are the text length in
16-bit words).  `ADVENTURE.DB` and `ADVENTURE.ST` are in `../src_original` and
are not read at run time — the `.DB` is the source-format database and the
`.ST` is the linker symbol table.

The emulator source is in `src/`; it is the shared AOS/VS 16-bit emulator, and
the same binary runs Thissala.  See `NOTES.md` beside this file.
