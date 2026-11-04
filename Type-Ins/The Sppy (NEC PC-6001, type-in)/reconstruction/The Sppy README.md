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

Both `.bas` files are **Shift-JIS (CP932)**, written with **fullwidth** kana (two bytes each)
and the dakuten/handakuten as separate characters (`ト゛`, `ス゜`), one character per PC-6001
screen cell. To get the machine's codes, map each fullwidth katakana or symbol to its JIS X 0201
halfwidth byte (`A1`–`DF`, which are the PC-6001's own codes; `゛` = `DE`, `゜` = `DF`) and each
hiragana to the PC-6001 hiragana code: the katakana code minus `20` for `ヲ`–`ソ` (`A6`–`BF` →
`86`–`9F`), plus `20` for `タ`–`ン` (`C0`–`DD` → `E0`–`FD`). The tape recording is the check:
every string literal on it converts back to exactly the text in `The Sppy.bas`.

The punctuation, checked against the PC-6001 character ROM (`CGROM60.60`):

| character | PC-6001 code |
|---|---|
| `。` | `A1` |
| `「` | `A2` |
| `」` | `A3` |
| `、` | `A4` |
| `・` | `A5` |
| `ー` | `B0` |

## The 13 graphic characters in line 2100

Line 2070 reads 13 `DATA` items and prints each after `CHR$(&H14)`, the PC-6001's graphic
shift — so the items are ordinary characters that the machine renders as block graphics.

`v` `シ` `ソ` `ァ` `オ` `「` `」` `P` `r` `シ` `ソ` `ソ` `ス`

The first pass matched each printed glyph against the character ROM and read items 10–13 as
`ッ リ リ ァ`. The 2026-11-04 recheck compared them instead with other characters printed by the
same printer in the same listing, which is a closer match than the ROM: item 10 is
pixel-identical to item 2 (`シ`, full height, not the short `ッ`); items 11 and 12 match the `ソ`
in item 3 and line 4020 and differ clearly from the `リ` of `アメリカ` in line 2260; item 13
matches the `ス` of line 3470 and not the short `ァ` of item 4. Running the game shows the drawn
picture (the microfilm), so it is still worth an eyeball.

## Corrections (2026-11-03)

Two transcription errors fixed in `The Sppy.bas` and, byte for byte, in the tape recording:

* **Function keys, lines 80–100.** `KEY1`..`KEY8` were transcribed in katakana (`"ウエ"` =
  PC-6001 `B3 B4`), but the command parser at `DA03` matches **hiragana** (`うえ` = `93 94`) —
  its table stores each command as `RRCA(first byte), CPL(second byte)`, and every entry
  decodes to hiragana. The magazine's printer showed hiragana as katakana; the article says
  the function keys work, so they must have been hiragana. Now `うえ した あけ とあ みる てる
  ひた みき`.
* **Line 300.** The second prompt `ト゛レ ラ` is `ト゛レ ヲ` (どれを, "which one"), as the article
  says — `ﾗ`/`ｦ` look alike in the print. Display only.

The tape was patched in place rather than rebuilt: the 17 changed bytes (offsets 179–286 and
929 of the decoded stream) had their 8 data-bit cells rewritten using 0/1 cell templates taken
from the recording itself, which reproduce all 14715 original bytes exactly. The patched WAV
re-decodes to the intended stream with no other difference. Originals are in
`work/pre-fix-2026-11-03/`.

## Corrections (2026-11-04)

A second, complete check of all 437 lines against the scan, cell by cell (each printed line
cut into its 15.93 px character cells and compared with the transcription for ink/no ink, and
doubtful glyphs compared side by side with the same character elsewhere on the page). Fixed
in `The Sppy.bas` and on the tape:

| Line | Was | Now (as printed) | Meaning |
|---|---|---|---|
| 670, 1050, 1510 | `ト゛ウシ゛ョウ ?` | `と゛うしよう ?` | どうしよう, "what shall I do?" — the command prompt. **Hiragana**: the article's screenshot shows this prompt on screen as `と゛うしよう ?` |
| 210, 3420 | `ト゛ウシ゛ョウ` | `ト゛ウシヨウ` | the same word; kept in katakana as printed, since no screenshot shows these two (210 is an `INPUT` prompt, 3420 the trapdoor's falling `ト゛ウシヨウ!`) |
| 2770 | `ヌクロ` | `ヌケロ` | `オリ ヲ ヌケロ !`, "get out of the cage!" |
| 2140 | `「アイ  ` | `「アイ」  ` | closing bracket was dropped: `アイコトハ゛ ハ 「アイ」`, "the password is 'Ai'" |
| 2260 | `ノ N.Y. ヘ トフ゛ !!` | `ノ  N.Y. ヘ  ト ヘ゛ !!` | とべ, "fly to N.Y.!" (imperative, a mission order); letter-spaced like `ハ ナ ヒ゛` in line 3260 |
| 2680 | `アカ゛ レナイ` | `アカ゛レナイ` | "broken, can't go up" |
| 2700, 3020, 3220 | `カキ゛ カ゛ ナイト、 ハ゛クハツスル。` | `カキ゛カ゛ナイト、ハ゛クハツスル。` | "without a key, it explodes" — no spaces in the print |
| 3630 | `ハ゛クハツ スル` | `ハ゛クハツ  スル` | spacing only |
| 3960 | `[ アト゛…ケ゛ーム ]` (31 columns) | `[  アト゛…ケ゛ーム  ]` (32) | the banner fills the screen line exactly |
| 3970, 3980 | 2 leading spaces | 3 | spacing only |
| 4000 | `  ア イナイニ` | `   7 イナイニ` | the glyph is the digit **7**: no inner stroke, identical to the 7s in the DATA lines, unlike all six `ア` on the page |
| 4010 | `ノ ニ アル。` | `ノ  ニ アル。` | spacing only |
| 2620 | `LINE(20,67)-(40,68)` | `(40,69)` | |
| 2100 | `r,ッ,リ,リ,ァ` | `r,シ,ソ,ソ,ス` | see the graphic-characters section |

Lines 4000–4010 now read `7 イナイニ ヤレ。 … ヒ゛ル / ノ  ニ アル。`. Both probably lost a
PC-6001 graphic kanji that the printer could not print: `7日 以内にやれ` ("do it within 7 days",
the article mentions next week's meeting) and `ビルの 中 にある` ("it is inside the building",
the article says "ジェミニ・ビルの中にある"). A printer that drops such characters without
leaving a cell would produce exactly the printed spacing in both lines. This is not
provable from the scan, so the listing follows the print and leaves them out.

Checked and correct as transcribed: line 2440 `PSET(55,140)` (the print shows `;`, a print
glitch — `;` would be a syntax error), line 3480 `PRINT"";`, line 1970 `マツワ` (待つわ).

Several fixes change line lengths, so this time the tape was **re-encoded** rather than
patched: the decoded stream (now 14710 bytes) was rebuilt with fresh line-link pointers and
resynthesised from the recording's own 0/1 cell templates and its alternating 3/4-stop-bit byte
framing, which regenerate the original recording's program section sample for sample. The
header and the trailing tone are copied unchanged. The new WAV re-decodes to exactly the
intended stream, and every string literal on it matches the `.bas`. Originals are in
`work/pre-fix-2026-11-04/`.

## How it loads

`The Sppy SAVER.bas` is the separate loader. In the main program, line 3640 does
`CLEAR 50,&HBFFF` — reserving everything from `&HC000` up — then `GOSUB 3700` to poke the
machine code. The title bitmap belongs at `&HC200`.

## Status

Ships as WAV cassette recordings in `game.zip`: the saver, the game (`CLOAD"BASIC"`, 3 pages,
`RUN`) and the title screen. Not yet loaded in an emulator since the 2026-11-04 re-encode.
