# UNDERG scan inventory — all 107 pages across V1, V2, and "Text and misc"

Covers every PDF in this project (`V1/`, `V2/`, `Text and misc/`). None have
a text layer;
all figures below come from rendering at 300 dpi, contrast-normalising, and
reading/OCR-surveying each page.

## V2\ — 22 pages — **transcribed** (see `UNDERG.BAS`)

| File | Pages | Contents |
|---|---|---|
| `07192608` | 6 | The 20-May-80 listing, PAGE 1–6. Page 1 cropped ~4–5 cols on the left |
| `07192609` | 7 | Re-scan pass; pp.6–7 are an earlier 05-Mar-80 revision + `$LEDIT` session + RUN transcript |
| `07192610` | 4 | Re-scan pass (cleanest for lines 158–850) |
| `07192611` | 4 | Re-scan pass |
| `07192612` | 1 | **Not this game** — a `$LISTER` catalog utility |

## Text and misc\ — 44 pages — **not yet transcribed**

This folder contains the missing `UNDERG.TXT` message data, in **three
different printings**:

| File | Pages | Contents |
|---|---|---|
| `07192613` | 3 | Message text, author's handwritten field numbers in the margin |
| `07192614` | 6 | Message text **with printed field indices** (~267→440). Handwritten brackets group multi-field messages |
| `07192615` | 2 | Message text, same style |
| `07192616` | 1 | Message text / notes |
| `07192617` | 1 | Message text with handwritten indices ~120→223 |
| `07192618` | 1 | **Author's handwritten room + object design list** — every room with the objects placed in it |
| `07192619` | 14 | Full message file dumped as `byteoffset,text`, offsets 512→~29,800 |
| `07192620` | 16 | The same dump again, second printing (noisier scan) |

## V1\ — 41 pages — **not yet transcribed**

An **entirely different, earlier version** of the game (RUN header dated
13-Mar-79), written in a much more verbose style — `1100 IFA$="U"ORA$="UP"
ORA$="CLIMB"THENY%=9%:GOTO2000` — with the map held in `DATA` statements
rather than a binary file. Contains source pages, `DATA` blocks, run
transcripts, and error/debug sessions across `07192600`–`07192607`.

---

# How the message file works

`FNR$(A%)` in the V2 source resolves message `A%` as:

- record `A%\7 + 1` (RSTS 512-byte blocks)
- field `A% MOD 7`, each field **73 bytes**
- `CVT$$(A$,128%)` trims it, `XLATE(...,X$)` de-obfuscates it

So each message index addresses one 73-character field, and **long text spans
consecutive indices**. The code relies on this directly:

```
850  &FNR$(Z%); FOR Z%= X% TO Y%      ' X%=M%(X0%,13%), Y%=M%(X0%,14%)
```

i.e. each room stores the **first and last field index** of its long
description in map columns 13 and 14, and its short name in column 15.

Confirmed against the scans: fields **184, 185, 186** are

```
184  You are on a path near a small building. The air here smells of
185  lake water. The path winds north into a dense forest, which surrounds
186  you on all sides.
```

which is exactly the opening room printed in the 05-Mar-80 RUN transcript.
Field splits fall mid-word (e.g. `…There is a hole i` / `n the ground goins…`),
as fixed 73-byte fields require.

## Two independent cross-checks are available

1. **Two printings of the same data** — `07192619` and `07192620` cover the
   same offset range, so each validates the other.
2. **A per-entry length checksum.** In the offset dump, the gap to the next
   entry is *always* exactly `len(text) + 3`. Verified on a clean run:

   | offset | text | len | Δ to next |
   |---|---|---|---|
   | 6947 | Bottom of Elevator Shaft | 24 | 27 |
   | 6974 | Sphere Room | 11 | 14 |
   | 6988 | Gnome Gate | 10 | 13 |
   | 7001 | Dark Room | 9 | 12 |
   | 7013 | Dwarf Door | 10 | 13 |
   | 7026 | Short Passage | 13 | 16 |
   | 7042 | Depression | 10 | 13 |
   | 7055 | Small Square Room | 17 | 20 |
   | 7075 | Prison Cell | 11 | 14 |
   | 7089 | Square Room | 11 | 14 |
   | 7103 | Well Room | 9 | 12 |
   | 7115 | Path | 4 | 7 |
   | 7122 | Tunnel | 6 | 9 |

   Any mis-transcribed character or digit breaks the arithmetic, so the whole
   file can be machine-verified line by line.

Note the offset dump contains genuinely **repeated blocks** (e.g. fields
197–199 repeat as 204–206; 210–216 repeat as 217–223). That duplication is in
the source data, not a scanning artifact.

---

# What is still missing

**`GROUND.DAT`** — the binary virtual array holding the 107-room map
(`M9%(107%,15%)`), the 65 object locations (`O9%`, `O8%`) and the starting
variables (`X%(5%)`). No printout of it appears in any of the 107 pages.

It may be reconstructible, but only by inference, from:
- `07192618`, the handwritten room/object design list (rooms in order, with
  the object placed in each);
- the V1 `DATA` statements, which encode the earlier version's map;
- the message field indices themselves, which pin columns 13/14/15 per room;
- the exit logic in the V2 source (`2000 X%=M%(X0%,Y%)…`).

That would be a genuine reconstruction rather than a transcription, and the
result would not be guaranteed to match the original data file.
