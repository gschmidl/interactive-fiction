# Artifacts

This directory contains artifacts digitized from line printer output of a different version of Adventure. It
preceded Dave Platt's 550-point version and includes some elements of his version. It was probably an early
version of his modification of Don Woods' version, adapted for Multics. Gary Palter's name is also on it, which
shows that it is related to Gary Palter's port of Adventure to Multics, and I apparently made some modifications
as well. It is a 462-point version of the game that ran on Multics. — Jim Lippard

The files here are the digitized artifacts, kept exactly as printed, errors included. Corrections belong in the
working copies elsewhere in this repository. The notes files list every known error.

## The listings

The information below comes from each listing's Multics printer header.

| Transcription | Original file | Printed by | Printed | Pages | Lines |
|---|---|---|---|---|---|
| `Multics/adventure.data` | `>udd>MED>Kaiser>Lippard>adventure.data` | Lippard.Scouting.a | 05/12/80 0250.1 mst Mon | 45 | 2,699 |
| `Multics/adventure_.fortran` | `>udd>MED>Kaiser>Lippard>adventure_.fortran` | Lippard.Scouting.a | 05/12/80 0249.5 mst Mon | 64 | 3,110 |
| `gcos/adv.fortran` | `>udd>Multics>Lippard>games>adv.fortran` | Lippard.Multics.a | 07/15/83 1655.5 mst Fri | 50 | 5,884 |

- **`adventure.data`** is the game database, used by `adventure_.fortran` on Multics.
- **`adventure_.fortran`** is the Gary Palter engine derived from Don Woods' version, with minor modifications
  by Jim Lippard. Some of these are handwritten changes on the line printer output.
- **`gcos/adv.fortran`** is a distinct GCOS port of Don Woods' 350-point version of Adventure. Its line printer
  listing was found alongside the other two.

The header line counts don't all match the transcriptions:

- `adventure.data` has 2,699 lines, as the header says.
- `adventure_.fortran` has 3,756 lines, of which 3,103 are not blank; the header says 3,110.
- `adv.fortran` has 2,942 lines, exactly half the header's 5,884.

These differences probably come from how the printer counted lines, since the transcriptions follow the printed
pages.

## Photographs

Each printout stack is photographed front and back, and the photos sit next to the transcription:

- `adventure.data`: [front](Multics/adventure.data-front.jpeg), [back](Multics/adventure.data-back.jpeg)
- `adventure_.fortran`: [front](Multics/adventure_.fortran-front.jpeg), [back](Multics/adventure_.fortran-back.jpeg)
- `gcos/adv.fortran`: [front](gcos/adv.fortran-front.jpeg), [back](gcos/adv.fortran-back.jpeg)

The page-by-page photographs used for the transcription are not included (about 90 photos, over 100 MB).

## Multics/: Adventure 1.2 (462 points), Multics, May 1980

The game this repository is about: Don Woods' 350-point Adventure, taken from Gary Palter's MIT-Multics Fortran
port and extended with early Dave Platt material.

| File | What it is |
|---|---|
| `adventure_.fortran` | Transcription of the Fortran source listing (40 program units). Columns follow the paper: `c` in column 1, statements in column 7, `&` continuations in column 6. |
| `adventure_.fortran-NOTES.md` | Page-by-page notes, comparison with Woods and Palter (via the SEL-32 transcription HORV0350), a completeness check, and the list of bugs to fix. |
| `adventure_.fortran-handwritten-notes.md` | Jim Lippard's handwritten ink annotations on the listing. The main one is an unfinished implementation of TAKE ALL / DROP ALL. |
| `adventure.data` | Transcription of the game database (2,699 lines), laid out as printed. The first field is right-justified in 8 columns and text starts in column 9, matching the program's `i8`/`70a1` reads. |
| `adventure.data-NOTES.md` | Page-by-page notes, comparison with Woods 350 and Platt's surviving 1984 A-code source ("Platt 1984"; the game itself dates from 1979), and the ERRATA list. |
| `transcription-source/` | `adventure.src`, the same database in Woods-style tab-separated form, which is easier to diff. `render.py` regenerates `adventure.data` from it byte for byte. |

## gcos/: GCOS Adventure (related artifact, printed 1983)

A separate port of Woods 350 to Honeywell GCOS, labeled "GCOS version" in Jim's hand on the cover sheet.
It is **not** an ancestor of the Multics game. The two ports solve the same problems independently:

- **Word storage:** 4-character words versus 5-character packed words.
- **Data file format:** different in each.
- **Save:** a core-image dump versus a common-block save.
- **Comment wording:** different variants of Woods' comments.

It is included because it was found with the Multics listings, and it documents the other Honeywell Adventure.
This listing was printed in 1983 from Jim's `games` directory on the Multics project.

| File | What it is |
|---|---|
| `adv.fortran` | Transcription of the GCOS source listing (2,942 lines = 49 sheets of 60 lines plus 2). The listing is lowercase, though the running GCOS version was uppercase. Tab stops are every 7 columns. Some statement labels are marked with `#`. |
| `adv.fortran-NOTES.md` | Page-by-page notes, differences from Woods (including a wizard-mode backdoor), and a review list. |

The GCOS game's own data file (`/adv.data`, `i4` layout) was not among the printouts.

## Transcription conventions

- Text is transcribed character for character, including original typos, misspellings and odd spacing.
  Anything uncertain is flagged in the notes.
- Column positions were measured from the photographs where it mattered: labels, continuation text and
  tab-expanded comments.
- Nothing in these files has been corrected. See the ERRATA and fix lists in the notes.
