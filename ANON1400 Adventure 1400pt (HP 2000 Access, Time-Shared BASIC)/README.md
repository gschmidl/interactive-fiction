# Adventure ]I[ — HP 2000 Access Time-Shared BASIC

A 1400-point Colossal Cave descendant written around 1978/79 by Alex Guma,
then an 11th-grader at Falls Church HS in Fairfax County, VA, on the school
district's HP 2000 Access timesharing network. Known in the archives as
ANON1400.

It is Colossal Cave by way of Zork jokes: the Frobozz Magic Sno-Disc Company,
a zarka that will only eat pizza, a ski resort, a subway, and a nuclear
reactor that melts down 216 turns in unless you do something about it.

* `src_original/` — the archived material, untouched, from
  <https://github.com/Quuxplusone/Advent/tree/anon1400/ANON1400>
* `port/` — a native Windows build. See [port/README.md](port/README.md).

## The port

`port/advent.exe` is an interpreter for HP 2000 Access Time-Shared BASIC. It
runs the archived listings directly, and builds the data files by executing
the game's own data-builder programs. Nothing was rewritten.

Both surviving sources are playable:

```
advent.exe            Alex Guma's 2023 reconstruction
advent.exe -v orig    Rick Hammerstone's 1981/82 line printer listing
```

Each source arrived with one defect the other does not have: the
reconstruction had dropped two lines, breaking `STAB` and `CHOP`, and the
printout reads `ALLEZ`'s room number into the wrong variable. Both are
repaired using the reading the other source supplies — two lines added, one
character changed, both shown in `port/basic/*/fixes.diff` and both undoable.

With that done the two sources are behaviourally indistinguishable: all ten
regression scripts produce byte-identical transcripts from either.
[port/README.md](port/README.md) lists every difference between them.
