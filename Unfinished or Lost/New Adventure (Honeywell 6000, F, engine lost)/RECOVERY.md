# New Adventure — Mark D. Niemiec, University of Waterloo, 1979

Honeywell 6000 (GCOS/TSS), written in Niemiec's own language **F**, built on top of
a 1977 Waterloo port of Crowther & Woods' *Adventure*.

The source archive is a sibling of *Martian Adventure*, which is a different game
by a different author on the same tape set. See
`../Martian Adventure (Honeywell 6000, B, unfinished)`.

---

## 1. What came off the tape

```
ar073.2520   23,176 bytes   GCOS archive member  /f/newadv/adv.f
ar073.2522    1,512 bytes   GCOS archive member  /f/newadv/wizard
ar073.2523   62,192 bytes   GCOS archive member  /f/newadv/loc.f
0, 2, 3                     `gcat +d` octal dumps of the three members
gcat.c                      an archive extractor, author unknown
adv.f, loc.f, wizard        gcat's output — visibly truncated mid-sentence
```

Niemiec's own note with the archive says *"adv.f, loc.f, and wizard are my code,
although they appear to be incomplete."*

They are not incomplete. `gcat.c` has a bug.

## 2. The gcat bug

A member is not one flat stream. It is a run of **segments of 0o7417 = 3855 words**
(a 15-word member header plus 12 blocks of 320 words), and each segment is padded
out to a whole **byte**:

```
3855 words x 36 bits = 138,780 bits = 17,347.5 bytes  ->  17,348 bytes stored
```

Those spare 4 bits mean every segment after the first sits at a 4-bit offset from
the one before it. `gcat.c` reads segment 0, keeps walking at the old alignment,
and immediately falls into noise — which is exactly where both files "end".

`port/gcat2.py` carries the offset (`SEG_STRIDE = 138784` bits) and reads the lot.
Both members finish on a clean GCOS end-of-file marker, so nothing was lost on the
tape at all:

| file | gcat.c | gcat2.py | ends on |
|---|---|---|---|
| `adv.f` | 549 lines | **647** | clean EOF marker, 16 blocks |
| `loc.f` | 563 lines | **1941** | clean EOF marker, 43 blocks |
| `wizard` | 35 lines | 35 | was one segment, always complete |

The overlapping lines are byte-identical to gcat's output, so the new reader only
adds. Results are in `recovered/`.

## 3. What survives, and what does not

`adv.f` ends with its own include list, and that is the inventory:

| | |
|---|---|
| `vocab { ... }` | **have** — 287 entries, 229 spelled-out words, in `adv.f` |
| `games/f/include/vocab` | **lost** — the verb and direction vocabulary |
| `games/f/include/main` | **lost** — the engine: parser, main loop, clocks, travel, scoring, save/restore |
| `games/f/newadv/obj.f` | **lost** — every object's behaviour. Not one byte survives |
| `games/f/newadv/loc.f` | **have** — 162 locations, complete |
| `games/f/newadv/wizard` | **have** |
| the F compiler, the pseudocode interpreter | **lost** |

The ~50 runtime functions the sources call (`travel`, `furnish`, `enchron`,
`objexec`, `behalf`, `mutate`, `wearing`, …) are recoverable from their call sites —
`travel` alone is called 140 times and `furnish` 105, which pins both down. The
engine is work, not a puzzle.

`obj.f` is the real hole. It holds take/drop/read/open per object, treasure values
and containers, and it would have to be **invented**, not restored. Filling it from
the 350-point original would be putting another game's content into this one.

**`ar073.2521` is missing from the sequence** — 2520, _2521_, 2522, 2523. That is
where `obj.f` would sit. Worth asking the tape's owner for before anything is
reconstructed.

## 4. The walkable map (`port/`)

Since the engine and the objects are gone but the geography is intact, `port/`
walks the map and nothing else.

```
sh build.sh      # gendata.py -> data.h, then gcc -> newadv
./newadv         # "help" lists the commands
sh demo.sh       # the guided walk in transcript.txt
```

* **`gendata.py`** plays the part of the lost F compiler: it parses the `vocab`
  block in `adv.f` and the `locexec()` switch in `loc.f`, and emits `data.h`.
* **`newadv.c`** walks it. Every description, refusal and exit is verbatim from
  `loc.f`.

What it produced:

