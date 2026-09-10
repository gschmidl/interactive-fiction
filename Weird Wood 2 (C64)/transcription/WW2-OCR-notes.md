# Weird Wood II — notes on the transcription

## What the source is

`1987 ww2 original.pdf` is a 16-page fanfold printout of the Commodore 64
BASIC listing of **Weird Wood II**, dated by hand *21 May 1987*. Line 5 of the
program says `REM LAST UPDATE (30:7:85)`.

`1987 ww2 original 2.pdf` is a single sheet, scanned both sides: the front is a
printed table of the game's 66 objects, the back carries the "problems in red"
notes.

The listing runs from line 5 to line 60075 — **992 BASIC lines on 993 printed
lines** (line 497 wraps onto a second printed line because its string contains
an embedded shifted-RETURN). Every page holds exactly 66 printed lines except
the first (59, the listing starts partway down) and the last (10 plus `READY.`).

## The character conventions — read this first

### Case is inverted

The printout was made through a printer interface that converts PETSCII to
ASCII by **swapping the two letter ranges**:

| PETSCII | screen (C64 in lower/upper mode) | printout |
|---|---|---|
| `$41`–`$5A` (unshifted) | lowercase `a`–`z` | **uppercase** `A`–`Z` |
| `$C1`–`$DA` (shifted)   | uppercase `A`–`Z` | **lowercase** `a`–`z` |
| `$A0`–`$BF`, `$DB`–`$FF` (graphics) | graphic | `c-128` |

So the string printed as

```
tHERE IS A TORCH HERE.
```

appears on the C64 as **`There is a torch here.`** To read any string in the
listing the way a player saw it, invert the case of its letters. BASIC
keywords and variable names were typed unshifted and therefore print
uppercase, which is why they look normal.

This is not a guess. Two things prove it:

* Line 485 is `PRINTCHR$(ASC(VB$)+128);MID$(VB$,2)" WHAT?"` — adding 128 to the
  first letter of a verb is exactly how you capitalise it on a C64 in
  lower/upper mode, so the stored words must display lowercase.
* Lines 3550–3558 draw a box whose corners print as `0`, `.`, `-`, `=` and
  whose sides print as `@` and `]`. Those are PETSCII 176/174/173/189/192/221
  minus 128 — the standard C64 box-drawing set.

The transcription keeps the case **exactly as printed**, because that is a
faithful, reversible record of the underlying PETSCII.

### `#` prints as `£`

The printer used a Norwegian character set, in which position `0x23` shows as
`£`. Everywhere the paper shows `£` the byte is `#`: `INPUT#1`, `OPEN8,8,8`,
`PRINT#15`, `IFA$="#"`, `REM #1`. The transcription writes `#`.

