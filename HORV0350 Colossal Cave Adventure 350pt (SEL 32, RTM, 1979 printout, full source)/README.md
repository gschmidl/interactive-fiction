# Adventure 350 for the SEL 32 / RTM - Horvath & Norwood, printout of 1979-03-21 (HORV0350)

**Status: PORTED 2026-09-21 (first pass).** `port\advent.exe` - see `port\README.md`. Complete copy of
the named directory of https://github.com/Quuxplusone/Advent at commit
d38e82550600144e3547d6472bc80dbf49ca214b (2026-09-15), md5-verified against the clone.

`src_original\HORV0350\`: `transcribed-code.txt` (130 KB FORTRAN), `transcribed-data.txt` (75 KB database), `README.md`.
Arthur O'Dwyer's transcription (Dec 2025) of fan-fold paper from Mike Willegal: Woods' 350 as ported by Gary Palter (MIT,
the "portable" PALT0350 with a conversion guide) -> Dick Reynolds "HCSD version 01.112777" -> Ned Horvath, SEL32/RTM,
1978-08-01 -> C. Norwood, warning clean-up 1978-10-11. Content is vanilla 350; section 4 adds G and T for GET/TAKE, SLAY,
Q, and drops "?" for HELP. Sibling of FUNADV's ancestor (PALT0350 -> WHIT0370).

The port found **ten errors in the transcription** - nine of them stop the game working, including a
missing comma that takes an array out of a COMMON block and a `1` for an `I` that breaks every
two-word command - and one thing the paper cannot have held: `CVLTUC`'s lower-case alphabet, which
the listing shows as 26 blanks. All are listed in `port\README.md`, which also compares this database
against the MSU port's, message by message: 32 differences, every one explained, two of them places
where *this* database has more than MSU's because the SEL read 128-column records.
`TRANSCRIPTION_ERRORS.md` writes the findings up for Arthur O'Dwyer's bounty, with line numbers: nine
claimed (the tenth, `DDIGGING`, is also in WOOD0350, so it became one of four questions for the printout).
