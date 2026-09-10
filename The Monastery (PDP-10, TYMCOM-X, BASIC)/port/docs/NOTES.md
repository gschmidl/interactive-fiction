# The Monastery — source notes

## What was recovered

Two files from the `novafield` Tymshare tape:

| file | size | dated | what it is |
|------|------|-------|------------|
| `save.tba` | 81,160 | 8 Jan 1987 | the complete BASIC source of the game engine |
| `text.gme` | 11,397 | 9 Jan 1987 | 94 numbered text entries — rooms, objects, messages |

`save.tba` ends with four bytes of tape padding (`00 00 7F 6C 69 76 04`) after the
last `DATA` line; everything before that is clean ASCII with CRLF line endings.

Nothing else on the tape references this game. In particular there is **no
character file and no character generator for it** — see below.

The game has no title of its own anywhere in the source. "The Monastery" is a
working title taken from the setting.

## The dialect

The source is BASIC for a **PDP-10 running Tymshare's TYMCOM-X**. The evidence:

* `pub.tba`, a sibling program on the same tape, declares externals in
  Tymshare's own SIMPL language and calls `XWD`, `RH`, `LH`, `RUNUUO` and
  `JOBNO` — PDP-10 half-word and UUO primitives.
* Files are opened `OPEN "TEXT.GME-A" FOR SYMBOLIC IO AS FILE 1`; the `-A`
  is a file-mode suffix, not part of the name (the file on tape is plain
  `text.gme`).
* `ERRCD` / `ERRLN` error variables, `CHARTABLE(n)`, `CIB` (clear input
  buffer), `LOL` (line length).

Language features the program uses that the port has to reproduce:

| construct | meaning |
|-----------|---------|
| `PRINT a:b:c` | `:` is the concatenating separator; a trailing `:` suppresses the newline |
| `DEFINE NAME` … `ENDF NAME`, `CALL NAME` | named subroutines; no parameters, no locals — everything is global |
| `stmt UNLESS cond`, `stmt FOR I=1 TO N` | postfix statement modifiers |
| `INPUT IN FORM "X":I$` | read exactly one character |
| `INPUT FROM 1 IN FORM "R":A$` | read one record (line) from a file |
| `INPUT FROM 1 IN FORM "DDDDD":X` | read a fixed-width 5-digit number |
| `MAT READ A`, `MAT A=" "`, `MAT A=B` | whole-array read / fill / copy |
| `ON ENDFILE(1) GOTO n` | end-of-file trap |
| `END "message"` | print and stop |

All numeric variables are floating point. Two expressions in the combat code
divide without an `INT()` wrapper (`CHARACTER(58)/10` in `KILL.SUB`,
`CHARACTER(1)/2` and `CHARACTER(7)/2` in `CROLL.SUB`), so the port keeps `A`,
`TH` and `DAM` as `double`.

## Data layout

`save.tba` carries 1,476 `DATA` items, consumed in exactly this order with
nothing left over — which is the best confirmation available that the source
survived intact:

| items | array | shape |
|-------|-------|-------|
| 500 | `MOV` | 50 rooms × 10 directions |
| 66 | `VERB$` | verb vocabulary |
| 10 | `DIR$` | direction names |
| 66 | `VCODE` | verb → action code |
| 44 | `ADJECT$` | adjectives |
| 7 | `PREP$` | prepositions |
| 5 | `ADVERB$` | adverbs (parsed, never acted on) |
| 20 × (2 + 24) | `NOUN$`, `NOUN2$`, `NOUN` | objects and monsters |
| 150 | `LOCS(*,7..9)` | day / night / brief description index per room |
| 108 | `DOORS` | 18 doors × 6 fields |

### `NOUN(i,j)` fields

