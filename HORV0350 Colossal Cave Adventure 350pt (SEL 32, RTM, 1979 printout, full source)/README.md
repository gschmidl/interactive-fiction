# Adventure 350 for the SEL 32 / RTM - Horvath & Norwood, printout of 1979-03-21 (HORV0350)

**Status: STAGED 2026-09-19 - not ported, nothing built.** Complete copy of the named directory of https://github.com/Quuxplusone/Advent at commit d38e82550600144e3547d6472bc80dbf49ca214b (2026-09-15), md5-verified against the clone.

`src_original\HORV0350\`: `transcribed-code.txt` (130 KB FORTRAN), `transcribed-data.txt` (75 KB database), `README.md`.
Arthur O'Dwyer's transcription (Dec 2025) of fan-fold paper from Mike Willegal: Woods' 350 as ported by Gary Palter (MIT,
the "portable" PALT0350 with a conversion guide) -> Dick Reynolds "HCSD version 01.112777" -> Ned Horvath, SEL32/RTM,
1978-08-01 -> C. Norwood, warning clean-up 1978-10-11. Content is vanilla 350; section 4 adds G and T for GET/TAKE, SLAY,
Q, and drops "?" for HELP. Sibling of FUNADV's ancestor (PALT0350 -> WHIT0370).

Before use: strip the curatorial markings and the printer's running header
(`grep -v '^==p' | grep -v '^   21MAR79'`), split into files, remove job control (`$ASSIGN1` ...). The code was printed
truncated at column 72 and the data at column 80; text beyond that is O'Dwyer's conjecture copied from WOOD0350
(see his README, which also explains the $5 error bounty).
Port = ordinary FORTRAN source port (SEL 32: 32-bit words, 4 characters per word).
