# Kjempens skatt — ZX Spectrum type-in recovery

**Kjempens skatt** ("The Giant's Treasure") is a Norwegian text adventure for the
48K ZX Spectrum, submitted by **Endre Danilojj** (B.A. Løvoldsveg 27, 9022
Krokelvdalen) and printed as a type-in listing in a Norwegian computer magazine.
The magazine paid kr 300 for it. The author credits the idea to *Sinclair User*.

Recovered 2026-09-05 from a scan of the magazine pages (6 pages: page 1 has
the blurb, pages 1–6 carry the listing).

## Deliverables

| File | What it is |
|---|---|
| `kjempens-skatt.tap` | **The game.** Standard TAP, auto-starts at line 1 on LOAD. |
| `kjempens-skatt-nostart.tap` | Same program, no autostart (loads, then you `RUN`). |
| `kjempens-skatt-fixed.tap` | Same, with the one riddle-selector bug fixed (see below). |
| `kjempens-skatt.bas` | The BASIC source, one logical line per text line. |
| `kjempens-skatt-fixed.bas` | Source of the fixed variant. |
| `listing.txt` | Identical to `.bas` — the joined listing. |
| `listing-as-printed.txt` | The 810 printed rows exactly as they appear, 32 columns wide. |
| `rows_p*.txt` | Per-page/per-column transcription (the working files). |
| `masks.txt` | Machine-generated ink/blank masks used to verify every row. |

356 BASIC lines, 19 519 bytes of program — about 22 KB free on a 48K Spectrum.

Verified in Fuse (`fuse.exe --machine 48 --auto-load --tape kjempens-skatt.tap`):
loads, auto-runs, shows the title and instruction screens, and plays — room
descriptions, exit lists and the inventory subroutine all behave.

## The source material, and why it needed tooling

The listing is not typeset. It is a photographed **32-column ZX-printer dump**
(`LLIST`), so:

* Every logical line wraps at exactly 32 characters, mid-word and mid-token.
* The author padded display strings with runs of 2–7 spaces to control where
  text breaks on the Spectrum's 32-column screen. Getting a run wrong by one
  space is invisible to the eye but changes the game's output.

So instead of transcribing by eye alone, the recovery works like this:

1. `grid2.py` fits the monospace character grid (pitch ≈ 26.9 px at 300 dpi) to
   each scanned page column, then emits a 32-cell **ink/blank mask** for every
   printed row.
2. `verify.py` compares the transcription against those masks, flagging
   * `MISSING-INK` — a character claimed where the scan is blank (a real error), and
   * `TAILDIFF` — the last character landing in the wrong column (a wrong space run).
3. `join.py` re-assembles the 810 rows into 356 BASIC lines, padding every
   non-final row back to its full 32 columns.

That found roughly twenty wrong space runs, one dropped row (the tail of line
5350, below the fold on page 4) and one dropped continuation row.

Remaining mask mismatches are all *extra* ink — ink bleeding vertically from the
tightly-spaced neighbouring row, plus scanner specks — and were each checked by hand.

### Ø is the digit zero

The Spectrum has no `Ø`. The author exploited the fact that the ZX Spectrum's
digit **`0` glyph is an O with an internal diagonal** — on screen it reads as Ø.
So the game genuinely contains `S0r`, `0st`, `KJ0P`, `N0KKEL`, `SV0M`, `KJ0TT`,
`d0r`, `sv0mte`, `0yet`, `innsj0`, `sj0uhyre`, and the east command is the digit
`0` while up is the letter `O` (lines 23 and 25). The two were told apart by
matching each scanned cell against the real ROM font — `0` scored 0.77 vs `O` 0.59
in `z$="0"`, and `O` 0.83 vs `0` 0.76 in `z$="O"`.

(Everywhere else the author writes `oe` for ø and `aa` for å: *moerkt*, *hoeye*,
*foer*, *sproeyte*, *paa*, *saa*.)

### `~` is the in-string quotation mark

