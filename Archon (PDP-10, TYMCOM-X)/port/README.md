# ARCHON — a Windows port

*The Vale of ARCHON*, a large text adventure written in FORTRAN-10 for the
DECsystem-10, recovered from the pair of files in `..\dump_original\`:

```
archon.shr    85,455 bytes    shareable high segment (code + FORTRAN runtime)
archon.low   192,430 bytes    low segment (world data, in TOPS-10 .SAV format)
```

The tape also carries `blocke.low`, a second low segment for the same
build. It is not another version: compared by address, the two differ in
407 words out of 38,000 and every one of them is runtime state — object
positions, object states, the clock block, scratch variables. Not one
word of the messages, travel tables, zones, conditions, vocabulary or
initial placements differs. `archon.low` is the distribution copy, saved
at state 2 with nothing carried; `blocke.low` is somebody's suspended game,
saved at state −1 with four objects in hand. This port uses the former.

There is no source. The port runs the original 1975-era binary on a PDP-10
emulator with just enough of the TOPS-10 monitor underneath it, so what you
play is the actual program, instruction for instruction — not a rewrite.

See [docs/REVERSE-ENGINEERING.md](docs/REVERSE-ENGINEERING.md) for how the
files were decoded, including the two 36-bit words that had been lost in
transfer and how they were identified and restored.

## Build

Needs a C compiler with 128-bit integer support (gcc or clang; MinGW-w64 is
fine) and Python 3 to regenerate the embedded core image.

```bash
make
```

That produces a standalone `bin\archon.exe` — the core image is compiled in,
so the executable needs nothing else at runtime.

## Play

```bash
bin\archon.exe
```

Starting a game runs the program's own world reset, which opens with the
table-load report the author left in — it is the game telling you how big it
is — and then a `Type G to Continue` checkpoint. The port answers that one
prompt for you, so the first thing you type goes to the game:

```
 Table space used:
 23839 of  25999 words of messages
   504 of    579 locations
   ...
Type G to Continue, X to Exit, T To trace.
*Welcome to the Vale of ARCHON!!  Would you like instructions?

