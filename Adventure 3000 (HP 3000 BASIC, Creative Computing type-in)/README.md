# ADVENTURE/3000 — Creative Computing, November 1979

ADVENTURE/3000 version 3.2 (27 FEB 1979) by **Benjamin Moser**, James Madison
High School, Vienna, Virginia: a 1,003-statement HP/3000 BASIC adventure
printed as a type-in listing in *Creative Computing* vol. 5 no. 11
(November 1979), pages 108–139, together with dumps of its four data files and
a sample run.

This folder rebuilds the program **and** its data files from the magazine
scans, with every character machine-checked against the printed page.

## What is here

| path | what it is |
| --- | --- |
| `port/` | the playable game: the listing, its data, and an HP 3000 BASIC interpreter to run them (see `port/README.md`) |
| `transcription/ADVENTURE3000.BAS` | the program, one statement per line, lines 10-9960 |
| `transcription/ADVENTURE3000_fixed.BAS` | the same with the four documented corrections applied |
| `transcription/listing_as_printed.txt` | the listing exactly as printed, column for column (continuation rows, `&` joins and all) |
| `transcription/data_print/` | the four data files as the magazine prints them: 35 items, 100 short descriptions, 569 message records, 100 movement rows |
| `transcription/data_play/` | the same with the documented corrections |
| `transcription/fixes.txt` | every correction, with the reason for it |
| `transcription/doubts_listing.txt`, `doubts_data.txt` | every uncertain reading, what settled it, and the original's own defects |
| `transcription/nd_diff.txt` | full diff of the printed data files against the `nd` copies |
| `transcription/tools/` | the OCR and verification pipeline (see below) |
| `transcription/tools/truth.txt`, `truth_data.txt` | the transcription itself, keyed by row position on the page |
| `hp3000/` | the scripts that put the listing into a real BASIC/3000 and recorded what it did |
| `scans/fetch.sh` | downloads the page images from archive.org |

The page scans (about 620 MB) and the pipeline's intermediates (ink maps, cut
glyph cells, proof sheets; about 2.9 GB) are not in the repository: `fetch.sh`
brings the scans back, and everything else is regenerated from them by the
tools.

## Sources

The PDF that archive.org serves is a ~200 dpi derivative; the dot-matrix
listing is *not* reliably readable in it (the `S`/`5`, `8`/`B` and `,`/`.`
distinctions collapse). Everything here was read from the full-resolution
scans instead:

| id | what | resolution |
| --- | --- | --- |
| `creativecomputing-1979-11` | lossless TIFFs inside the 8.37 GB CBZ (primary) | 600 dpi |
| `CreativeComputingbetterScan197911` | second paper scan | 600 ppi |
| `sim_creative-computing_creative-computing_1979-11_5_11` | microfilm | 800 ppi |

Page numbering: printed page = leaf − 3 = full-PDF page − 4.
Article pp.108–110, data dumps pp.112–124 (even), program pp.126–138 (even),
sample run p138 right column–p139.

## How the transcription was verified

The pipeline treats the page as what it is — a monospaced dot-matrix printout
pasted into a magazine column — and checks the transcription against the paper
rather than trusting any OCR:

1. **`pagegeom.py`** loads a scan, deskews it and builds an ink map
   (grey-closing background estimate, soft threshold).
2. **`geom2.py`** finds the text rows and, per row, the character cells:
   pitch ≈36 px, line pitch ≈62 px, fitted by folding the ink profile, then a
   dynamic program places cell boundaries; cell 0 is anchored on the
   right-aligned line-number field so drift can't shift a row.
3. **`cells.py`** cuts one 42×64 image per character cell.
4. **`truth.py`** aligns the typed transcription to those cells (DP, with glyph
   scores), so every character is bound to a position on the page.
5. **`classify.py`** learns glyph templates from the aligned samples
   (k-means sub-templates, NCC with ±2 px shifts).
6. **`verify.py`** re-classifies every page with templates trained on the
   *other* pages only (leave-one-page-out) and reports any cell where the
   glyph evidence disagrees with the transcription.
7. **`spacing.py`** checks the typed spacing against the printed columns —
   every gap inside a string, a REM or a data record.
8. **`targets.py`** resolves every GOTO/GOSUB/THEN/ELSE/OF-list/RESTORE/
   ON END/CONVERT target against the statement table.
9. **`wit.py` / `witsub.py`** crop the same spot from all three scans for
   comparison; **`dotcheck.py`** tests a single cell against candidate glyphs
   using the fact that a worn printer only ever *loses* dots.

