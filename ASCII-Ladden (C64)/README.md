# ASCII-ladden — C64 type-in recovery

Norwegian text adventure by **Tor Engebakken and John Andersen**, published as a
type-in listing across four issues of *Mikrodata* / *PC Mikrodata* (1/1985–4/1985),
as part of the article series *"Adventure på norsk"* (begun in nr. 10/1984).

Source document: `ASCII-Ladden (Norwegian).pdf` (13 pages).
Symbol legend: `ASCII-Ladden symbol explanation.png`.

## Deliverable

Two disk images, so you can pick fidelity or playability:

* **`ascii-ladden-fixed.d64`** — the playable one. Single file `ascii-ladden`,
  carrying the one-line repair described under *Known defect*. Use this to play.
* **`ascii-ladden.d64`** — the archival one. Two files:
  * `ascii-ladden` — verbatim transcription of the printed listing
  * `ascii-ladden.fix` — the repaired build, as a second file

Run with e.g. `x64sc -autostart "ascii-ladden-fixed.d64:ascii-ladden"`, or
`LOAD"ASCII-LADDEN",8` then `RUN`.

## Where the listing lives in the PDF

| PDF page | Magazine | Lines |
|---|---|---|
| 3 | Mikrodata 1/1985 p.46 | 10, 100, 2000–2099, 10000–11220 |
| 4 | Mikrodata 1/1985 p.47 | 11230–11460 |
| 6 | Mikrodata 2/1985 p.36 | 100–3000 |
| 8 | Mikrodata 3/1985 p.14 | 300–5040 |
| 12 | PC Mikrodata 4/1985 p.55 | 600–6050 |

Later installments replace four placeholder lines from earlier ones
(100, 300, 600, 3000). Merging the four parts in line-number order also
places 2070–2090 into the start-up path, which is exactly what the
accompanying article describes — a useful confirmation that the merge is right.

## Files

| File | What it is |
|---|---|
| `part1.lst` … `part4.lst` | Per-installment transcripts, in the magazine's own bracket notation |
| `ascii-ladden.lst` | The four parts merged, sorted by line number |
| `ascii-ladden.pet` | petcat source (see *Character encoding*) |
| `ascii-ladden.prg` | Tokenized BASIC |
| `merge.py` | Merges the installments, reports which lines were overridden |
| `validate.py` | Checks every GOTO/GOSUB/THEN target resolves |
| `convert.py` | Bracket notation → petcat source |

Rebuild:

```bash
python merge.py && python validate.py && python convert.py ascii-ladden.lst ascii-ladden.pet
petcat -w2 -o ascii-ladden.prg -- ascii-ladden.pet
```

## Character encoding

Unbracketed letters in the printed listing are **unshifted** PETSCII; `[S>X]`
marks a **shifted** character. The program runs in lower/upper case mode
(`CHR$(14)`), so unshifted prints lower case and shifted prints upper case.
petcat uses the same convention — lower case in source = unshifted — so
`convert.py` lower-cases plain text and upper-cases `[S>X]` contents.

`PRINT£1` / `INPUT£1` in the save/load routines are `PRINT#1` / `INPUT#1`;
the listing was printed through a UK/Nordic ISO-646 variant that renders
`#` as `£`.

## Two transcription problems worth recording

### 1. The apparently missing lines 10115 and 10285

Rooms 11 and 29 seem to have no direction-string DATA line. They do: lines
10110 and 10280 each end with a **trailing comma**, supplying an empty second
DATA item. Both lines are exactly 83 printed characters, so the 80-column
listing wraps their last three characters — `G",` and `U STAAR",` — onto the
next row, where the comma is easy to miss.

Semantically correct: room 11 is up a tree and room 29 is on a tabletop, and
both are reachable only by climbing, never by a compass direction.

### 2. Digit `0` versus letter `O` in the direction strings

Direction strings are groups of three characters: a direction letter plus a
two-digit room number. Both *øst* and *opp* would naturally be "O", so the
authors used the **digit `0` for øst** and the **letter `O` for opp** — which
line 2446 tells the player: *"OBS! Bruk null for o."*

This changes the map, so it had to be resolved per character rather than
guessed. The listing's printer font distinguishes them by counter shape — the
digit's counter is tapered and often broken open at the lower right, the
letter's is a closed rounded rectangle — and each glyph was classified on that
basis. Line 10185 (`S20019O17`) carries both in one string.

Only two rooms have an *opp* exit:

* room 18, at the bottom of the shaft → 17 (the iron-bar room)
* room 28, inside the hollow tree stump → 8 (the forest)

Cross-checks, all of which pass: every reciprocal exit in the 30-room graph
matches; the graph agrees with the map printed on PDF page 5; and running the
game prints room 18's exits as *Sor / Ost / Opp* and room 28's as *Opp*.

## Known defect in the original listing

Line 1010 branches to **line 1030, which was never printed**:

```
1000 IFV$<>"FLY"THEN1050
1010 IFS$<>"STE"ORS<>22THEN1030      <-- 1030 does not exist
1020 IFP(19)<>IVTHEN265
1025 P(43)=S:GOTO398
1050 IFV$<>"KRY"THEN1100
```

It is the only unresolved branch target out of 145 in the whole program.
Typing `FLY` anywhere other than at the stone in room 22 therefore stops the
game with `?UNDEF'D STATEMENT ERROR IN 1010`.

`ascii-ladden` on the disk is left **verbatim**, defect included.
`ascii-ladden.fix` adds one line:

```
1030 GOTO265
```

which routes to *"Det er jeg ikke istand til."* — the same fall-through lines
780 and 870 use for the identical "right verb, wrong object" case. That line is
**a reconstruction, not recovered text**; the original line 1030, whatever it
said, is not in the PDF.

## Verification performed

* All 362 lines merge with exactly the four expected placeholder overrides.
* 145 branch targets checked; 1 unresolved (the defect above).
* petcat round-trip (tokenize → detokenize) is identical on all 362 lines.
* Boots in VICE, shows the title screen, and plays: room 1 prints the right
  description, the right object, and the right single exit.
* Decoded exits verified against the data for rooms 16, 18, 23, 25, 26, 28, 30.
* Defect reproduced and the repair confirmed: typing `FLY STEIN` in room 1 gives
  `?UNDEF'D STATEMENT ERROR IN 1010` on the verbatim build, and
  *"Det er jeg ikke istand til."* followed by a normal prompt on the fixed build.

## Remaining uncertainty

Line 2400's guard list `"N","S","O","V","0"` — the *order* of the letter-O and
digit-0 literals is hard to read on the screened page-6 background. It has no
functional effect: the line only tests set membership, and the set is the same
either way.
