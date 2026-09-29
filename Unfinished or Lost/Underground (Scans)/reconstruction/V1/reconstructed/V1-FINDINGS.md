# UNDERGROUND V1 (13/14-Mar-79) — assessment

**V1 is a better restoration target than V2, because nothing is missing.**

V2 is permanently short its map: `GROUND.DAT` was never scanned and the room
numbering is unrecoverable. V1 has no equivalent problem — its initial state is
**in DATA statements inside the source**, and all its text is inline.

## Why V1 is complete

Line 70 of the main program reads the whole world out of DATA:

```
10  OPEN"KB:"ASFILE1% : DIM #2%, M%(87%,14), O%(45), X%(11)
    : &"Are you recalling a saved game?" : GOSUB 3080 : IF Z$="Y" THEN 40
...
70  X%(11%)=30% : X%(X%)=0% FOR X%=10% TO 1% STEP -1%
    : M%(X%,0%)=0% for X%=1% to 87% : O%(0%)=39%
    : MAT READ M%,O% : X%(0%)=8%
    : &"If you want to recall this game at a later date, the password is "P$"."
    : goto 970
80  DATA ,8,,,,,,,2,,8,8,
90  DATA -1,,,,,,,,1,,-1,-1,-1,
```

`MAT READ M%,O%` fills `M%(1..87, 1..14)` and `O%(1..45)`. Column 0 (the seen
flags) is zeroed explicitly just before, which is exactly consistent with MAT
READ not touching row/column 0.

**The DATA block is entirely present and exactly the right size.** On
`07192605` p1–p2 the room rows run:

| Lines | Count |
|---|---|
| 80–500 step 10 | 43 |
| 501–507 | 7 |
| 510–870 step 10 | 37 |
| **total** | **87** |

— one row per room, matching `M%(87%,14)`. Line **880** then carries the 45
object locations for `O%(1..45)`, wrapped across two printed lines, and line
**970** resumes program code. Nothing is torn or missing in that stretch.

Known starting state, straight from the source: **start room 8**, `O%(0%)=39%`,
`X%(11%)=30%`, all other flags 0.

## Architecture

V1 is two chained programs, both present in the scans:

1. **The main program** (`UNDERG`) — parser, verbs, all inline text, and the
   DATA world. Header on `07192605` p1 reads `UNDERG 10:26 AM 14-Mar-79`.
2. **`LOOK.BAC`** — the room describer. Line 990 chains to it:
   ```
   990 R%=100%*X%(0%) : R%=R%+50% IF M%(X%(0%),0%) AND A$<>"LOOK"
       : M%(X%(0%),0%)=1% : CLOSE 2% : CHAIN "[3,1]LOOK.BAC" R%
   ```
   so the chain line number is `room*100`, plus 50 if the room has been seen —
   i.e. long description at `room*100`, short at `room*100+50`. `07192607` is
   its source, and its line numbers match: `1850 &"Top of ramp." : goto 10000`
   is room 18's short description.

State lives in `UNDER<letter>.DAT`, a virtual array on channel 2 created at
runtime — so it is a save file, not a missing input. Line 20 scans A–Z for a
free letter; line 40 asks for the one-letter password to resume.

## Page coverage

| File | Pages | Contents |
|---|---|---|
| `07192605` | 7 | Main listing, 14-Mar-79 — appears to be a complete pass (starts at line 10) |
| `07192607` | 5 | `LOOK` describer source |
| `07192603` | 5 | Another listing pass, includes DATA |
| `07192604` | 3 | Listing fragments |
| `07192600/601/602` | 4 | Listing fragments (verb handlers, ~2180–2860) |
| `07192606` | 17 | Mostly RUN transcripts and debug sessions; some listing + DATA |

The redundancy is useful in the same way V2's two scan passes were: the
fragment sheets overlap the main pass and can cross-check it.

## What a V1 restoration would take

Transcribing roughly 12 core pages (`07192605` ×7 + `07192607` ×5), with the
fragment sheets as cross-checks — comparable in size to the V2 source job
already done, and with the same verification available: resolve every
`GOTO`/`GOSUB` against defined line numbers, and check the DATA block reads
exactly 87×14 + 45 values.

The result would be a **complete, runnable game** under RSTS/E on SIMH, which
is more than V2 can currently be.

## Caution

V1 is a genuinely different game from V2 — different room count (87 vs 107),
different object set (45 vs 65), different parser, different text. It is not a
source of missing V2 data, and nothing here should be back-filled into the V2
reconstruction.
