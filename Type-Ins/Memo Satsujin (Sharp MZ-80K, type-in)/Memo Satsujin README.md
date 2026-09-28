# メモ殺人 (MEMO SATSUJIN) — Sharp MZ-80K / MZ-1200

Recovered from a magazine type-in listing (scan supplied by the user).

| | |
|---|---|
| Title | MEMO ｻﾂｼﾞﾝ (Memo Murder) |
| Machine | Sharp MZ-80K / MZ-1200, SP-5025 BASIC |
| Source PDF | `src_original/Memo Satsujin (MZ-80K).pdf`, listing on pages 2–11 |

MZ's friend TRS is murdered; MZ visits the house to find the memo TRS wrote naming the
culprit. The game is built from numbered room-display subroutines (10000–11421) with a
title screen at 9300 and an END screen at 14001.

## Files

* `Memo Satsujin.bas` — the listing, 936 lines, range 10..14034, plus lines 9022, 9025 and 9290
  (see "Changes to the original" below)
* `memosat.txt` — the same program as raw MZ-80K bytes (one byte per character, CRLF), the
  form DumpListEditor loads
* `Memo Satsujin.backup-2026-09-27.bas`, `memosat.backup-2026-09-27.txt` — the version before
  the art review of 2026-09-27 (see below)
* `Memo Satsujin.mzt` — `memosat.txt` attached to SP-5030A BASIC as one tape image (made with
  DumpListEditor): loading it starts the game, and it starts again when the game ends
* `Memo Satsujin walkthrough.md` — how to find the memo and name the culprit
* `work/pages/pNN.txt` — the per-page transcriptions it was assembled from
* `work/art_blocks.json` — the 56 art blocks mapped to scan pages
* `src_original/` — the source PDF

## Verification

* **936 program lines, 10..14034** (939 with the added lines 9022, 9025 and 9290). No duplicates, no
  out-of-order line numbers, quote balance clean on every line.
* **Every GOTO / GOSUB / THEN / ON..GOTO target resolves to a line that exists** — 0 bad
  targets.
* Checked against the two confusions that survived every structural test on this batch
  (see the working script `confusecheck.py`, not published): **0 hits**. Those are lowercase `l` read as digit
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

## Art

The pictures (fourteen rooms 10000–11421, the title 9300–9322, the END screen 14001–14034 and
the animated scenes 3140–7222) were decoded from the scan on 2026-09-13 and hand-corrected.
On 2026-09-27 every art line was checked again, cell by cell, against a model of how the
listing's printer draws each character. What the scan shows about that printer:

* **Graphics print in 7 dot rows, the screen font has 8.** The eight horizontal bars
  `￣ ⑦ ⑥ ⑤ ━ ③ ② ▁` (screen rows 0–7) print on rows 0, 1, 2, 3, 3, 4, 5, 6: only `⑤` and `━`
  share a row. Staircases that step through all eight rows (9005, 9010, 10111, 10114) show
  it directly. Every other bar is read straight off the scan; for `⑤`/`━` the join decides
  (`━` meets ┣┫┳┻ and ╭╮, `⑤` meets ╰╯), otherwise the choice that gives the staircase even
  steps.
* **Eight dot columns.** Vertical strokes are placed by the average over the whole run of a
  line, since neighbouring columns are only about 2 pixels apart on the scan.
* **Diagonals print corner to corner**, unlike the text `/` of "I/O" in line 6628, so the art
  diagonals are `╱` `╲` (the program also POKEs these codes).
* **Block glyphs:** `▀` prints 3 rows, `▄` 4, `═` and `▂` 2 each.
* **Closing quotes** show where each string ends, which fixed string lengths (line 3565 also
  lost a stray `":` that had left its `GOSUB7300` inside a string, and the eraser in 5485 is
  two spaces for the two-cell `◥◤`).

