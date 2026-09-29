# HOUSE ADVENTURE — NEC PC-8001 / PC-8001mkII

Recovered from a magazine type-in listing (scan supplied by the user).

| | |
|---|---|
| Title | HOUSE ADVENTURE, Ver1.2 |
| Dated | 1983年 6月 19日 |
| Credited | Programed by J.M & H.M (the title screen says "PC Penguins") |
| Machine | NEC PC-8001 / PC-8001mkII, N-BASIC (the listing is headed 《PC-8001/mkII》) |
| Source | マイコン (My Computer) 1983年10月号, pp. 364–373 |
| Listing | PDF pages 3–10 (pages 1–2 are the article, the command table and screenshots) |

This is the **original** of the game in `House Adventure (Fujitsu FM-7, type-in)`: the FM-7 article
calls itself a conversion of this listing. The author says it was inspired by the MZ-80K
*MYSTERY HOUSE T.S2* in the June issue.

It is not a PC-8801 N88-BASIC program: coordinates are the PC-8001's 160×100 graphics, and it uses
`CONSOLE`, three-argument `COLOR`, `GET@`/`PUT@`, `INP(9)` and `OUT81`.

## Files

* `houseadv.txt` — **for DumpListEditor.** Raw PC-8001 character codes, CRLF, 635 lines. Load it with
  File ▸ *Textファイル読み込み* (the plain one, not the S-JIS/UTF-8 conversions) with the model set to
  PC-8001. It is the same form DLE's *Textファイル書き出し(マシンコード)* writes.
* `House Adventure.bas` — the same listing as readable text: UTF-8 with BOM, CRLF, halfwidth katakana,
  and Unicode stand-ins for the graphic characters (table below).
* `work/pc8001_encode.py` — converts the `.bas` to `houseadv.txt` and checks the round trip.
* `work/ocr/` — the OCR and verification pipeline (below); `edits.py` lists every hand correction.
* `src_original/` — the source PDF.

## Graphic characters

| in `.bas` | PC-8001 code | where |
|---|---|---|
| █ | `87` | REM lines 20/30, TIME banner 6070–6110 |
| ◢ ◣ ◥ ◤ | `E4 E5 E6 E7` | the M of the TIME banner |
| 年 月 日 | `F2 F3 F4` | lines 50, 390 |
| 時 分 秒 | `F5 F6 F7` | line 6120 |

The codes were read off the machine's own character generator (the 8×8 ANK font at 0x1000 in the
PC-8801 KANJI1 ROM, which is the PC-8001 set; `work/proof/ank8.png`). Every rectangular block, including
the ones in REM 20/30, prints at full cell height, so all are `87`. The M's diagonals really are four
different triangle characters (`work/proof/zoom_banner.png`).

## How it was recovered and checked

The scan is a clean 600 dpi printout, so it was OCR'd rather than typed:

1. Pages deskewed; printed rows found from the line-number column (digits have no descenders); a common
   character pitch plus a per-row phase fitted to the glyph centres (resultant length ≈ 0.99 on every
   page). 658 printed rows, all 80 columns wide.
2. Glyph cells clustered and the clusters labelled → rough text; BASIC grammar repaired keywords and
   numbers.
3. Every cell re-classified against nearest exemplars trained only on trusted cells (DATA digits, line
   numbers, keywords). Ink-vs-blank is checked for every cell, so spacing is exact.
4. Confusable pairs decided by measured features instead of templates: O vs 0 by the flat top of O,
   period vs comma and semicolon vs colon by ink height, small vs full-size kana by height.
5. Every row containing a string, REM, kana or lowercase (177 rows), every row with a flagged cell,
   and every code line that differs from the FM-7 port was proofread against the scan, with the text
   drawn on the same cell grid.
6. 555 of the 635 lines match the FM-7 transcription once its ×4/×2 graphics scaling is removed and
   spacing is ignored (476 exactly, spaces included). The spacing itself comes from the ink check in
   step 3. Two independent listings agree.
7. Structure: line numbers 10–6350 strictly increasing; every GOTO/GOSUB/THEN/ELSE/ON…GOSUB target
   exists; every RESTORE points at a DATA line.

## Things in the original worth knowing

* **Kept as printed:** `ADVENTUER` (line 380), `EO=14` where `RO=14` is meant (5270), `ﾀﾞｯｼｮﾂ` with a
  small ｮ (6040; 6130 has `ﾀﾞｯｼｭﾂ`), `COLORTMOD6+1` (6200).
* **No closing quote** on lines 3780–3910, 3950–4060 and 5830. That is really how they print (the cell
  after the text is blank), and BASIC accepts it.
* **Printer wraps** at 80 columns; long lines (e.g. 3590, 3740 over four rows, 5030 over three) were
  rejoined. Two specks on the page (right of 4230, right of 5770) are not part of the program.
* **DATA where the FM-7 port differs:** 580 has `36` (FM-7 `38`), 1280 `12,28` (FM-7 `12,29`),
  1580 `2,64` (FM-7 `2,84`). The PC-8001 scan is unambiguous on all three.
* **`ｰ` vs `-`:** this printer draws the ASCII hyphen and the katakana long-vowel mark identically.
  `ｰ` is used after a kana (ﾃｰﾌﾞﾙ, ｹﾞｰﾑ, ｷﾞｬｰ, ﾔｯﾀｰ …) and `-` everywhere else (`GOOD-BY`, code). This is
  a reading of the text, not something the scan shows.
* Lines 6340–6350 are machine code POKEd to &HE200 and called with `DEFUSR`/`USR(0)` when you quit.

Status: transcription complete and verified. Not yet run on an emulator.
