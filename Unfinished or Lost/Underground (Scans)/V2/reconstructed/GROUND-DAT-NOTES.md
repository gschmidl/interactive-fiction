# GROUND.DAT — what is recoverable, and what is not

`GROUND.DAT` is UNDERGROUND V2's initial-state file. Line 5000 of the source
writes a save file from exactly the same four arrays, so the two formats are
identical — Gary confirmed this independently.

`build_ground_dat.py` emits a structurally valid file with everything the
surviving material actually pins down, and zero elsewhere. **It is not a
playable game**, and the reason is specific and worth stating plainly.

## File layout

```
DIM #3%, M9%(107%,15%), O9%(65%), O8%(65%), X%(5%)
```

| Array | Shape | Elements | Offset | Block |
|---|---|---|---|---|
| `M9%` | 107 × 15 | 1728 | 0 | 1 |
| `O9%` (= `O%`) | 65 | 66 | 3584 | 8 |
| `O8%` (= `O1%`) | 65 | 66 | 4096 | 9 |
| `X%` | 5 | 6 | 4608 | 10 |

Total **5120 bytes = 10 blocks**. Integers are 2 bytes, little-endian (PDP-11).

Two layout assumptions are worth flagging, because they are conventions of
BASIC-PLUS virtual arrays rather than anything the scans prove: that each array
in a `DIM #` list starts on a 512-byte block boundary, and that `M9%(i,j)` is
stored row-major at element `i*16 + j`. Both are easy to change at the top of
the script if a real RSTS/E run disagrees.

## What is filled in

**`O1%()` — object descriptions: 58 of 65 objects.** These are real. Eleven are
*proved* by the source, because the code assigns the other state of the same
object and the pair has to match:

| Object | Initial | Becomes | Proof |
|---|---|---|---|
| 1 gold | 1 "buried in the ground" | 0 "gold nuggets" | `6700 IF O1%(T%)=1%` → "You cannot get the gold out of the ground"; `10200 O1%(1%)=0%` |
| 21 flashlight | 18 "a flashlight here" | 24 "shining" | `9700 O1%(21%)=24%` / `9800 …=18%` |
| 22 bottle | 22 "small glass bottle" | 23 "water inside" | `10000 O1%(22%)=23%` |
| 35 ring | 212 "green ring" | 213 "glowing" | `11600 O1%(35%)=213%` |
| 40 rat | 50 "hungry rat" | 51 "smelly, dead rat" | `13000 O1%(40%)=51%` |
| 41 cat | 52 "hungry cat" | 225 | `13200 O1%(41%)=225%` |
| 42 chest | 56 "oaken chest" | 57 "it is open" | `10400 IF O1%(T%)=56%` |
| 52 computer | 27 "giant computer" | 28 "melted" | `15300 O1%(52%)=28%` |
| 53 wall | 59 "vulnerable to laser fire" | 237 | `15200 O1%(53%)=237%` |
| 54 safe | 44 "locked safe" | 45 "open safe" | `4400 O1%(54%)=45%` |
| 62 vault | 183 "vault is closed" | 182 "is open" | `12700 O1%(62%)=182%` |

The other 47 are read straight off the object-description block of the text
file (fields 0–46) matched against the 41 object names in `DATA 174-185`.

**`X%(1..5)`** — turns, flashlight turns, deaths, book-read, laser shots — are
all 0, which Gary confirms is correct.

**`M%(x,0)`** — the seen flags — are all 0, likewise correct.

## What is missing, and why it can't be inferred

**The movement table `M%(x,1..12)` — 108 rooms × 12 directions.** This is the
map, and it is in no surviving scan. Directions are 1=N 2=E 3=W 4=S 5=UP
6=DOWN 7=IN 8=OUT 9=NE 10=NW 11=SE 12=SW (from vocabulary codes 201–212).
Positive = destination room; negative = a message, dispatched by
`2000 … ON 1%-X% GOTO 3000,3000,3000,3000,3000,3000,3100,3200,3300,3400,3500,3700`.

The source constrains exactly **three** of those 1296 entries, and only to a
choice of two values each — the computer switch at line 9300 toggles them
against a constant, which only works if the value is 0 or that constant:

```
9300  M%(18%,8%)=63%-M%(18%,8%)     room 18, OUT  is 0 or 63
      M%(30%,3%)=50%-M%(30%,3%)     room 30, W    is 0 or 50
      M%(33%,7%)=18%-M%(33%,7%)     room 33, IN   is 0 or 18
```

Which state each switch *starts* in is not recorded, so the script leaves them 0
rather than guessing.

**`M%(x,13..15)`** — the description field indices per room. The text file gives
the descriptions and their field numbers, and they are clearly grouped (a run of
long-description fields, then a short "You're at …" line). But mapping a group
onto a *room number* needs the room numbering, which only `GROUND.DAT` held.

**`O%()`** — object starting locations. The design sheet `07192618` really does
list every room with the objects placed in it, in the author's own hand, so the
*pairings* survive. What does not survive is which room number each name has.

**`X%(0)`** — the starting room. The RUN transcript opens on the room whose long
description is fields 184–186, so we know which room it is; we don't know its
number.

Everything missing reduces to the same single unknown: **the room numbering.**

## What the source does tell us about specific rooms

31 room numbers are named outright by the code, which is a real foothold — see
`ROOM_FACTS` in the script. The most useful:

- **rooms > 18 are dark** (`800`), so 1–18 are the surface: building, path,
  forest, lake, hole. Room **54** is the one exception, lit while underground.
- **room 3** is where you reincarnate after death (`15700 X0%=3%`) and has the
  great door (`10800`) — matching the basement's "great door … locked from the
  other side" in field 189.
- **room 37** leads to room 3 with the key (`10900`), so it is the far side of
  that same door.
- **rooms 1, 19 and 40** are each worth 50 points on first sight (`16200`).
- **room 39** is reachable only with ≥380 points (`3500`), which is the room
  behind the "YOU MUST HAVE AT LEAST 380 POINTS TO COME THIS WAY!" sign
  (field 383).
- **room 68 → 95** is the space warp (`4700`); **room 99** is inside the vault;
  **room 108** is a pseudo-room whose entry triggers the endgame.

## The honest next step

Reconstructing the map would mean *designing* 1296 movement entries from a
hand-drawn room list, and the result would be a new game that reused Gary's
text — not his game. I have not done that, and I would not without you asking
for it explicitly and labelling the result as such.

Two routes that would produce the real thing:

1. **Ask Gary.** He has already reconstructed the format from memory. The
   design sheet plus the 31 known room numbers may be enough to jog the
   numbering, which is the only genuinely missing piece.
2. **Look for a save file.** Any surviving `UNDER<letter>.DAT` has the identical
   layout. A save from a game in progress would hand over the entire map, the
   object placements and the room numbering in one go — the seen flags and a
   few counters would differ from the pristine initial state, and nothing else.
