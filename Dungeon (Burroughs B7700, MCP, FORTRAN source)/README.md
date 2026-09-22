# Dungeon V2.0 - Burroughs B7700, MCP, FORTRAN source

**Status: PORTED 2026-09-22.** `port\play.bat` runs `port\dungeon.exe`, the game compiled from its own B7700 FORTRAN
source with its own text; see `port\README.md`.

The DECUS FORTRAN Dungeon (Supnik's translation of MIT's MDL Zork) as converted for a Burroughs B7700. The code is
V1.2c ("3rd set of bug fixes by Chris Wilson"). The text is V2.0 ("Initial version for I'ntl by Tom Fota"), "This
version created 01-DEC-80". The source's own comments mark its "B7700 CHANGES". File titles are under usercode 00661
on the pack SYMBOL30. The game's newspaper, dated 23-APR-80, calls it "a first, trial version of Dungeon on the B7700".

Sources (verified copies, md5):
- `archive_original\CSL-1000_Rev-2213.zip` - the INTEREX CSL/1000 library release 2213 (TF tape image; also in
  `..\_HP1000_work\CSL-1000\`, from `F:\bits\HP\HP_1000_software_collection\specials\`). Contribution A072, "Dungeons
  And Dragons Game" by an unknown contributor: "This is the source for Dungeons and Dragons that so many have
  wanted and waited for. This copy has not been edited to work on the HP-1000."
- `src_original\` - its three files as text: the submission form (a07201), the source `&DUNGN.ftn` (a07202, md5
  78a30d010eafbc1cc5dc0e0d221af243) and the text file `DUNTXT.txt` (a07203, md5 1fd686e6babdb8dca19d1886293150e5).
  These are the same files as `..\Dungeon (HP 1000, RTE, FORTRAN source)\src_original\CSL_2213_Burroughs`.

Tom Hutchinson made it work on the HP 1000 as V3.0a (18 Nov 1982): `..\Dungeon (HP 1000, RTE, FORTRAN source)`.