Final state:

| check | listing | data dumps |
| --- | --- | --- |
| rows aligned | 1038, 0 failures | 703, 0 failures |
| statements / records | 1003 statements, lines 10–9960 | 35 + 100 + 569 records, 100 movement rows |
| spacing mismatches | 0 in 5317 gaps | 0 in 20576 gaps |
| leave-one-page-out glyph flags | 69, all reviewed by eye | 53, all reviewed by eye |
| line references | 423 resolved, 0 missing | — |

## The original's own defects (kept as printed)

Typos in the printed source and data, confirmed on all three scans:
`DESCRIPITIONS` (7810), `CRSTAL` (1350), `VALAUBLES` (6480), `"Enter you gripe"`
(9420), and in the data `overlloking`, `litle`, `fierece`, `tweleve`, `celing`,
`throught`, `persioan`, `dessend`, `ath`, `stiflingg`, `vansihing`, `inot`,
`psoted`, `tthe`, `imbedded`, `enbtrance`, `alabastaer`, `myraid`, `roff`,
`wwest`, `sya`, `hgher`, `avery`, `paht`, `followind`, `aflarge`, `misth`,
`orage`, `You'reein`, `You are.in`, and `"fee fie foe foo" [sic].`
Two rooms print with a character missing altogether (`small  it.`,
`soft  oom.`) and one description reads `the pii.` for "the pit."

Genuine bugs (kept as printed, corrected only in the playable build):
`7580 S[28]=0` zeroes the velvet pillow where the smashed ming vase (`S[6]`)
was meant, `6070 S[17]=0` clears the oil when you drink the water (`S[16]`),
and `5980`'s `B0=0` empties your bottle when you eat the food.  Line 4100's
`TTLE)` is hand-lettered in the magazine.  Statements 1450 and 5050 are
unreachable.

Lines 15 and 16 of the listing were cut off at the printer's 72-column limit;
the magazine set the missing tails in type, and they are restored from there.
The data dump device cut five records at 73 columns; those tails are taken from
the other witnesses and are listed in `transcription/nd_diff.txt`.

## The `nd` data files

The `nd` copies of the data files (a separate type-in that came with the
material, not published here) are a type-in of *this* magazine (they reproduce
the print's own 73-column truncations), not a machine copy of the author's
files, and they differ from the page in ways that matter:

* `AMESSAGE` records **#246–#288, #298 and #402** are empty — the typist
  stopped at the p118→p120 page break. Those are the long room descriptions
  and the instructions text; they are transcribed here from the scans.
* long messages are joined onto single lines, losing the record structure the
  dump shows (the game prints one record per line, so this changes the output).
* spelling was quietly corrected in places (`pyramd`→ the print's `pyramid`
  is *nd*'s error, but `twelve`, `little`, `fierce`, `ceiling`, `persian`,
  `belch` are nd's corrections of the print).
* `AMOVING` matches the printed table in 999 of 1000 exits; the one difference
  is room 98's east exit (print 93, nd 95).

`data_print/` is what the magazine prints. Where the print is damaged, the
completion is documented rather than silently applied.

## It runs

The program was entered into a real BASIC/3000 -- a Series 58 running MPE V/E
under SIMH, interpreter HP32101B.00.26 -- and **all 1,003 statements were
accepted without one syntax error**, which is an independent check of the
transcription that no amount of glyph matching can give. The four data files
were built there as well and verified record by record against
the transcription, the movement table's binary records included.

`port/` plays the same listing natively, on an HP 3000 BASIC interpreter grown
from the one written for Adventure ]I[. Two walkthroughs recorded on the
machine replay **identically, line for line** -- 100 and 138 lines of game
text, including the inventory's column layout, the score line's number
formatting, and a game saved to a file:

    walkthrough b: matches the HP 3000 (100 lines)
    walkthrough c: matches the HP 3000 (138 lines)

    port/adventure3000.exe              play
    port/adventure3000.exe --no-fixes   play it exactly as printed

Four corrections are applied by default, each with its reason in
`transcription/fixes.txt`: three bugs of the author's own (drinking cleared the
oil instead of the water, eating emptied your bottle, the smashed ming vase
removed the pillow rather than the vase) and the one data record that reads
`You'reein the giant room.`  The author's misspellings stay in both variants.

## Status / next

* [x] program transcribed and verified
* [x] data files transcribed and verified
* [x] entered into BASIC/3000 on the real machine: no syntax errors
* [x] playable port whose output matches the machine line for line
* [ ] sample run (p138 right column-p139) transcribed as a third witness
