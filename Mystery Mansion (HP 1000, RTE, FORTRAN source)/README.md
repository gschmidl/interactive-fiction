# Mystery Mansion - HP 1000 RTE FORTRAN version

**Status: STAGED 2026-09-19 - not ported, nothing built.** Files here are verified copies (md5) of the originals named below.

Sources: `F:\bits\HP\HP_1000_software_collection\specials\CSL-1000_Startup-Tape.zip` (FORTRAN source) and
`F:\bits\HP\Crisis_Computer_Tapes\ccc_9trkTapes_20050826\f1_dskup_f2_rte2250sys.tap.gz` (a second copy, ~21.47 MB in).

Startup tape ~3311898-3390378: `PROGRAM MMSC(5)`, `MMSD`, `MMSE`, `MMSF` ... (segments, "C ******* SEGMENT C *******"),
`COMMON/MMBC/IWRD(9,9),IPR(5),IVRB(4,89)`, FORMATs with the Mystery Mansion text ("YOU ARE IN A MAZE OF TWISTY LITTLE
PASSAGES. THE PASSAGES ARE LOW AND YOU HAVE TO STOOP", "HIDEOUS HIGHWAY", papyrus scroll, library arch).
We run the HP 3000 original under our MPE emulator; this is the FORTRAN source of the RTE conversion, so a source port
is possible and would also document the HP 3000 game's logic. Segments MMSA/MMSB and data files not yet located.
