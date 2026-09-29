# HOUSE ADVENTURE — Fujitsu FM-7 / FM-8

Recovered from a magazine type-in listing (scan supplied by the user).

| | |
|---|---|
| Title | HOUSE ADVENTURE, Ver 1.2 |
| Dated | 1983年10月14日 |
| Credited | Programed by H.KAWAGUCHI & TANKENTAI, SANKO PAAKONBU |
| Machine | Fujitsu FM-7 / FM-8, F-BASIC |
| Source | マイコン (My Computer) 1983年12月号, pp. 384+ |
| Listing | PDF pages 3–10 (pages 1–2 are the article) |

Conversion of PC Penguin's PC-8001 *HOUSE ADVENTURE* (マイコン October issue). The article
also cites MZ-80K/1200 *MYSTERY HOUSE T.S2* (マイコン June issue) — which is another game in
this same batch — and ミステリーハウス by マイクロキャビン.

The FM port scales the PC-8001 screen by ×4 horizontally and ×2 vertically (hence the
`*4` / `*2` in every LINE statement); everything is drawn in green except the title.

## Files

* `House Adventure.bas` — the listing, **Shift-JIS (CP932)**, 635 lines, range 10..6350
* `work/pages/pNN.txt` — the per-page transcription it was assembled from
* `House Adventure.t77` — **working FM-7 tape** (built by the user from this listing)
* `House Adventure 2.bas` — the user's working copy used to build the tape; identical in
  content to `House Adventure.bas`, it differs only in line endings
* `src_original/` — the source PDF

Status: **finished** — the tape loads and runs.

## Verification

* 635 program lines, 10..6350; no duplicates, no out-of-order line numbers
* **Every GOTO / GOSUB / THEN / ON..GOTO target resolves to a line that exists** —
  including the 19-way `ON RO GOSUB` at line 3590 that dispatches the room routines
* Quote balance clean on every line
* The in-game object list (lines 3950–4060) matches the article's own object table
  exactly — an independent check on that stretch of text

## Things in the original worth knowing

* **Printer wraps.** Long lines continue on the next printed row. Lines 390, 1790, 1850,
  1940, 2220, 2640, 2660, 2680, 2850, 3500, 3590, 3610, 3740, 4100, 4230, 4300, 4350,
  4720, 4770, 5030, 5140, 5190, 6110, 6120 all wrap and have been rejoined. They are not
  separate program lines.
* **Line 4870 is split across PDF pages 8 and 9** (ends `THEN`, continues `GOSUB6250`).
* **A misprinted line number.** Page 8 prints `4960` where the sequence requires `4860`;
  page 9 has its own genuine `4960`, which confirms it. Transcribed as `4860`.
* **The author's name is inconsistent in the original**: line 70 says H.KAWAGUCHI, line
  390 prints H.KAWAKAMI. Both left as printed.
* **Line 2370** prints `17*5` and `17*4` where every neighbouring coordinate uses `*2`.
  Left as printed — it may be an original typo, but it is what the magazine shows.

## Graphics characters

The listing uses FM-7 semigraphic characters in three places. The FM-7 keeps those in a
machine-specific code range that has no Shift-JIS equivalent, and no FM-7 character table
was available, so they are written with stand-ins that *are* Shift-JIS-safe:

| stand-in | Unicode | Shift-JIS | stands for |
|---|---|---|---|
| ■ | U+25A0 | `81 A1` | a solid block cell |
| ▲ | U+25B2 | `81 A3` | the half-tone cell forming the M's diagonal |

**Each stand-in is one FM-7 character even though Shift-JIS stores it as two bytes.**
Substitute the real FM-7 codes when typing in, and the column counts below will be right.

### The title box (lines 10–30)

Line 10 is 23 asterisks, so the box is 23 cells wide. Fitting that grid on the scan
(pitch 65.7 px at 1200 dpi, first asterisk centre x = 1813) places the decoration exactly:

* line 20 — `*` at cell 0, **one** block at cell 3, `HOUSE` at cells 13–17, `*` at cell 22
* line 30 — `*` at cell 0, `ADVENTURE` at cells 9–17, **two** blocks at cells 20–21, `*` at cell 22

Both lines come to 21 cells between the asterisks, which is what the 23-asterisk rule
requires. Rendering the cells on the printer's own dot grid shows line 20's glyph fully
inked across its 8×8 cell, so it is a solid block; line 30's pair measures 2.07 cells wide
and is read the same way.

### The END banner (lines 6070–6110)

These draw the word **TIME** in FM-7 block graphics. The column positions were measured
off the scan on the fitted character grid, so the layout is right; only the two character
codes need substituting.

## Machine code

Lines 6340–6350 are a DATA block loaded by line 110 to &HE200..&HE213 (20 bytes).
