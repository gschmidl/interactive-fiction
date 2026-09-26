# src_original — where these files came from

`ADV.F4`, `ADV.DAT` and `IOFIL.FOR` are Will Crowther's original Adventure, abandoned
unfinished in early 1976, as it sits in `DSKB:[1,2]` on the RP06 pack `t10.dsk` of Jimmy
Maher's **"TOPS-10 in a Box" v1.1** (2011).  The user's copy of that distribution is
the one in eXoIF (`eXo\emulators\TOPS-10\`); the pack is never written to — every run works on a
copy.

## These are not a fresh extraction

They were extracted and reconciled in September 2026 for the Don Woods 350-point port,
which came off the same pack, and were copied here unchanged.  The full account is
`../../WOOD0350 Colossal Cave Adventure 350pt (PDP-10, TOPS-10, full source)/src_original/PROVENANCE.md`.
In summary, three independent routes were run and **all three produced byte-identical
output for all five text files**:

| route | method |
|---|---|
| A | a from-scratch TOPS-10 filesystem reader (HOME → MFD → UFD → RIB → retrieval pointers) run offline against the image |
| B | a content-only scan of the whole image decoded as packed 7-bit text, locating runs by content, parsing no directory or RIB field |
| C | **booting the pack** and running a FORTRAN-10 program *on the PDP-10* that opened each file `MODE='DUMP'` and typed the 36-bit words as octal |

Route C is the reference because TOPS-10 itself reported both the names and the lengths
(`DIRECT/DETAIL`'s "Words written" reconciles byte-exactly).  Routes A and B independently
confirm the bytes, which matters because route C's payload had to cross a terminal.

`DIR.txt` (the pack's own directory listing, from route C) stayed with the 350 folder; it
covers both games.

## The one thing this port re-derived

Nothing about the files — but the pack was booted again, because ADV needs three facts about
the machine that the Woods port never had to ask:

* **`RAN`.** Crowther calls the FORTRAN-10 library's `RAN`, where Woods wrote his own
  generator in FORTRAN. FORLIB's was read straight out of memory: a LINK map put the `RAN`
  module at 717–745 with two words of state at 746–747, and dumping that memory from a
  FORTRAN program gives the whole routine — `MUL` by 630360016, `DIV` by 2147483647, seed
  := remainder, then `DFAD` 0.0 to normalise seed/2³¹.  `SETRAN`'s own code shows the reset
  seed, 1777777 octal, which is what 746 is assembled with.  So the seed is 524287 at every
  start, the argument is ignored, and **ADV is completely deterministic**.
* **`PAUSE`.** All nine of ADV's `PAUSE` texts were compiled and run to see exactly what
  FOROTS typed, including how it pads the message.
* **The real constants and the continued FORMAT.** `0.05` assembles as `174631463146`
  octal — truncated, not rounded — and FORTRAN-10 does *not* pad a source line out to
  column 72, so the string continued across two lines in `FORMAT 67` is simply
  concatenated.

## Files

| file | |
|---|---|
| `ADV.F4` | 744 lines. Crowther's FORTRAN-10 source: the main program plus `SPEAK`, `GETIN`, `YES` and `SHIFT`. |
| `ADV.DAT` | 733 lines, six sections: long descriptions, short descriptions, the travel table, the vocabulary, the object descriptions and the messages. |
| `IOFIL.FOR` | `IFILE`/`OFILE`, written by Paul T. Robinson at Wesleyan in June 1980 as DECUS conversion programmer to replace the old library's `IFIL`/`OFIL`. **This file belongs to `ADV`, not to `ADVENT`** — `ADVENT.FOR` never calls `IFILE`, so the distribution README's instruction to load it with either game is inherited, not true. |
| `TOPS10-in-a-Box-README.txt`, `tops10.cfg` | the distribution's own, for reference. |

## Two traps in the distribution's README, for the record

* It names `ADV.F4` correctly, but for the other game it names `ADVENT.F4`, and no file of
  that name is on the pack — it is `ADVENT.FOR`.
* "To play Crowther's Adventure, you need only type `RUN ADV`" understates it: `ADV.EXE` on
  the pack is 48 blocks, the same size as a fresh `LOAD` and a fraction of the 380 blocks an
  initialised image takes.  It was never `SAVE`d after its first run, so `RUN ADV` re-reads
  `ADV.DAT` and stops at `PAUSE INIT DONE` every single time, waiting for you to type `G`.
  Unlike the pack's `ADVENT.EXE`, though, it has not been tampered with: driven through the
  whole test corpus it agrees with a fresh build of `ADV.F4` line for line.
