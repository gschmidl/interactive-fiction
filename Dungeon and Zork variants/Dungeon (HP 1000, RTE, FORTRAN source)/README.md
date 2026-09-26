# Dungeon (Zork) - HP 1000 RTE FORTRAN, HP Contributed Software Library

**Status: PORTED 2026-09-21 (first pass).** `port\run.bat` runs `port\dungeon.exe`, Dungeon V3.0a compiled from its
own FTN4X source with its own data base; see `port\README.md`.

Dungeon V3.0a, "Initial version for HP 1000 by Tom Hutchinson" (Dome Petroleum, Calgary, 18 Nov 1982): the DECUS
FORTRAN Dungeon (Supnik's translation of MIT's MDL Zork) by way of the Burroughs version, V2.0 "for I'ntl by Tom
Fota". The main game only (500 points); the endgame is a sign, "Soon to be constructed on this site".

Sources (verified copies, md5, in `archive_original`; all eleven CSL-1000 zips are in `..\..\_work\_HP1000_work\CSL-1000\`):

- `bitsavers.org/bits/HP/HP_1000_software_collection/specials/CSL-1000_Rev-2240.zip` - TF tape, release 2240 (22 Sep 1986 copy):
  contribution **F042 "DUNGN - DUNGEONS AND DRAGONS"** by Tom Hutchinson - the main program and five segments
  (#DUNGA-#DUNGF), the subroutine library #DUNGL, the segment linker #DLINK and #A2A1/#A1A2 in assembler, the LOADR
  command file and the messages and initialisation file @DUNGN; plus contribution F017 "SUBS" (the same group's
  library), whose #UMOVE the loader file names. **This is what the port is built from.**
- `...\CSL-1000_Rev-2213.zip` - TF tape, release 2213: contribution A072, the Burroughs FORTRAN source (V1.2c code, V2.0
  text) "not edited to work on the HP-1000" - the version Hutchinson started from. In `src_original\CSL_2213_Burroughs`;
  ported on its own (2026-09-22) as `..\Dungeon (Burroughs B7700, MCP, FORTRAN source)`.
- `...\CSL-1000_Rev-2830.zip` - FMGR tape, release 2830 (July 1988): M060 "CDS DUNGEON & DRAGONS GAME", a later
  version for RTE-A; not read, and it will not be (user, 2026-09-22).

`src_original` has the 2240 and 2213 files as text (`MANIFEST.txt`); the port reads the tape image itself
(`port\src\hptape.py`).
