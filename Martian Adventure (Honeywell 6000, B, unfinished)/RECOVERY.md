# Martian Adventure — what is in the archive, and what runs

Waterloo, Honeywell 6000 (GCOS / Waterloo timesharing), 1978–79.
Brad Templeton (`bstempleton`), with the adventure definition language by
Mark Niemiec.

**Short answer to "can it be recovered into something playable?": partly, and
the part that plays is real.** You can walk the Martian desert from your ship
to the Viking I lander, round the boulder to the WELCOME sign, down the airlock
and into the domed city — 76 rooms, every word of it out of the 1978 files. You
can also play the Math building, which is written in Mark's adventure language
and still has working puzzle logic. What you cannot do is play *the game Brad
describes*: the well house, the keyboard, the rocket, the treasures and the
whole object system are not in the archive, and about half of them were never
written in the first place.

---

## 1. What the archive actually holds

`hw.txt` is the backup listing. Under `jmc/mars` it names exactly the files we
have, and nothing else:

| file | bytes | dated | what it is |
|---|---|---|---|
| `game.v1.txt` | 5106 | 8 May 1978 | the game, in **B**, breaking off mid-verb |
| `game.v2.txt` | 5108 | 16 Sep 1978 | same, two edits later |
| `exter` | 172 | 8 May 1978 | the `extrn` list `game` includes with `%jmc/mars/exter` |
| `goto` | 2001 | 8 May 1978 | the exit table — 79 rooms |
| `snames` | 3010 | 8 May 1978 | short room names — 80 rooms |
| `lnames` | 5136 | 8 May 1978 | long descriptions — 15 rooms |
| `indat.v1.txt` | 7221 | 8 May 1978 | long descriptions, second region — 36 rooms |
| `indat.v2.txt` | 11675 | 16 Sep 1978 | same region grown to 63 rooms |
| `math.v1.txt` | 806 | 19 Mar 1979 | the Math building, 2 rooms, in the **adventure language** |
| `math.v2.txt` | 6037 | 10 May 1979 | same, grown to 15 rooms |
| `polar/indat.txt` | 11675 | 24 Mar 1979 | byte-for-byte copy of `indat.v2.txt` |
| `polar/lnames.txt` | 5136 | 24 Mar 1979 | byte-for-byte copy of `lnames` |

Plus `marsgame.b` at the top level: a printout of `game.v2.txt` with trailing
spaces on every line and a `bstempleton.` banner — the same 5108 bytes, no more.

Every one of these files is byte-exact against the sizes in `hw.txt`, with
Honeywell newlines intact. Nothing was mangled in transit. `src_original/` holds
them unchanged.

### What is missing

* `jmc/mars/desc` and `jmc/mars/rooms` — the two compiled binaries `main()`
  opens. These were built from the text files above by a tool that is also gone.
* `jmc/mars/expl` — the instructions the banner points you at.
* **The entire object system.** `game.b` uses `treasures[]` (three words per
  item, indexed up to at least 300), `trehere` (a per-room object file),
  `scant()`, `mtno`, `foodnum`, `magn`, `hints[]`, `dict`. No vocabulary file,
  no treasure table and no per-room object file is in the archive.
* **The connection table for the `indat` region.** 63 rooms of description with
  nothing to say how they join.
* **The second half of `game.b`**, from the middle of `case'kill'` onward:
  `quit`, scoring, the endgame, `groom()`, `prlon()`, `geta()`, `scant()`,
  `any()`, `rbot()`, and the `spmove` special-movement routine.
* **The adventure language compiler.** `math.v2.txt` is the only program in
  that language anywhere in the archive; the compiler that ate it is not here.
* Everything Brad remembers from the opening: the well house, the keys on the
  keyboard, the rocket, the Viking landing sequence, the ray gun, the artifacts.

That last point is worth being precise about. The archive is a *subset of a
subset* — files people asked to have backed up — so absence is not proof that
something never existed. But `game.v2.txt` ends mid-statement with a closing
brace the author added himself, and 52 of the 80 named rooms have no long
description written for them. The files are consistent with exactly what Brad
says: he never finished it.

---