| j | meaning |
|---|---------|
| 1 | location: room number, `-1` carried, `-2` scenery, `-n` carried by monster *n* |
| 2 | takeable |
| 3 | is a monster |
| 4 | weapon damage die / number of monster attacks |
| 5 | container state (0 none, 1 open, 2 closed); for monsters, a defence rating |
| 6, 7 | adjective indices |
| 8 | container capacity; for monsters, hit points |
| 9 | containment: 0 loose, 1 left hand, 2 right hand, 3 worn, ≥4 inside noun *n* |
| 10 | monster damage die |
| 11 | armour size class (1 human, 2 elf, 3/4 hobbit & dwarf, 5 orc); on a light source, 1 = lit |
| 12 | scratch — previous location, used by `HOBBIT.SUB` |
| 13 | weight; for monsters, the noun index of the wielded weapon |
| 14 | kind: 1 armour, 2 sword/belt, 3 pouch, 5 light source, 6/7 undead, 9 backpack or demi-human |
| 15 | bulk; for monsters, aggression |
| 16 | unused |
| 17 | `TEXT.GME` entry printed when the object is in the room |
| 18 | stun countdown; 1 also marks a corpse |
| 19 | condition: 1 stunned, 2 unconscious, 3 paralysed |
| 20 | hold value; on armour, the armour-class bonus |
| 21 | engaged-in-combat flag |
| 22, 23 | monster strength, dexterity |
| 24 | monster skill |

### `CHARACTER(n)`

Deduced from how `SAVE.TBA` itself uses the array (see `DISPLAY` at line 19450).

| n | meaning |
|---|---------|
| 1–7 | strength, dexterity, personality, endurance, intelligence, mana, luck |
| 8, 9 | current hits, maximum hits |
| 10 | gold |
| 58–62 | fighting, thieving, trading, magic, mechanics |
| 63 | a combat skill used by `CROLL.SUB` |
| 64, 65 | noun held in left hand, right hand |
| 66 | incapacity countdown |
| 67 | 1 stunned, 2 unconscious, 3 paralysed, 5 helpless |
| 68 | handedness: 64 left, 65 right |
| 69 | the monster currently being fought |
| 70 | the hold the monster has on you |
| 71 | armour class, recomputed every turn |

`CHARAC$(1..6)` is name, class, alignment, race, password, rank. The order
follows from `DISPLAY` printing `CHARAC$(1) CHARAC$(6) CHARAC$(4)-CHARAC$(2)`
and from `WEAR.SUB` testing `CHARAC$(4)` against `ELF` / `HUMAN` / `HOBBIT` /
`DWARF` / `ORC`.

### The missing character file

`ADDPLAYER` (line 8870) asks for a name, opens `NAME.GME`, and reads six
strings followed by 82 five-digit numbers. No such file is on the tape.

`charc.tba` on the same tape is a character generator, but it writes a
*different* layout (32 numbers, then seven `@`-terminated strings) to
`NAME.ADV` for a different game. It is kept in `src_original/` for reference
only; **none of its values were carried across**. The port's roll-up is new
work — see the README.

## Quirks reproduced verbatim

The program was still under construction when the tape was written. These are
all faithfully preserved in the port; each is marked `[original bug]` in
`monastery.c`.

1. **Door 1 is self-referential.** `DATA 2,1,2,4,0,0` names room 2 on both
   sides. Opening the north door in room 2 writes `MOV(2,1)=2` and
   `MOV(2,4)=2`, turning the entrance hall into a self-loop you cannot leave.
   Almost certainly a typo for `2,1,3,4`.
2. **The game is never dark.** `LIGHT.MODE` lines 22080/22085 test `L`, which
   was set to 0 two lines earlier; the author meant `N`, the room number.
3. **The lamp can never be lit.** `LIGHT.IT` exists but no `VCODE` dispatches
   to it. Given (2), that is just as well.
4. **`QUIT` does nothing.** `QUIT` is verb 35 with `VCODE` 21, and nothing
   dispatches 21. In 1987 the only ways out were dying or hanging up. The port
   wires it up; `--strict` restores the original behaviour.
5. **`DROP` has no missing-noun guard.** Lines 5700–5740 sit *above*
   `5750 DEFINE DROP.SUB`, so they are unreachable.
6. **`GET.SUB` line 5500 reads `CHAR(65)`**, a variable that is never
   assigned, so the "in your right hand" message can never appear.
7. **`KILL.SUB` computes damage and throws it away.** Line 14930 subtracts a
   flat one hit point per landed blow, no matter what `DAM` came to. Armed
   combat is therefore far slower than punching or wrestling.
