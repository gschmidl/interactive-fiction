# "ADVENTURE - FORTRAN FROM MSU" - CBT overflow tape COV466, file 119

**Status: PORTED 2026-09-21, refined the same day.** `port\advent.exe` - see `port\README.md`. Verified against
the original members compiled with FORTRAN IV G and run on MVS 3.8j under Hercules: the installation
program `ADVWIZ` (the whole database reader and maintenance dialogue) and a test driver over the
routines only `ADVENT` calls both come out byte-identical. `ADVENT` itself is too big for either
FORTRAN compiler on that system - the author used VS FORTRAN G1 (`IGIFORT`), which MVS 3.8j has not.
Files in `src_original\` are verified copies (md5) of the originals named below.

Source: `CBT.COV466.FILE119.PDS`, a folder in a batch of downloaded files (IEBCOPY members as .txt, complete copy, 61 files).
Doug Moore (901 area code). IBM FORTRAN IV G + one assembler helper (`GETDTM`; AND OR XOR SHIFT CVLTUC
CVSTB are FORTRAN) + JCL (`$ADVASM $ADVFORT $ADVLINK $WIZLINK`, doc in `$ADVDOC`). `ADVWIZ` builds an
unformatted init file from `ADVTDATA`; `ADVENT2` = test build with SAVE/RESTORE. 350 points; adds a
shack (141) and outhouse (142) by the road ("AH..... DOESN'T THAT FEEL SO MUCH BETTER!") and PLOVER in
flaming letters at the volcano. Unrelated to CBT134's PL/I 382 (Tapecave).

The .txt copies are a cp037 decode of the EBCDIC members, and two characters of the game's own 64-
character set survive only as the bytes `A2` and `FF` (EBCDIC `4A`, a cent sign, and `DF`). Both are
put back where they belong in the port and in the MVS reference build; only the `POOF` message uses
one of them.