## 2. The two engines

There are **two different systems** here, a year apart, and it is worth not
conflating them.

**1978 — Brad's own engine, in B.** `game.v1/v2.txt` is hand-written B: one
enormous `switch` on a four-character command word, reading two compiled binary
files. This is the engine behind `goto`/`snames`/`lnames`/`indat`. It is the
one that was never finished.

**1979 — Mark's adventure language.** `math.v1/v2.txt` is not B at all. It is a
declarative world description:

```
m1stfloor : {
        long : "You are in an empty hallway at the base of a ramp. ...";
        brief:"You are on the first floor of the math building";
        open : "You can not open any of the doors";
        w : if ( .roomdoor == OPEN ) redroom;
                else "The door into the large red room is locked";
        e : ioroom;
        u : out : s : mentrance;
}
```

Rooms are blocks; `long`/`brief` are the two description levels; a list of words
before a colon binds them all to one action; an action is a room to move to, a
string to print, a `{ }` block, or an `if`. There are variables (`.roomdoor`,
`onmidjet`), constants (`OPEN`, `YES`), a `` ` `` prompt form, `getstr` and
`getarg(0)` for asking the player a question. That is Mark's compiler input,
and it matches Brad's description of it exactly.

The dates say Brad moved from his own B engine to Mark's language during the
first half of 1979. `math.v2.txt` (10 May 1979) is the newest game file in the
archive.

---

## 3. The Mars map, reconstructed

`goto` is a complete, honest exit table. Its format is a three-digit flag word,
a room number, then direction/destination pairs:

```
000 006N14 NE14 E5 SE17 S16 SW16 W15 NW15 D41
```

The direction slots line up with the fall-through chain in `game.b`, which
computes an index `po` and then does `q = char(rvec, po)`:

| po | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|---|---|---|---|---|---|---|---|---|---|---|
| dir | N | NE | E | SE | S | SW | W | NW | U | D |

The flag word's leading digit is the "print the short name, not the long one"
bit that `main()` tests as `roombits[(r-1)/36] ... &01` — 36 bits to a word,
which is the Honeywell 6000 word size. It is set for exactly the 13 rooms that
have no `lnames` entry and were never meant to have one (the featureless desert,
the ramps). The trailing digit marks the 51 rooms inside the domed city.

### Reachability

Starting from room 256 — the ship, which has no description, only a Down exit —
**76 of the 80 named rooms are reachable**, plus the ship itself and room 40,
which the map points at but which has no data of any kind. The spine is exactly
what Brad remembers:

```
lamp            (see §5)
d               out of the ship onto the desert          room 1
w w w           to the Viking I lander                   room 4
w               the boulder, with the four-inch hole      room 5
w               the far side: WELCOME sign, airlock hatch room 6
d d d           down the airlock shaft                    rooms 41-43
d               the bubble over the domed city            room 44
d d             down the Tower to the plaza               rooms 45-46
n ne e se s ...  three concentric rings of radials        rooms 53-100
```

Side trips: `e e e` from the ship reaches the diamond well; `n n n` reaches the
rim of the Valles Marineris.

Brad's memory checks out against the data on every point:

* Room 4 is the toppled **Viking I** lander (he says "Voyager" once and "Viking"
  twice; the file says Viking I).
* Rooms 5 and 6 have the boulder with a hole "almost exactly four inches wide,
  perfectly straight and parallel to the ground … it sights directly towards
  your ship" — Bottomos, from Sheckley's *The Holes Around Mars*.
* Room 6 has the flashing sign, `"GREETINGS---THE SENTIENT LIFE FORMS OF MARS
  WELCOME YOU"`, and the airlock hatch, on the side of the rock you cannot see
  from the lander.
* The city is unfinished: 52 of the 80 rooms are flagged as having a long
  description and none was ever written.

### Holes in the map

* Rooms **10–13** — the broad white highway running south to the great Dome —
  have exits *out* but no room anywhere has an exit *into* them. They are
  orphaned, not lost: `goto` is complete for them, nothing points at them.
* Rooms **51 and 52** (the Northern and Southern Apartments) have names and are
  reachable, but have no exits at all. One-way.