Four times the listing shows a quote-like mark **inside** a string —
`...i bakgrunnen~Ha nei,...ogsaa~.` (line 4815) and `...Han svarte~Hvis...99~.Hvor...`
(line 5330). A real `"` there would close the string and make the line a syntax
error. Correlating the scanned 8×8 cell against the 48K ROM character set
identifies it as **`~` (CHR$ 126)**, scoring 0.85 against the runner-up's 0.55 —
while the genuine `"` in the same printed row scored 0.97 as `"`. The author used
`~` as a stand-in for quotation marks.

## Bugs in the published listing (kept as printed)

* **Line 5315 — `LET a=INT (RND*5)+5`.** The king's riddle is chosen by `a`, and
  lines 5320/5330/5340/5350 test `a=1..4`. With `+5` the value is always 5–9, so
  every game falls through to the Per/Paal riddle at 5360. Almost certainly `+1`
  was meant; `kjempens-skatt-fixed.tap` makes that one change and nothing else.
* **Lines 50–60 are dead code.** They are the drop (`KAST`) handler, but nothing
  jumps to line 50 — line 40 returns for any verb that is not `TA`, and line 45
  goes straight to 100. So `KAST` only works in the one room that special-cases
  it (line 3310). Line 50 also reads `o$(q, TO LEN b$)` where `z` was meant,
  which would have been a "variable not found" error had it ever run.
* **Line 6020 — `GO TO 9990`.** No line 9990 exists; the Spectrum falls forward
  to 9995, which is the intended "another game?" prompt. Harmless.
* **Line 30 — `IF z$=" STOP " OR z$="STOP"`.** The first comparison, with literal
  spaces, can only match if the player types spaces around `STOP`.
* `1510 ... endverg.` is the author's spelling (for *endevegg* / dead end), and
  `Der vardet fullt av vampyr flaggermusersom` (line 2300) is missing two spaces
  in the original.
* Line 9509 is `LET A=1` with a capital A; the Spectrum treats it as the same
  variable as the `a` used on lines 9512–9515.

## Playing it

Verbs: `RI, TA, KAST, GI, KJØP, SVØM, DREP, LAASOPP` — a verb must be followed by
a noun (`TA SVERD`, `DREP DVERGEN`). Movement: `N`, `S`, `V`, `0` (øst), `O`
(opp), `NE` (ned). `I` lists what you carry, `SE` redescribes the room, `SAVE`
and `LOAD` use tape, `STOP` quits. The program turns CAPS LOCK on
(`POKE 23658,8` at line 9502), so type in capitals.

## Tooling in this directory

| Script | Role |
|---|---|
| `gen.py` | Renders the PDF page columns to readable 400 dpi crops. |
| `grid2.py`, `mask.py` | Character-grid fitting and per-row ink mask generation. |
| `cellmask.py`, `crop.py` | Helpers for spot-checking individual rows. |
| `glyph.py` | Matches a single scanned 8×8 cell against the real 48K ROM font. |
| `verify.py` | Transcription ↔ mask checker. |
| `join.py` | 32-column rows → BASIC lines. |
| `zxbas.py` | ZX Spectrum BASIC tokeniser, 5-byte float encoder, TAP writer, and a `LIST` renderer. |
| `regions2.json` | Fitted page-column geometry (origin, pitch, row pitch). |

`zxbas.py` reproduces the ROM's `LIST` spacing rules, which were derived from the
listing itself and then used to validate the tokeniser:

* a token gets a **leading** space if its index is ≥ 32 (`OR` and above) and its
  text starts with a letter — suppressed if a space was just printed;
* a token gets a **trailing** space if its index is ≥ 3 (i.e. not `RND`,
  `INKEY$`, `PI`) and its last character is `$` or a letter.

Round-tripping all 356 lines through `tokenise_line` → `render` reproduces the
printed text exactly, apart from trailing spaces the printout cannot show. That
is the guarantee that the bytes in the TAP will `LIST` the way the magazine did.
