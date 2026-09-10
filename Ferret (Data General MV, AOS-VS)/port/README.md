# Ferret — Data General MV, AOS/VS

"AOS/VS Ferret Rev 4.10", per its own `FERRET.CLI`.  Written in PL/I in 1982
by programmers at Data General's UK Systems Division, who chose to stay
anonymous, and extended by them for decades afterwards — the version here is
the one that was on the machine.

**It plays.**  `aosvs32.exe` is an ECLIPSE MV emulator with an AOS/VS shim,
and it runs the unmodified `FERRET.PR` — this is not a rewrite:

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

The parser, the world model, the room and message text, the plaque's ASCII
art, `HELP`, `SCORE` with its phases and ranks, `BRIEF`/`VERBOSE`, and
`SAVE "name"` / `RESTORE "name"` — the quotes are Ferret's own requirement,
and it says so if you leave them off.  Save files go in the current
directory; use `-s <dir>` to put them somewhere else.

This is **rev 4.10**, not the 10.00 the authors released for DOS in 2022, so
the wording differs here and there and the map is smaller: `lift lid` where
the modern one takes `push lid`.  That later port was the thing that finally
gave the game away — set against its opening, this emulator's "You are in a
very dark room" stopped looking like an authentic 1982 quirk and turned out
to be one bit-addressing rule wrong, with the whole world model reading
garbage as a result.  `NOTES.md` has that and the rest.

`data/` holds the original files exactly as they came off the tape.  `src32/`
is the emulator's source, `tools/` the disassembler and the symbol matcher,
and **`NOTES.md` is the thing to read** — everything known about the machine,
where it came from, and what is still guessed.

    make            # rebuilds aosvs32.exe

A script piped into the emulator's standard input on Git Bash is
intermittently unreliable; redirect from a file instead (`< script.txt`).
Interactive play is unaffected.

The same emulator, from the same source, runs Zork; each port carries its
own identical copy of it in `src32/`.