8. **Armour class is inverted.** Line 17610 is `AC=AC-CHARACTER(2)-8` where
   `AC-(CHARACTER(2)-8)` was meant, so a dexterous character ends up with an
   absurdly good armour class.
9. **`PUT.HAND` lines 17050/17060 are dead.** `A` has already been remapped
   from 31/32 to 64/65 further up, so `NOUN(...,9)` is never updated.
10. **The combat parser sends `LOOK` to `CLOSE.SUB`.** Line 14060 dispatches
    `VCODE` 28 (which is `LOOK`) to the close routine; 27 is `CLOSE`. In
    combat, `LOOK` answers *I DO NOT RECOGNIZE A NOUN IN THAT SENTENCE.*
11. **Searching the ashes never finds the book.** Line 6590 requires
    `NOUN(11,1)=4`, but the `ASHES` noun sits at location 0. `GET BOOK` in
    room 4 does work, and prints the "you rummage about the ashes" text — if
    the wandering hobbit, who starts in that room, has not taken it first.
12. **`TAKE INVENTORY` always ends with *I DON'T UNDERSTAND THAT.*** The
    special case at line 910 jumps to 2210 with `PRS(1)` still zero. Plain
    `INVENTORY` is clean.
13. **Room 46 runs off the map.** North and north-west from room 46 lead to
    room 52, in a 50-room table. The 1987 interpreter would have stopped with
    a subscript error; the port treats those two exits as walls.
14. **Night descriptions are never used.** `LOCS(*,8)` is read from `DATA` and
    never selected — there is no clock in the program.
15. **Two wall-search messages read the wrong slots.** `SEARCH.DOOR1` line
    6890 tests `PRS(3)`/`PRS(4)` but prints `ADJECT$(PRS(7))`/`ADJECT$(PRS(8))`;
    `SEARCH.DOOR2` line 7310 mixes `PRS(3)` and `PRS(8)`.
16. **`MOVE` cannot see what is in front of it.** Line 12380 reads
    `IF NOUN(PRS(5),1)=P AND NOUN(PRS(5),1)<>-1`, where `<>P` was meant, so
    moving an object that really is in the room answers *I DON'T SEE ONE
    HERE.* Only the cathedral statue, which is scenery at location `-2` and
    takes the branch above, can actually be moved.
17. **Walking into a `-1` wall.** `-1` exits print nothing the first time; on
    a later attempt the stale `Z` left over from the previous failed move
    makes the room description print again.

Rooms 11, 12, 13, 15, 17, 35 and 36 have a description index of 0 — laid out
in the movement table but never written. Several verbs (`EAT`, `DRINK`,
`BREAK`, `SNEAK`, `RUN`, `WALK`, `HELP`, and others) parse but have no
handler. The five adverbs are recognised and stored in `PRS(10)` but only ever
used to let `PICK UP` / `PUT DOWN` be preceded by one.

## The parser

A fixed-slot template, filled left to right:

```
[adverb] VERB [prep] [adj] [adj] NOUN [prep] [adj] [adj] NOUN
```

stored in `PRS(1..10)`. `THE`, `GO` and `A` are dropped; `IT` copies the
previous sentence's noun and adjectives. A comma splits the input into two
commands, the second re-parsed on the next pass through the word-list search.

`PICK UP`, `PUT DOWN` and `TAKE INVENTORY` are special-cased before the verb
table is consulted.

The secret-door search needs the full template, either way round:

```
SEARCH FOR SECRET DOOR ON THE NORTH WALL
SEARCH THE NORTH WALL FOR A SECRET DOOR
```

## "SAIRE SUBDIR"

Text entries 71 and 72 describe a hidden wooden room — room 10, reached by
moving the statue in the cathedral — with the words `SAIRE SUBDIR` written on
the north wall in oddly coloured ink. Nothing in the program acts on those
words: they are not in the vocabulary, no flag watches for them, and room 10
has no exits of its own. Either the payoff was never written, or the phrase
was a note to the author's own file system. It is the last thing in the game.
