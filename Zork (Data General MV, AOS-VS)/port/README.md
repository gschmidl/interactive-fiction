# Zork — Data General MV, AOS/VS

The 1984 AOS/VS port of Dungeon, endgame included.  `ZORK.LOGO` says so
itself: *"This is an AOS/VS rev of ZORK as of 7/16/84 by PZ"*.

**It plays.**  `aosvs32.exe` is an ECLIPSE MV emulator with an AOS/VS shim,
and it runs the unmodified `ZORK.PR` — this is not a rewrite:

    ./aosvs32.exe data/ZORK.PR

    West of house
    You are in an open field west of a big white house with a boarded
    front door.
    A rubber mat saying 'Welcome to Zork!' lies by the door.
    There is a small mailbox here.
    >open mailbox
    Opening the mailbox reveals a leaflet.

Movement, the parser, containers, combat, scoring, `DIAGNOSE`, the trophy
case — and `SAVE "name"` / `RESTORE "name"`, which round-trip.  Save files go
in the current directory; use `-s <dir>` to put them somewhere else.

`data/` holds the original files exactly as they came off the tape, and the
game reads them as they are — all 261 records of `ZORK_RO_HEAP` and
`ZORK_RW_HEAP`.  They are `?ORVR` files, variable-length records, which on
disk means every 512-byte record carries a four-byte header; the shim frames
them.  That header is why both heaps appear to begin with the ASCII bytes
`0516` — four for the header itself and 512 of data — and why the game used to
stop at `Heap version not compatible`.

`src32/` is the emulator's source, `tools/` the disassembler and the symbol
matcher that made it possible, and **`NOTES.md` is the thing to read** —
everything known about the machine, where it came from, and what is still
guessed.

    make            # rebuilds aosvs32.exe

A script piped into the emulator's standard input on Git Bash is
intermittently unreliable; redirect from a file instead (`< script.txt`).
Interactive play is unaffected.

The same emulator, from the same source, runs Ferret; each port carries its
own identical copy of it in `src32/`.
