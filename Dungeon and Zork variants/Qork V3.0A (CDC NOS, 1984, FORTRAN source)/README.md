# Qork V3.0A - S. O. Lidie, CDC NOS, "created 84/05/30"

**Status: PORTED (2026-09-21, plan step 14, first pass).** `port/run.bat` plays it; see `port/README.md`. Checked
against the original compiled by FTN5 on NOS 2.8.7 (Qork needs FORTRAN 77, so not NOS 1.3's FTN 4.7): seven
sessions identical, the endgame won 600/600 on both. Files here are verified copies (md5) of the originals named
below.

Source: `qork.src` + `qork.txt`, in the folder `Cyb` of a batch of downloaded files.
**Older version of a game we already run**: eXo has Lidie's own 2022.04.01 V5.0a gfortran build
(`Zork (Mainframe) (1978)\Qork\Windows\qork.exe`). DECUS-lineage Dungeon without the endgame or Bank; the 1984 text
differs from the 2022 one in ~120 lines (typos, three later messages, NOS/BE -> NOS/VE jokes). Kept because the download
is going away; it was ported last in the plan, as the optional step it was.