* Room **40** is the Down exit from the plaza (room 46, the city ground level)
  and has no entry in `goto`, `snames` or `lnames`. Rooms 21–39 are absent too.
  Whatever was under the plaza was never written.

---

## 4. The `indat` region — descriptions, no map

63 long descriptions: the Martian underground (the hall of the Old Ones, the
offices of *Modern Martian* magazine, King Mwa's corridors, Gmog's vault) and
the interior of **Deimos** (the picture window over Mars, the airlock, the
landing field, the chemical lab, the church complex, the hall of
discorporation). This is the "teleport to Deimos" half of the game Brad
describes, and the writing in it is the best in the archive.

There is no exit table for it. Building one from the prose would be writing the
game rather than recovering it, so the port presents these as a reader — you can
page through all 63 rooms — and does not pretend they connect.

### Five lines put back

`indat.v2.txt` (Sep 1978) is a re-cased, expanded rewrite of `indat.v1.txt` (May
1978). In the process **five whole lines went missing** from rooms that v1 still
has intact:

| room | line restored |
|---|---|
| 16 | *where there is a breathtaking sight.  Out of the window are the stars, and* |
| 18 | *stars is breathtaking, and you pause for a breath.  Mother Earth can be seen* |
| 24 | *has a northeast branch that goes on for a great distance* |
| 35 | *You are at the bend in an angled passage.  Branches lead* |
| 36 | *The vast crimson face of Mars turns slowly past, bringing a ruddy tinge to* |

`gendata.py` puts these back by aligning v1 and v2 case-insensitively. The
*wording* is the original's; the *capitalisation* is mine, applied by the same
rule the author used by hand (lowercase everything; capitalise at the start of a
room, after a line that ended a sentence, and after `.!?` followed by **two**
spaces but not one — rooms 27, 28 and 31 show the single-space case staying
lowercase; keep the proper nouns). Rooms carrying a restored line say so when
you read them.

Room 4 is deliberately **not** restored: there v1 carries two extra lines of a
superseded rewording that the author removed on purpose when making v2.

Rooms 37–63 exist only in v2, so if lines were dropped there, nothing survives
to compare against.

---

## 5. Faithfulness notes on the B engine

The port is a transliteration, not a tidy-up. By default it reproduces the
source's bugs; **`-fix` repairs all four** (and the two in the Math building,
section 6). Nothing else about the port changes between the two modes.

* **The game starts in the dark.** `light` is an undeclared extern, so it is 0,
  and the first thing the game says on a sunlit Martian desert is *"It is now
  quite dark, should you proceed, you may die"*. Type `lamp`. (`-light` starts
  it on without the other repairs; `-fix` implies it.)
* **The lamp verb is self-contradictory.** Read it: after the argument
  handling there is an unconditional `if(light) printf("It already was"); else
  light=1;`, so the lamp always ends up on, and `lamp off` turns it on and then
  off again. Transliterated literally, dangling elses and all. It also prints
  `Your light is nowon`, because the format string is missing a space. Under
  `-fix`, `lamp` toggles, `lamp on`/`lamp off` do what they say, and the space
  is there.
* **`south` does nothing.** The source has `case's'` but no `case'sout'`, while
  north has both `case'n'` and `case'nort'`. Commands are matched on four
  characters because `geta()` returned one 36-bit word — four 9-bit characters.
  `-fix` supplies the missing case.
* **`read` falls through into the teleport verb.** There is no `goto` or `break`
  at the end of `case'read'`, so with the vocabulary table gone — `scant()` can
  only return 0, and `no.things` is 0, so the `cn > no.things` guard never
  fires — reading anything prints *"I can't read that, it's all Old High
  Martian to me!"* and then immediately *"You are not carrying the
  transmitter"*. `-fix` adds the missing `goto`.

Reproduced in both modes, because they are absence rather than bugs:

* **Unknown words are silent.** There is no default case, so a word the
  truncated source never handled just returns you to the prompt. The port says
  so with a `--` note rather than leaving you guessing.
