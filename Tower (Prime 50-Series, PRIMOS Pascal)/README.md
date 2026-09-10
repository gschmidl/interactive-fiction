# Tower (PROGRAM CHIP1)

A castle-and-caverns adventure in PRIMOS Pascal, signed inside the game itself
as *"This dungeon courtesy of R. L. Tice."* -- the same Randall Tice who wrote
[Ankh](../Ankh%20(Prime%2050-Series,%20PRIMOS%20Pascal)/README.md), and the
program name `CHIP1` points at Chip Roberson, one of the friends thanked in
Ankh's banner.  `GETCOMMAND` is again Dave Montuori's, with the source's own
note that RLTICE modified it "continuously".

Fifty-nine rooms: a moated castle, a forest with climbable trees, a belfry with
a vampire bat, a torture chamber, a Troglodyte king, a Tyrannosaurus and a
gypsy who buys your treasures and steals them back.

## What is here

    src_original/   the Pascal source and the two data files it loads
    port/           the C port and a native Windows executable

To play, run `port/tower.exe`.  To rebuild it, run `port/build.bat` (or `make`)
with MinGW gcc on the PATH.

## Recovering the game

As with Ankh, the Pascal is only half the game.  `PROGRAM CHIP1(INPUT,OUTPUT,
ROAD2,LEX2)` loads:

* **LEX2** — nine vocabulary lists.  `INIT_GAME` skips one leading line, then
  reads directions, treasures, movers, props, places, animate, inanimate,
  nouns and verbs, each list ending in `XXX`.
* **ROAD2** — a `FILE OF INTEGER` holding `ROUTES[1..59][1..10]` and then the
  object placements, read `UNTIL K > 150`.  There are 21 real placements
  followed by nineteen zeros and then a `200`, and it is that 200 that stops
  the loop.

Both came off the emulator's PRIMOS disk pack (`p50em/disk26u0.600m`);
`src_original/extract_from_primos.py` documents and repeats the extraction.

The recovered vocabulary cross-checks against every hard-coded subscript in the
source: noun 65 is `WAL`, which is the wall `MOVE_IT` swings open in room 49;
nouns 75 and 76 are `WYS` and `BUS`, the wysteria bushes that hide the hole in
room 3; verb 46 is `STU`, the `STUCK` restart the help text advertises.

## Verification

The port was checked against the original running under the Prime 50-series
emulator (PRIMOS 23.4.Y2K.R1), by driving both with the same input and diffing
the output.  Three sessions of 110, 159 and 141 lines -- the opening room and
the `SUN` wall, scoring, inventory, drowning in the moat, climbing a tree for
the branch and again for the iron key, unlocking the castle door, the `FALL`
trapdoor into the caverns, selling a treasure to the gypsy, letting him keep a
worthless one, and restarting with `STUCK` -- are **identical line for line**,
including the eight-column integer fields PRIMOS Pascal writes in the score
line.

## Original behaviour that was kept

The port is a transliteration, not a repair.  Two of the original's own bugs
are load-bearing enough to call out:

* `MOVE_IT` lists nouns 11..52 and 54 but skips 53, so `MOVE PURPLE` falls
  through to the catch-all while `MOVE AMULET` does not.
* `GAD_ABOUT` pads `bird's nest` to 17 columns where every other object name is
  padded to 18.

Also preserved: `BONUS` is initialised to zero and never touched again, and the
misspellings `moat-encricled`, `Tyrranosaurus` (in one message and correctly in
another) and `THere is a hole under the bushes`.

Unlike Ankh, Tower's `GETCOMMAND` guards its subscripts with
`and (linelength > 0)`, so a blank line here is harmless in the original too.

## Deliberate deviations

**1. The game is made winnable.**  `SCORE` awards `10+Y` for each of the nine
treasures sold, which totals exactly 135, and `BONUS` is always zero -- but the
original then tests `IF SCORE_TOT > 135 THEN WINNER := TRUE`.  135 is reachable
and never exceeded, so in CHIP1 as written `WINNER` is never set and the game
ends only on death or `QUIT`: "of 135" was a target you could meet but not
beat.  This port tests `>= 135`, so selling all nine treasures wins.

What winning looks like is the original's machinery, unchanged.  `WINNER` is
only ever set inside `SCORE`, and the main loop tests it at the end of the
turn, so the win fires when you type `SCORE` holding a full 135: that score
line prints as usual and the program then ends, because `IF DEAD OR QUIT THEN
SCORE` is false and nothing further is written.  There is no victory text
anywhere in CHIP1 to print -- writing one would be invention rather than
restoration, so the ending is quiet.

**2. End of input exits** instead of raising a Pascal run-time error, so the
executable can be driven from a pipe.

The verification transcripts above were captured before this change and remain
valid: none of them reaches 135, and below that score the two builds behave
identically.

## Notes for a player (spoilers)

* Three-letter abbreviations; diagonals are `N-E`, `N-W`, `S-E`, `S-W`.
* You *go* directions but only *move* things.  `CLIMB` goes up.  `STUCK`
  restarts the game from the beginning.
* Score by carrying treasures to the gypsy's clearing and `SELL`ing or
  `GIVE`ing them -- but he steals from you every thirtieth turn everywhere
  else, and anything you merely drop in his clearing he keeps for nothing.
  Sell all nine and type `SCORE` to win (see Deliberate deviations).
* `SAY SUN` opens the east wall of the starting room.
* You cannot swim.  Take the old branch from a treetop and drop it in the moat
  before you cross, or you drown in ten turns.
* The iron key is in another treetop; `UNLOCK DOOR` at the moat gets you in.
* `SAY FALL` in the entrance chamber drops you into the caverns.  `SAY T42` in
  the tea parlor conjures the teapot.  `SAY HERE` / `SAY THEN` and friends are
  read off the cavern walls -- and you need the parchment scroll in hand before
  the Troglodytic writing means anything.
* The dead rat feeds the Tyrannosaurus; ice keeps the rat, and the tongs carry
  the ice.  Glass beads buy rough diamonds from the Troglodyte King.  Carry the
  copper cross to the gravesite, and to the belfry unless you like vampires.