* **The printer is not the screen.** Glyphs that print on almost the same spot are a pixel apart
  on the MZ-80K screen: `▕` and the next cell's `▏`, and `⑺`/`▕`/`⏋`/`⏌` (all within 0.7 dots on
  paper). Where the transcription mixed them inside one line, the screen showed one-pixel jogs
  and doubled strokes. A second pass drew every picture and every open-this scene through the
  MZ-80K character ROM and gave each line that is straight on the listing one position on the
  screen (the corridor's door frames and knobs, the kitchen, the cupboard, the house door on the
  first screen and more). Steps the listing itself shows were kept: the sloping bottom of the
  kitchen's wall cabinet, frames that stand a pixel outside their lintels (the author's own POKEd
  start door is drawn that way), a wall line that steps aside where a floor diagonal takes its
  cell, and the TRS figure's head on the title.
* **Cells the first decode lost:** a check of every blank art cell against the scan found wall
  and door strokes missing in a few rows (rooms 10200, 10600, 10700, the title's wall and TRS
  figure, the house window on the first screen, the cupboard scene 4020).
* **Rounded or square corners:** the printer puts the strokes of `╭╮╰╯` where `┏┓┗┛` have theirs,
  so only the corner itself shows which it is: a rounded one leaves the angle open. Read that way,
  the kitchen's left-wall windows, the TRS figure's head, a cabinet in room 11300, the cups in the
  cupboard and two boxes on the END screen are rounded; no object now mixes the two kinds.
  Every picture and every frame of every open-this scene was checked on the screen font.

Three changes are deliberate and not readings of the scan: the posters in rooms 10200 and 10300
have a single right edge (the listing draws it double, `▕▏`, which breaks up where text sits in
that column), and the kitchen bucket tapers evenly with a rounded bottom (the listing has `⑵`
left and `⑹` right, one and two pixels in, and a square `┗━┛` bottom).

The review changed 380 lines (about 1190 cells); `work/review-2026-09-27/changes.txt` lists
them. Outside the pictures only these touch code: 3565 and 4100 (a misplaced quote each), 10017
(its closing `";` had lost the `;`, so the 40-character line wrapped and left a blank row through
the picture), and the two changes below (120, 9030, new 9022, 9025 and 9290). The examine animations
(5170, 5270, 5420, 5470, 5485, 5760) erase their two-cell arrow with two spaces as the listing
does; with one space the arrow's tail stayed on the screen. Stand-ins that are deliberate choices
rather than readings of the scan: `■` for the checker texture of walls and furniture (the printer
draws it as a checker, `░` kept for decorative texture), `◢◣◥◤` for the printer's checkered
triangles and `▌` for its half-width checker, none of which the Japanese MZ-80K font has.

## Changes to the original

### The memo's hiding place

The listing picks the memo's hiding place, and the code TRS left on the title picture, at
`GOSUB8000` straight after RUN, before any key is pressed, and it never seeds `RND`. Loaded
fresh, for example from the BASIC-plus-program image that DumpListEditor builds, every game
therefore hides the memo in the same place with the same code. The place is meant to be random:
line 8020 draws one of the 42 places in the DATA list, and the title's code is made from its name.

Now `RND` runs while the `[Y/N]` question at 9030 waits, and the new line 9290 picks the memo
after the answer, just before the title is drawn, so the player's own timing makes every game
different. Run on an emulated MZ-80K with the DumpListEditor image, the place does follow the
length of that wait (1 to 80 polls of the keyboard gave 16 different kinds of place). A key that
is already down or buffered when the question appears would be taken at the first poll, every
time, so line 9025 first waits for the keyboard to be clear, and 9290 also moves `RND` on by
the seconds of the MZ's clock. 9025 is written `IF(A$="")=0`, the listing's own way: the MZ's
BASIC stops with a SYNTAX ERROR on `A$<>""`. The original lines were:

    120 GOSUB8000:GOSUB9000:IFA$="H"GOTO280
    9030 GETA$:A=-1*((A$="Y")+(A$="N")*2):ON A+1 GOTO9030,9100,9300

### Kana entry after TRY AGAIN

The game switches the keyboard to kana itself before every command (`POKE4464,1` in 210 and
1010; 4464 is the monitor's kana flag), and TRY AGAIN at the end is answered with the kana on the
Y and N keys (ﾝ, ﾐ). It never switches kana off, so after `RUN` (13020) the `[Y/N]` and `[S]`/`[H]`
questions of the opening got kana and waited for a key they could not get. DumpListEditor's chained
image re-runs the program after END, so it ran into the same. The new line 9022 `POKE4464,0`
switches kana off before those questions.
