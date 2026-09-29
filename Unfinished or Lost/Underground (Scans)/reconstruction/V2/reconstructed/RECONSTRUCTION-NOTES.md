# UNDERGROUND (UNDERG.BAS) — OCR reconstruction notes

Source: `../*.PDF` — the 5 PDFs in `V2/`, 22 scanned pages, no text layer
(pure JPEG scans of pin-fed fanfold).

**UNDERGROUND by Gary Kleppe**, BASIC-PLUS (RSTS/E), listing dated **20-May-80**,
header note "THIS VERSION CREATED SUMMER,1979".

## Output files

| File | What it is |
|---|---|
| `UNDERG.BAS` | The 20-May-80 listing, transcribed as printed (printer's 72-column wraps preserved) |
| `UNDERG-unwrapped.BAS` | Same program with the printer's hard wraps re-joined into logical lines — easier to read/run |
| `UNDERG-05Mar80-variant.txt` | Earlier 05-Mar-80 revision fragments, a `$LEDIT` patch session, and a RUN transcript |
| `LISTER-partial.txt` | A *different* program (`$LISTER` catalog utility) that was in the same scan batch |

## What the 22 pages actually are

The scans are **two passes over the same fanfold**, which is why you saw
duplication and missing pieces:

- **`07192608.PDF` (6 pp)** — pass A: the complete listing, PAGE 1–6.
  Only **page 1** is cropped, losing ~4–5 columns off the left edge
  (so `150 DATA NORTH…` prints as `ATANORTH…`). Pages 2–6 are complete.
- **`07192609/10/11.PDF` (15 pp)** — pass B: re-scans of the same sheets,
  shifted right so the left margin and line numbers are captured, but
  skewed, faint, torn, and cropped on the right. Heavy overlap between them.
- **`07192612.PDF` (1 p)** — not UNDERGROUND at all; a `$LISTER` utility.

Pass B was used to recover page 1's line numbers; pass A supplied the rest.
Every page was contrast-normalised (local background division) before reading —
the raw pass-B scans are nearly invisible otherwise.

## Confidence

**High.** The program is 208 numbered lines. As a check, every branch target was
extracted from the reconstruction and resolved against the defined line numbers:

- 92 distinct `GOTO` / `GOSUB` / `THEN` / `ELSE` targets
- **0 dangling references**

The two passes also agree with each other everywhere they overlap, and the
41 object-name `DATA` strings (lines 174–185) count out to exactly `O$(41%)`,
matching the `DIM`.

## What is inferred rather than read

1. **The three title lines and line 100 have no recoverable line numbers.**
   They sit in the strip cut off page 1, and pass B's coverage of that strip is
   physically torn away. The number `100` on the opening `X$=CHR$(X%)+X$…`
   statement is a **guess**; the text of the statement is not.
   The three `!` title lines are shown without numbers.

2. **Lines 150–157** — the line numbers are inferred by counting back from
   `158 DATA READ,309…`, which pass B shows explicitly (as do 156 and 157).
   The `DATA` text itself is read directly and confirmed by both passes.

3. **Line 100's continuation lines** — the leading 4–5 characters of each
   wrapped physical line were cut. They are reconstructed from context and are
   unambiguous (`DIM `, `READ `, `OPEN `, `$(15`), e.g. `,W` + `$(15` + `0%)`
   gives `W$(150%)`, which the `DIM` requires.

4. **Line 16000** — the printer wrapped inside a string literal
   (`". For the` / `next rating you need"`). The lister trims trailing blanks at
   the 72-column wrap, so the space was lost in print. `UNDERG.BAS` keeps it as
   printed; `UNDERG-unwrapped.BAS` restores the space.

## Things that look like OCR errors but are not

- **`-` where you expect `=`.** Lines such as `6300 IF T%-41% OR …`,
  `2900 … O%(37%)-X0% THEN 800`, `12700 IF T%-19% OR X0%-98% OR O1%(62%)-183%`
  use subtraction as a "not equal" test — a standard BASIC-PLUS space-saving
  idiom (`IF <nonzero>` is true). These are real minus signs, verified at high
  magnification, and are transcribed as such.
- **Missing spaces**, e.g. `7700IF T%=41%…`, `13300IF O%(27%)…`,
  `THEN13000ELSEIFT%=26%ANDO%(40%)=X0%`, `GOTO1000`. The author really did type
  them that way; BASIC-PLUS ignores spaces outside string literals.
- **Lowercase lines** — `15050`, `15350`, `16500`, and part of `11700`
  (`and x0%=88%`) are in lowercase. These are later patches typed at the
  terminal and are preserved verbatim.
- **`A $=""`** in line 1300 has a real space inside the variable name as printed.

## Known bug in the source (author flagged it)

Line 1300 reads:

```
\    A$=FNB$(A$) IF C%=103%
```

`C%` is **circled in pen** on the 20-May-80 sheet. The earlier 05-Mar-80
listing (see `UNDERG-05Mar80-variant.txt`) has `IF X0%=103%` here, and `X0%`
is what the identical test in `FNR$` (line 300) uses. So `C%` is a typo the
author had spotted but not yet fixed.

**Status: fixed in `UNDERG-unwrapped.BAS` (now `IF X0%=103%`); left as `C%` in
`UNDERG.BAS`.** The as-printed file is the archival record of what the paper
actually says, so it keeps the typo; the un-wrapped file is the runnable copy,
so it gets the correction. That is the only place the two files differ in
substance. (X0% is the player's current location; 103 is the backwards room,
where `FNB$` reverses every string.)

## External files the program needs (not in these scans)

- `[3,3]UNDERG.TXT` — opened as file 2%, a fixed-length record file holding all
  game text. `FNR$(A%)` fetches message `A%` as 73-byte fields, 7 per record,
  `XLATE`d through `X$` (a reversed CHR$ table built in line 100) — i.e. the
  message text is obfuscated in the file.
- `[3,3]GROUND.DAT` — opened as file 3%, the virtual-array initial state
  (`M9%`, `O9%`, `O8%`, `X%`): the 107-room map, 65 object locations, and
  starting variables.
- `UNDER<name>.DAT` — per-player save files (lines 4900–5200).

Without `UNDERG.TXT` and `GROUND.DAT` the program will list and compile but
cannot produce room descriptions or a map. Message numbers referenced in the
code run up to 267 and 431–446.

## Structure summary

- 100 — setup, `DIM`, opens, reads vocabulary and object names
- 150–172 — vocabulary: 150 word/code pairs, words truncated to 5 chars
  (codes: 1xx = intransitive verbs, 2xx = directions, 3xx = transitive verbs,
  4xx = nouns, 500 = articles)
- 174–185 — 41 object display names
- 200 `FNB$` — string reverse; 300 `FNR$` — message fetch/decode;
  400 `FNZ$` — Y/N prompt
- 700–850 — restore state, describe room, list objects
- 1300–1400 — input and parse
- 1500–1600 — verb / intransitive dispatch
- 2000–3700 — movement
- 3800–5300 — LOOK/INVENTORY/QUIT/SCORE/SAVE/RESTORE/UNSAVE etc.
- 5800–15400 — transitive verb handling
- 15600–16200 — death, scoring (500 points max), rank
- 16300–16600 — exit (`SYS("^E")`, `SYS("^NBYE/F")`) and END