* **`if(!light&&!rand()&07)` cannot fire.** It parses as
  `(!light) && ((!rand()) & 07)`, so you die in the dark only when `rand()`
  returns exactly zero. The intent was plainly `!(rand()&07)`, a one-in-eight
  chance. Left as written — changing it would change the game's difficulty,
  which is not the same kind of repair as the four above.
* Similarly `switch(rand()<<18 &0777)` in the teleport verb masks off the bits
  it just shifted away. It is unreachable anyway — you can never be carrying the
  transmitter, because there are no objects.
* `take`, `drop` and `inventory` run and answer, on an empty world. `read dict`
  answers *"You aren't carrying it"*, with the original's missing newline.

Everything the port says on its own account starts with `--`, and every command
it adds starts with `*` (`*map`, `*exits`, `*goto N`, `*back`, `*notes`,
`*quit`). `quit` also works, because the banner promises it, even though its
case is in the lost half of the source.

---

## 6. The Math building, and two typos that break it

`math.v2.txt` is 15 rooms of the University of Waterloo Mathematics and
Computer building, transplanted to Mars: the red room full of ancient machines
behind glass, the **VM** room ("A sign on the wall says 'Very Martian'" — "The
unit limit on your VM account has run out long ago"), **MIDJET** (the *Martian*
Interactive Debug Job Entry Terminal, after Waterloo's real WIDJET), the CSC tea
room, MARSOC with its stale donuts, and a sixth-floor terminal room labelled
`unix`.

It has a real puzzle. Log on at MIDJET, crash it, and a robot comes out of the
red room to the `11/45 console` — leaving the red room door open behind it.

**As written it cannot be solved.** Two typos, both from renaming WIDJET to
MIDJET and missing a spot:

1. `m2ndfoyer` says `w : nw : widjet;` but the room is called `midjet`. You can
   never get there.
2. `logon` sets `onmidjet = YES`; `crash` tests `if ( onwidjet == YES )`. Even
   standing in the room, the game says you are not signed on.

The port leaves both alone by default. `-fix` repairs them (and aliases `up`/`u`
and `d`/`down`, which the source also uses inconsistently), and then the puzzle
runs end to end:

```
up  w  logon  crash  no  s  d  d  w  n
```

Answering `no` to *"Are you a hack? "* is what crashes the system —
`if (getarg(0) != yes)` — which reads like the joke and may also be an inverted
test. Left as written.

The rooms `science`, `village1` and `widjet` are named as exits but never
defined; the port says so instead of pretending.

---

## 7. Was it finished later by someone else?

Nothing in this archive can answer that, and it is worth saying plainly rather
than guessing. What the files do establish:

* As of **16 September 1978** the B engine was unfinished and the second region
  had 63 described rooms and no map.
* By **10 May 1979** Brad had abandoned the B engine and was writing in Mark's
  adventure language instead — the Math building is that work, and it is itself
  only 15 rooms with three undefined exits.
* The last file in the whole `jmc` home directory is dated **30 November 1980**,
  and by then nothing under `jmc/mars` had been touched in eighteen months.

If a version was being played to completion on Honeywell sites around 1984, it
was finished from something later than, or parallel to, what is here. The
recognisable landmarks a player would remember — the Viking lander, the boulder,
the welcome sign, the underground city — are all present in these 1978 files, so
a later completion could well have been built on exactly this material.

---

## 8. Building and running

```
cd port
sh build.sh          # or: python gendata.py && gcc -O2 -o mars.exe mars.c
./mars.exe           # faithful: every 1978 bug reproduced
./mars.exe -fix      # repair the authors' bugs (sections 5 and 6)
./mars.exe -light    # only the dark start repaired
```

`gendata.py` is the missing compiler: it reads `src_original/mars/*` and emits
`data.h`. It prints a summary of everything it found and everything it could not
find. `mars.c` is the part the compiler used to emit — the transliterated B
program, plus the interpreter for Mark's language.

No dependencies beyond a C compiler and Python 3.

```
port/
  gendata.py   the compiler:   archive text -> data.h
  data.h       generated; do not edit
  mars.c       the engine:     B transliteration + adventure-language interpreter
  build.sh
  mars.exe
src_original/  the archive, byte for byte, untouched
```
