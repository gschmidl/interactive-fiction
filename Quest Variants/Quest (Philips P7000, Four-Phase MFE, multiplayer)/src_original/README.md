# src_original - the site's disc pack

`P7000.PACK` (2,457,600 bytes, md5 `1c3f08442a9f80d8fb491528488f7882`) is the disc part of
`../archive_original/DTUX_QUEST_ADV_HGHSEC.TAP` (md5 `d12617056fd166a34bdce768c9630be9`), cut out unchanged by
`_FourPhase_work/tools/fptape.py`; it is the same file as the ADVENT port's `src_original/P7000.PACK`. 3200 sectors
(200 cylinders of 16) of 256 24-bit words, 3 bytes a word, high byte first.

The pack boots IDOS S-AD33; the site's banner reads "QUEST AND ADVENT" / "QUEST UNDER MFE" / "ADVENT UNDER IDOS".
QUEST's files and MFE, from the IDOS directory (`_FourPhase_work/tools/idosdir.py`; sectors in octal):

| file | sectors | what |
|---|---|---|
| QUEST | 03616-04051 (156) | the game, a load file (loaded at 04000), Pascal ("PASCAL INIT ERROR!!") |
| QLIB | 03106-03351 (164) | the library QUEST reads at the start, chained: "THIS IS THE 'HARD' QUEST VERSION 1 LIBRARY" |
| QLHRDS | 02642-03105 (164) | the same hard library (QHELP calls it QLHARD) |
| QLIBSV | 03352-03615 (164) | the 'EASY' library QUEST came with, saved when the site put the hard one in QLIB |
| QHELP | 02626-02641 (12) | QUEST's manual, chained text; `QHELP.txt` here is its text |
| MFE | 04052-04114 (35) | MFE/7000 BN03-C, the Multifunction Executive, with MFESYS, MFELIB, MFEFIG, MFEBDC, MFEDMP |
| SCREEN, SIMED | 01711-01746 | other load files on the pack; QUEST does not use them |

At a terminal MFE shows the key prompt line "TYPE Q, S OR ?": Q signs on to QUEST (a job's key character is the
first letter of its name), S selects MFE's status displays, ? lists the active jobs (MFE Operator's Quick Reference).

The library data after its first record is not plain text. The libraries are chained files: in each sector word 0
holds the byte count (and, in its low 12 bits, bits of the sector's own number) and word 1 the previous and next
sectors, so two copies at different places differ in those words only.

`QHELP.txt` was made with `_FourPhase_work/tools/idoscat.py P7000.PACK QHELP`: the chained file's bytes, lines ending
in LF, ESC n standing for n blanks.

The same pack holds ADVENT under IDOS (see `(ANON0350) Colossal Cave Adventure 350pt (Philips P7000, Four-Phase
IDOS)`) and the site's utilities.