```
162 locations (140 room blocks + the global default; 2 blocks are range
              labels, TREE_0::TREE_11 and TREET_0::TREET_11, covering 24 rooms)
856 exits — 751 plain, 105 guarded by game state
158 rooms with a plain Look text; 4 whose Look is a conditional
```

**Nothing is simulated.** The walker models no lamp, no grate, no acorn. When an
exit or a description depends on state, it says so and prints the original F
source rather than guessing:

```
ABOVE_GRATE> d
  [guarded: this exit depends on game state the walker does not
   model.  loc.f says:]
    | switch(.GRATE) {
    | default:
    | 	"You can't go through a locked steel grate!\n";
    | 1:
    | 	"You can't go through a closed steel grate!\n";
    | 2:
    | 	BELOW_GRATE;
    | }
  ["force" to go anyway]
```

`force` takes it anyway, and says that it did. `src LABEL` shows the source behind
any of the room's other verbs. `dot FILE` writes a graphviz map (523 edges).

### Two things the source computes, and this re-executes

`gendata.py` contains a small F evaluator rather than transcribing these by hand:

* **The mallorn grove** (`TREE_0::TREE_11`) never calls `travel()`. It fills
  `travtab[]` in a loop with `which((loc-TREE_0)*5/4-i*5/4+13, 0,0,0,0,0,0,0,SE,S,
  SW,0,0,E,0,W,0,0,NE,N,NW,0,0,0,0,0,0,0)`. Re-running that arithmetic yields a
  clean 4x3 grid — `g(L) = 5*(L/4) + L%4`, eight-way movement, no wraparound at
  the row edges — plus `W`/`OUT` out of `TREE_4` and `U`/`CLIMB` into the treetop.
  84 exits across 24 rooms that would otherwise have been missing.
* `TREET_0::TREET_11` does `travel(D,OUT, loc-TREET_0+TREE_0)` — a computed
  destination, folded the same way.

The evaluator refuses to fold anything touching a state variable (`.GRATE`,
`.ACORN`, `.ELEVATOR`), so those stay guarded. That is what stops it inventing.

### The compass order is derived, not assumed

`vehexec()`'s raft block does

```c
.X += which(fn-N, 0,1,1,1,0,-1,-1,-1);
.Y += which(fn-N, -1,-1,0,1,1,1,0,-1);
```

which is dx/dy for **N, NE, E, SE, S, SW, W, NW** with y increasing southward.
That fixes both the order and the origin of the range label `N::NW` without
appealing to convention. Long forms (`north` for `N`) are a convenience of the
walker — the synonym table was in the lost `include/vocab`.

## 5. Findings from the recovered map

* **150 of 162 locations are reachable from `ROAD_BTM` through unguarded exits**;
  155 counting guarded ones.
* The remaining **7 are not entered through `locexec()` at all**, and the sources
  say where they are entered instead: `ISLAND` and `SEA` from `vehexec()`'s `RAFT`
  block in `loc.f` (the dream sequence, `.X`/`.Y` on an 8-way grid), and the five
  `REPOS_*` rooms from `endgame()` in `adv.f`, which does `move(-REPOS_NE)`.
* A negative sign appears on both motion words (`-DEPRESSION`) and destinations
  (`-FOREST_1`, `move(-i)`). **Its meaning is not determinable from what survives** —
  most likely "matched but not the primary word" and "force a long description",
  but that is a guess and the port does not act on it. The flag is carried through
  and shown: `(DEPRESSION)` for a negated word, `-FOREST_1` for a destination.
* Niemiec's own indentation puts two statements of `STATUE_RM`'s `Look` at label
  depth rather than body depth. They are body, and the parser treats them so.
* The cave is Tolkien-flavoured well beyond the original: mallorn trees, Durin's
  tomb, an orc and Gollum on wandering clocks, a ring that makes you invisible,
  and a stone door whose inscription reads *"Speak, friend, and enter."*
  (`FRIEND` is a magic word; it does `.STONE_DOOR ^= 1`.)
* `adv.f`'s last line is `#text "pass 2:"` with nothing after it — the block ends
  on a genuine EOF marker, so that is how the file really ends.

## 6. Layout

```
src_original/   exactly as it came off the tape, plus gcat.c and its dumps
recovered/      the full extraction: adv.f (647), loc.f (1941), wizard (35)
port/           gcat2.py, gendata.py -> data.h, newadv.c, build.sh, demo.sh,
                transcript.txt, newadv.dot
```

Related: `[[feedback_no_cross_game_borrowing]]`, `[[project_martian_adventure_port]]`.
