# THE SPPY (ザ・スッパイ) — NEC PC-6001

Recovered from a magazine type-in listing (scan supplied by the user).

| | |
|---|---|
| Title | THE SPY 006 — the title screen credits **YELLOW SOFT** |
| Machine | NEC PC-6001, 32K, N60-BASIC |
| Listing | PDF pages 2–6 = the program; page 6 (bottom) – page 9 = the title-screen dump |

## Files

* `The Sppy.bas` — the main listing, **Shift-JIS (CP932)**, 437 lines, range 10..4370
* `The Sppy SAVER.bas` — the separate 7-line loader (`EXEC &HDF00`)
* `The Sppy TITLE.bin` — the title screen, **6144 bytes, loads at &HC200**
* `The Sppy TITLE.hex` — the dump transcription in the magazine's own line format
* `The Sppy TITLE.png` — the decoded title screen, for checking at a glance
* `work/sppy_ml.bin` — the 340-byte machine-code block from the DATA statements
* `work/title/pNNcN.hex` — the per-page, per-column transcriptions
* `src_original/` — the source PDF

## Verification

### The main listing
437 lines, 10..4370. No duplicates, no out-of-order line numbers, quote balance clean,
and **every GOTO / GOSUB / THEN / ON..GOTO target resolves to a line that exists**.

### The title-screen dump — every line independently checksummed
Each printed line is `ADDR:bb bb bb bb bb bb bb bb:cc`, and unlike the Burglar House dump
in this same batch the trailing value **is a real checksum: the low byte of the sum of the
eight data bytes**. So every line verifies on its own.

* **768 of 768 lines pass**, addresses contiguous `C200`–`D9FF`, no gaps, no duplicates.
* Total **6144 bytes = 0x1800 exactly**, which is one whole PC-6001 graphics page — the
  size falling out exactly right is itself a check that no line was lost or doubled.

### …and then confirmed visually
6144 bytes at 128 pixels wide, 2 bits per pixel, is 32 bytes per row × 192 rows. Decoded on
that geometry the data draws a clean title screen — lettering, a hat and bow tie, a pistol,
a stick of dynamite, musical notes, palm trees, and the studio name. A single wrong byte
would show as four stray pixels, so this checks all 6144 bytes at once, independently of
the checksums. See `The Sppy TITLE.png`. (The palette there is provisional — the real
colours come from the `SCREEN`/`COLOR` setup in the main listing.)

### The machine-code DATA block (lines 3720–3930)
Line 3700 does `RESTORE3720 : FOR I=&HDA03 TO &HDB56` and pokes one byte per DATA item.

* The block holds **exactly 340 items, and `DA03`..`DB56` is exactly 340 bytes**.
* Every item is valid 1- or 2-digit hex. (The author writes a bare `0` for some zero
  bytes; `VAL("&H"+"0")` is 0, so those are correct as printed.)
* It enters `CD 41 07` (call ROM `0741`) and leaves `C3 AA 1A` (jump ROM `1AAA`).
* The only address it references inside its own page is `DA3B`, which lands **inside** the
  loaded block. The other three, `DA00`–`DA02`, sit just *below* it — the parameter bytes
  BASIC fills in before calling.

## Character encoding

Both `.bas` files are **Shift-JIS**. That matters: in Shift-JIS every halfwidth katakana is
a single byte in `A1`–`DF`, and those byte values *are* the PC-6001's own character codes —
so the file maps 1:1 onto the machine's character set. All 57 distinct non-ASCII characters
in the listing are single JIS X 0201 bytes; none needs a multi-byte code.

The five that a UTF-8 copy tends to trip over, checked against the PC-6001 character ROM
(`CGROM60.60`, 16 bytes per character — codes `A1`–`DF` are plain JIS X 0201):

| character | Unicode | PC-6001 code |
|---|---|---|
| `｡` | U+FF61 | `A1` |
| `｢` | U+FF62 | `A2` |
| `｣` | U+FF63 | `A3` |
| `､` | U+FF64 | `A4` |
| `･` | U+FF65 | `A5` |
| `ｰ` | U+FF70 | `B0` |

They are all genuinely present in the ROM at those codes, so if a tool refuses them the
tool's character table is at fault, not the transcription — entering the byte directly works.

## The 13 graphic characters in line 2100

Line 2070 reads 13 `DATA` items and prints each after `CHR$(&H14)`, the PC-6001's graphic
shift — so the items are ordinary characters that the machine renders as block graphics.
They were recovered by matching each printed glyph against the machine's own character
ROM, with the ink height used to separate small kana from full-size ones (the shapes are
identical once scaled, so only the height distinguishes `ォ` from `オ`).

`v` `シ` `ソ` `ァ` `オ` `「` `」` `P` `r` `ッ` `リ` `リ` `ァ`

Position 8 (`P`) was already known from the first transcription pass and came back out of
the matching independently, which is a useful check on the method. The full-height items
(`シ ソ オ P リ リ`) and the inherently short ones (`v 「 」 r`) are solid; **`ソ` (item 3)
and `ッ` (item 10) carry the most residual doubt** — both are shapes the printer renders
close to a neighbour. Running the game shows the drawn picture immediately, so those two
are worth an eyeball.

## How it loads

`The Sppy SAVER.bas` is the separate loader. In the main program, line 3640 does
`CLEAR 50,&HBFFF` — reserving everything from `&HC000` up — then `GOSUB 3700` to poke the
machine code. The title bitmap belongs at `&HC200`.

## Still open

No tape image yet. The title data and the machine code are both verified and ready; what
is missing is a PC-6001 `.cmt`/`.p6` carrying the BASIC program plus the two binary blocks.