(Consequently a real PETSCII `£` (`$5C`) would have printed as `\`. None
appears in the listing.)

### Box-drawing and graphics characters

Transcribed as the literal glyph that was printed, so the byte is recoverable:

| printed | PETSCII | on screen |
|---|---|---|
| `0` | 176 | `╭` top-left |
| `.` | 174 | `╮` top-right |
| `-` | 173 | `╰` bottom-left |
| `=` | 189 | `╯` bottom-right |
| `@` | 192 | `─` horizontal |
| `]` | 221 | `│` vertical |

One exception: the three cell separators in the star-game grid (lines 23644,
23648, 23654) print as a faint short bar that could not be pinned down, and
have hand-drawn pen tracing over them. They are written `|` in the
transcription — the column positions are measured and correct, but the exact
byte is **not** established. Almost certainly PETSCII `│`.

### Non-printing characters inside strings

Four strings contain a control character that consumed **zero print columns**,
so nothing is visible between the quotes:

* line 808 `PRINTMID$(CU$,K+1,1)"";` and line 828 `PRINT"";` — from context a
  cursor-left, `CHR$(157)`, in the input/delete routine.
* lines 23572 and 23576 `PRINT""` — this one is identifiable: it switched the
  printer into double-width for line 23576 (visible on the paper), and printer
  code 14 is double-width, so the byte is `CHR$(142)`, upper-case/graphics
  mode.

These are written as empty strings `""`. Line 23616 does the reverse in the
open, `PRINTCHR$(14)""`, returning the C64 to lower/upper mode.

Line 497's string contains an embedded shifted-RETURN (`CHR$(141)`), which is
why it occupies two printed lines. It is transcribed across two lines to match
the paper.

## How the transcription was checked

Four independent checks, all of which the final text passes:

1. **Line inventory.** The printed lines were located by a phase-locked
   projection at the 50-pixel line pitch, giving an exact count per page. The
   transcription has the same count on every page (59 / 66×14 / 10). One line
   — `305 DR=0` — was caught this way after an earlier pass merged it into its
   neighbour.

2. **Branch targets.** Line numbers are strictly increasing, and all **311
   distinct `GOTO`/`GOSUB`/`THEN` targets resolve to a line that exists**. A
   single misread digit anywhere in a line number or a jump would almost
   certainly have produced a dangling reference.

3. **Character grid.** The listing is monospace at 17.5 px/char. Every line was
   fitted to that grid and its per-cell ink compared against the transcription,
   which is what settles the multi-space runs. Five gaps were one space short
   and were corrected. `REM*********` on line 795 was corrected to eight stars.

4. **The 40-column rule.** The C64 screen is 40 columns wide, and the author
   padded his strings to it by hand. Every corrected gap lands exactly on a
   multiple of 40, and so do all the run-together words — `DWARFAPPEARS`
   (col 40), `THEDOOR` (col 40), `HEARTHE`, `YOUSOME`, `ASYOU`. These are not
   typos and not missing spaces: they are where the text wrapped on screen.

Two further corroborations came for free:

* The twelve red-pen notes on the back of the object sheet quote twelve
  specific lines of the program (448, 460, 728, 762, 763, 1008, 1010, 1921,
  2007, 2912, 4010, 7011/7012). All twelve match the transcription exactly.
* The handwritten `EXAMINE`-handler line numbers on the object sheet match the
  `ON…GOTO` dispatch in lines 3005/3006/3313/3314, and the text each handler
  prints matches the object it is claimed to belong to.

## Remaining uncertainties

Small, and all flagged in place:

* **Line 103** is struck through by hand. `CU$="& "` — the `&` was confirmed by
  the file's owner; the rest of the line reads cleanly. `&` is corroborated by
  line 23580, which uses `&` as the on-screen prompt cursor.
* **Line 3064** `IFA$="              "THENA$=A$+M$+"]!` — 14 spaces measured on
  the grid; a handwritten `?` sits over that part of the line, so the count is
  from the column measurement rather than direct reading.
* **The star-grid separator** in 23644/23648/23654, as above.
* Whether the long blank runs (line 23568's 35 characters, the 26 in
  23642/23646/23650, the 18 inside the security-pass box) are spaces (`$20`)
  or shifted spaces (`$A0`) **cannot be determined from a printout** — both
  print as blank. On screen `$A0` is a solid block, so this does change what
  the star-game background looks like.
* Line 32300's constant is unambiguously `80`, but what object name it is
  meant to catch is not obvious; the neighbouring tests (78/72/79) strip
  "AN ", "THE " and "SOME " respectively.

## Bugs the annotator marked, confirmed in the text

* **18001** `IFOJ=10THEN18190` — object 10 is the door, 19 is the desk; the
  LOCK handler can never reach the desk branch. Marked "BUG lock desk".
* **60067** `IFLC%(62)=0THEN60067` — jumps to itself, an infinite loop.
  Marked "CHIP?".
* **36000/36010** — the EMPTY handler prints "yOU CAN'T FILL". Marked "err!".
* **35570/36570** — both build a message in `A$` and then `GOTO199` without
  ever printing it. Marked "err!".
* **432** `IFJ2=OJTTHEN…` — reads as `IF J2 = OJT THEN`. Harmless in practice,
  since C64 BASIC only distinguishes the first two characters of a variable
  name, so `OJT` *is* `OJ`. Marked "BUG!".
* **760** `T$(T)=T$(T):T=T+1:…` — a self-assignment that does nothing. Not
  marked, but it is what the paper says.

Original typos left as-is: "ATEMPT" (19591), "COFINEMENT" (34011, 34101),
"POSSIBILITES" (371), "ALL READY" for "already" (35570, 36570).

## Files

| file | what it is |
|---|---|
| `WW2-listing.txt` | the BASIC listing, 993 lines, pages 1–16 |
| `WW2-objects.txt` | the 66-object table from the second PDF |
| `WW2-annotations.txt` | every handwritten note, by page |
| `WW2-OCR-notes.md` | this file |

`WWDAT.TXT` and `WWSOLUTION.TXT` in the same folder belong to the later
reconstruction, not to this 1987 listing, and were **not** used as a source for
any of the above. They are consistent with it where they overlap, which is
worth noting but is not evidence either way: the sausage-roll hint
("try room 477", line 3541), the navigation code 8181 (3612), the blue cable
(27510), and the `#`/`@`/`$` markers that `WWDAT.TXT` uses are exactly what the
reader at lines 665–690 expects.
