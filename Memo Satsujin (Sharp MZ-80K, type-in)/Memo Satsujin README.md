# メモ殺人 (MEMO SATSUJIN) — Sharp MZ-80K / MZ-1200

Recovered from a magazine type-in listing (scan supplied by the user).

| | |
|---|---|
| Title | MEMO ｻﾂｼﾞﾝ (Memo Murder) |
| Machine | Sharp MZ-80K / MZ-1200, SP-5025 BASIC |
| Source PDF | `src_original/Game 2 (MZ-80K).pdf`, listing on pages 2–11 |

MZ's friend TRS is murdered; MZ visits the house to find the memo TRS wrote naming the
culprit. The game is built from numbered room-display subroutines (10000–11421) with a
title screen at 9300 and an END screen at 14001.

## Files

* `Memo Satsujin.bas` — the listing, 936 lines, range 10..14034
* `work/pages/pNN.txt` — the per-page transcriptions it was assembled from
* `work/art_blocks.json` — the 56 art blocks mapped to scan pages
* `src_original/` — the source PDF

## Verification

* **936 program lines, 10..14034.** No duplicates, no out-of-order line numbers, quote
  balance clean on every line.
* **Every GOTO / GOSUB / THEN / ON..GOTO target resolves to a line that exists** — 0 bad
  targets.
* Checked against the two confusions that survived every structural test on this batch
  (see `_typein_work/confusecheck.py`): **0 hits**. Those are lowercase `l` read as digit
  `1` inside MML, and capital `O` read as `0` in `OR` — both produce perfectly valid BASIC,
  so nothing but running the program or this scan catches them.
* **Row mapping validated on all 10 pages** by decoding each row's *printed* line number
  and aligning the sequence against the transcription. Every page resolves to its exact
  line count. This matters: it is the check that caught a one-row offset in the sister
  game, and it is the prerequisite for any art work.

## Character encoding

**UTF-8 with CRLF, and that is the correct format for this game** — verified in
DumpListEditor: every MZ character in the listing pastes in, "Adjust Code" leaves them
untouched, the Check button passes, and they assemble to the expected bytes.

This differs from House Adventure and The Sppy, which had to be Shift-JIS. Six of the
characters used here (`Ⓒ ║ ╱ ╲ ▁ ░`) have **no Shift-JIS encoding at all**, so converting
would lose them. DLE accepts the Unicode forms directly, so no conversion is wanted.

## Control characters

These print as **inverse letters** in the listing, which is why they read as solid blocks
on the scan. Resolved against the scan:

| character | MZ code | meaning | count |
|---|---|---|---|
| `下` | 0x01 | cursor down | 38 |
| `Ⓒ` | 0x06 | clear screen | 8 |
| `Ⓗ` | 0x05 | home | 1 |
| `▂` | 0xCF | the understrike used inside `MUSIC` strings | 1 |

## Still open

**The art pass — 639 `{ART}` placeholders across 569 lines in 56 blocks.** The bulk is
fourteen room-display subroutines (10000–11421) of ~22 lines each, plus the title screen
(9300–9322) and the END screen (14001–14034).

Nothing has been written into those placeholders. They are honest markers, not guesses —
this listing carries no checksum, so the only real check on recovered art is rendering it
back as a picture and comparing with the scan.