You have 5 gold pieces!
There is a man here, with the face of a weird hampster.
You are standing at the intersection of two roads, surrounded by a
number of decayed mud buildings.  To the north, the road dips down
to a dried watercourse (wadi), and then disappears into a gorge...
```

504 locations, 129 objects, a 509-word vocabulary and about 24,000 words of
prose. Type `INFO` for the rules, `HELP` for hints about the parser, `HITS`
for how combat is resolved, and `SCORE` to see how you are doing. Only the
first five letters of a word matter, so `NORTHEAST` has to be typed `NE`.

### Suspending

The game has no save file. On TOPS-10 you suspended it the way you suspended
anything else: you typed `SUSPEND`, the game told you to break out and save
your core image, and you did. This port keeps that model —

```bash
bin\archon.exe            # play, type SUSPEND, answer yes
bin\archon.exe -c         # resume exactly where you stopped
```

— and writes the image to `archon.core` at the moment the game asks for it.
The game refuses to resume an exploration less than 90 minutes old, which was
deliberate on the author's part; `-t HH:MM` tells it a later time if you would
rather not wait.

### Hours, the turn limit, and the wait to resume

ARCHON was a program on a commercial timesharing service, and it
polices the clock the way such a program had to. Two of its rules
still bite a player today:

* **A turn limit.** Run past your allowance -- which the game itself
  extends as you get on -- and the dungeon master appears in green
  smoke to declare that *"THIS EXPLORATION HAS LASTED TOO LONG"*.
* **90 minutes** before a suspended exploration may be resumed.

A third, the hours ARCHON is open, is in there too but inert: the
three prime-time masks in the copy that survived are all zero, so the
game is open all day, every day. A dungeon master who sets hours can
shut it, and after that visitors get a short exploration or nothing.

`-u` disarms all three. The game runs as long as you like, and
`archon -c -u` picks a suspension back up the moment you make it.
Nothing else changes: without `-u` the program behaves exactly as it
always did, and `-t` still works if you would rather lie about the
time than lift the rules.

What `-u` writes to is the game's own core: the hour masks at
`110451`-`110453` and the restart latency at `110464` go to zero, and
the two branches at `403733` and `403736` that act on the turn counter
become `JFCL 0,0`. Patching the branches rather than the counts is what
makes it work on a game resumed from a core image, where the counts are
already set. The saved image stays honest: the originals go back before
`SUSPEND` writes `archon.core`, so a game saved under `-u` is an
ordinary saved game and continuing it wants `-u` again.

### LEAKAGE, and the five planes

The Vale is five parallel planes, and every location carries a plane bit
in the array at `110503` — 149, 90, 90, 43 and 115 rooms. A travel entry
is `kind*10^8 + condition*10^4 + destination`, and the ten-millions digit
on top of that is a **critter flag**: the wandering-monster mover divides
the entry by 10,000,000 and refuses the exit if the quotient is 1, while
your own travel decoder divides by 10,000 and then by 1,000 and keeps
only the remainder. The digit is invisible to you and opaque to the
wildlife — that is how the author kept the doorways between planes open
to the player and shut to everything else.

He left a check behind it. Having accepted an exit for a critter, the
mover compares the plane bit of the room it is standing in against the
plane bit of the destination, and if they differ it prints

```
LEAKAGE...CRITTER #nnn got from LOCnnnn to nnnnn!!
```

with a `BEL` either side of it, to ring the terminal. It is his own
alarm, compiled into the shipped binary, and it goes off on his own map
data. Of the 3,456 travel entries, 645 cross planes and 640 carry the
flag. Five do not, in two places, and both read as slips:

| | |
|---|---|
| `LOC 66 -> 264` | the plane 1 / plane 3 doorway. All four entries coming back the other way, `264 -> 66`, are written `10000066`. The one going out is a bare `264`. |
| `LOC 383 -> 1` | six exits, all to the same room. Two are written `10000001`. The other four are a bare `1`. |

Location 66 is the morgue behind the police station — where the game
puts you when it reincarnates you — so this is not an obscure corner of
the map. Anything that follows you there, and the werehamster follows you
everywhere, stands on the one exit the author forgot to flag and rings
the bell. In a 150-turn game two different critters did it.

`-m` gives those five entries the digit their own siblings already carry.
It states the rule rather than the five addresses: any exit a critter
could take into a room with a different plane bit. `-v` reports how many
it changed, which should be five. It cannot affect your own travel,
because your travel never reads that digit — a scripted game played twice
with and without `-m` came out byte-for-byte identical apart from the
missing LEAKAGE lines. As with `-u`, the saved image stays honest: the
original entries go back before `SUSPEND` writes `archon.core`.

### Why the distribution copy matters

The two low segments on the tape are not interchangeable, and the
difference is worth knowing because it is invisible until you go shopping.

The program's new-game path puts every object back where it started, from
the initial-placement tables at `113561`/`116734` that survive in the image,
but it never touches `PROP` — an object's *state*. The only code in the
program that ever initialises `PROP` is the `ARCHON.DAT` reader. So
positions come back and states do not: drop the shovel two rooms away and
force a new game and the shovel returns; spend a gold piece and it stays
spent.

Build on `blocke.low` and you therefore inherit the last player's shopping
trip, because he was carrying four things when he suspended — a lamp, a
shovel, a leather jacket, and his money. In the shop `PROP < 0` means "on
the shelf, unsold", and such an object is not described at all; that is
what the sign on the wall is for. `BUY` refuses anything at `PROP >= 0`
with *"You already own it!"*, so his three purchases lie there loose,
unbuyable and free to take, and his change is what you start with while
the opening line still promises five gold pieces.

`archon.low` has none of that. The stock is on the shelf, the purse is
full, the merchant greets you with *"Anything in the place, half price (in
gold)!"* — a line the other image never shows, because his own `PROP` was
mid-game too — and `BUY` works as written:

```
> buy shovel
OK
The merchant places it on the counter, saying what a fine deal
you've made.
You have 4 gold pieces left!
There is a fine shovel here.
```

The picks stay sold out on both. The pick is object 56 and it starts at
location 187, out in the world — the one line on the sign with no stock
behind it, and *"I'm sorry, the last delver through here bought the last
one"* is the author's own joke.

### Options

```
  -c, --continue      resume a suspended exploration
  -f FILE             use FILE instead of archon.core
  -t HH:MM            tell the game it is HH:MM
  -q, --no-delay      skip the pauses the game asks for during combat
  -u, --unlimited     ignore ARCHON's hours, the turn limit and the wait
                      before a suspended game may resume
  -m, --fix-map       stop critters straying between the Vale's five planes,
                      which is what LEAKAGE complains about
      --tables        enter at the world-reset instead, so the program prints
                      its own table-space report first
  -v, -vv, -vvv       report monitor calls in increasing detail
  -T                  trace every instruction
  -w ADDR             report every change to that octal core address
```

`archon.low` holds the program's state word at 2 — "tables built, greet the
player and start playing" — which is what the author's maintenance mode leaves
behind when it says *"OKAY.  YOU CAN SAVE THIS VERSION NOW."* A distribution
copy is ready to play as it stands, so the port sets nothing and the game
opens on its greeting. `--tables` winds the word back to 1 instead, which
makes the program print its own table-space report on the way in; the reset
that follows has nothing to undo, and the "Type G to Continue" checkpoint
after it is answered for you.

## Layout

```
src/pdp10.h      machine definitions
src/cpu.c        the processor: instruction set, floating point, byte pointers
src/monitor.c    the TOPS-10 side: UUOs, terminal, buffered device I/O
src/main.c       driver, core-image save and restore
src/image.c      generated — the core image as C data
tools/mkimage.py builds src/image.c from ..\dump_original\, applying the two repairs
tools/dumptext.py dumps the game's message table
docs/            the write-up, and the extracted message corpus
```

## Known limits

* The game can build its world tables from a file called `ARCHON.DAT`, which
  was not on the tape. It does not need to: the tables are already built inside
  the core image, which is how the game was distributed. Only the from-scratch
  build path is unreachable.
* Play is deterministic. The random number generator is seeded from the image
  and nothing observed reseeds it from the clock, so a fresh game opens the same
  way every time; it diverges as soon as your choices do.
* Two instructions the program never executes, `ADJBP` and `EXTEND`, are not
  implemented and will report themselves if they are ever reached.
