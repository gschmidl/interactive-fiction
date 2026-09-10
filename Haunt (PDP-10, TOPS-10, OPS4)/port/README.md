# HAUNT — a Windows port

*HAUNT*, by **John E. Laird** at Carnegie-Mellon University, written in
**OPS4** — a rule-based production-system language developed at CMU — and
run on the DECsystem-10. The game says so itself:

```
This is HAUNT. Version 4.6
Copyright (C) 1979,1980,1981,1982 John E. Laird
The max score is 440.
Send gripes to John.Laird@CMUA.
Or John Laird, Computer Science Department, Carnegie-Mellon University
Pittsburgh, Pa. 15213
```

There is no source. This port runs the original image on a PDP-10
emulator with just enough of the TOPS-10 monitor underneath it, so what
you play is the actual program, instruction for instruction — not a
rewrite.

## This is the OPS4 original, not the OPS5 rewrite

Laird began rewriting Haunt in OPS5 in the early 1980s and never finished
it; that unfinished source survives on the IF Archive as `haunt.ops5` and
is what most people mean by "the Haunt source". **It is not this game.**
What runs here is the finished OPS4 article, version 4.6 of 21 June 1982,
440 points, still being maintained three years after it was started.

The distinction was established from the pack itself, not assumed:

* `literalize` and `strategy mea`, which saturate the OPS5 file, occur
  **zero** times anywhere on either disk pack.
* `I assume that means yes.`, a literal message in the OPS5 file's
  `name01` production, is absent. Game text on this pack *is* stored as
  readable packed ASCII, so that absence means something.
* `OPS4` *is* present, in `USER.LSP[A110PS99]` — "a basic set of
  extensions to OPS4".
* The `OPS5` strings on the pack are all `OPS5E.DOC`, DEC's own manual,
  which also appears on the second pack that has no Haunt content at all.

`haunt.ops5` is kept in `..\reference\` as a cross-check only. It is a
different and unfinished version, so nothing from it is used to fill any
gap in what was recovered here.

## Where the image came from

eXo's Haunt package is SIMH's `pdp10` (KS10) plus two RP07 packs carrying
Bill Schaub's Steuben Technologies TOPS-10 7.03 distribution, with Haunt
preinstalled. The game is `SYS:HAUNT.EXE` = `DSKB:[1,4]HAUNT.EXE`, 1432
blocks, and it was lifted off the pack offline — no emulator involved.

See [docs/EXTRACTION.md](docs/EXTRACTION.md) for the full account. In
short, four independent checks agree that the extraction is byte-exact:

* the `.EXE` page-map directory is self-consistent — a `1776,,17` header
  with its `1777,,1` terminator exactly where the length says;
* the page map accounts for file pages 1–357 contiguously, plus page 0
  for the directory itself: 358 pages of 4 blocks = **exactly the 1432
  blocks** TOPS-10 reports;
* an identical allocation-boundary word brackets the region on both
  sides;
* 20 of 24 512-byte chunks spread across the assembled core appear
  **verbatim** in a SIMH save state taken from the game running under the
  original TOPS-10. The four that differ are low-segment pages holding
  JOBDAT, the stacks and OPS4's working memory, which are exactly the
  pages a running game rewrites.

`.JBSA`, `.JBREL` and `.JBHRL` in the loaded image independently confirm
the entry point (`3230`) and both segment limits.

## Build

Needs a C compiler (gcc or clang; MinGW-w64 is fine) and Python 3 to
regenerate the embedded core image.

```
make
```

That produces a standalone `bin\haunt.exe` — the core image is compiled
in, so the executable needs nothing else at runtime.

## Play

```
bin\haunt.exe
```

You start at a bus stop with a fistful of tokens. Waiting is the opening
move: a bus arrives, and it is the only way out of the intersection.

```
bin\haunt.exe -h
```

lists the options.

## Notes

The emulator core and monitor shim are shared with the EXPLOR, Archon,
Crystal Cave and Advent-382 ports. Haunt needed no changes to either: the
image ran correctly on the first attempt.

EXPLOR's `-u` flag is deliberately **not** carried over. It patches fixed
core addresses that police cave hours and turn limits in *that* build; in
Haunt's image those addresses hold unrelated data, so the flag would
silently corrupt the game. Haunt has its own clock — the OPS5 rewrite
declares `realtime`, `morning`, `midnight` and several timed statuses —
but nothing equivalent has been located in this build, and no such patch
will be invented.

Five monitor calls go unanswered at startup — `CALLI 110`, `115`, `116`
and five `GETTAB`s — each asked once and accepted by the game with the
shim's benign default. Nothing observed so far depends on them.

Haunt has **no save command of its own** — the original never had one.
The only save it can be given is a snapshot of the emulated machine,
taken from outside the game, and the port provides one:

```
*#save
[saved in haunt.core -- "haunt -c" resumes it]
```

A line beginning with `#` is answered by the emulator and never reaches
the game, so it costs no turn and the parser never sees it. `#save
[file]` writes a snapshot; `haunt -c` (with `-f` if you named a file)
resumes it. `#help` lists the meta-commands.

The moment `#save` is typed is a genuinely safe one to snapshot. The
input buffer is empty — that is why the emulator was asking for a line at
all — and the instruction underway is an `XCT` of the `TTCALL` in AC 11:
`INCHRW`, a single-character read whose store into memory happens only
after the character is returned. Nothing has been written yet, so
saving `PC-1` resumes by re-executing that read, which simply waits for
input again. Verified end to end: saving at the gate outside CHEZ MOOSE
and resuming restores the location, the inventory (`token`, singular —
one was spent on the bus fare) and the score, and play continues
normally.

The snapshot carries the program counter and processor flags as well as
core, because Haunt has no resume logic of its own; a memory-only image
would restart the game from the top carrying stale state.
