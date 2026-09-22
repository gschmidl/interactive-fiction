# Unnamed NorthStar BASIC adventure (volcano / headhunters / lava tube)

**Status: PORTED 2026-09-21 (first pass).** `port\volcano.exe` (`port\play.bat`) runs the game in the North Star
BASIC it was written for, taken from the same disk, on an emulated Z80. See `port\README.md`. Files here are
verified copies (md5) of the originals named below.

Source: `F:\bits\NorthStar\NorthStar_Horizon\101DISK.NSI` (179200 bytes, md5 e52c2c204cf77c096ea293ef9aa4b913).

**It is not an orphaned fragment**, as the folder's first name said (renamed 2026-09-21 from "..., orphaned
fragment)").
- The disk is NorthStar DOS 5.1, double density: 350 blocks of 512 bytes, not 256.
- The program is the ordinary BASIC file **DBACK** (directory: block 140, 20 blocks, type 2).
- All of it is there: 179 lines in ascending order, 1 to 32530, 5890 bytes, then the end-of-program byte.
- The rest of the disk is a lab/data-acquisition work disk (WDIG, DIGITIZE, CATASM...).

`recovered\detok.py` lists it as `recovered\DBACK.bas`. The token values come from the keyword table of HYBASIC on the
same disk: each keyword follows its token byte (80 LET, 82 PRINT, 84 IF, ... F1 <>, F5 =, CE RND). 9A is a line
number constant. Every byte of DBACK decodes, and the port shows that the listing is exact: HYBASIC, given the
listing, tokenises it back into DBACK byte for byte.

The game: escape the headhunters down the volcano into a lava tube, find a lantern (L) and a gold brick (G), then
cross a random maze to a trap-door room with orcs, or fall into a pit.
- Commands are single letters (U, D, B, R, F, T) and INVENTORY.
- The input routine at 32000 also takes the author's own warps MAZE1, TRAPDOOR, PIT and SHAFT.
- It is unfinished by design. The pit ends with "DEAD END, DUNGEON UNDER CONSTRUCTION, PLEASE COME BACK NEXT WEEK",
  and every other way ends in death or the orcs' prison.
- One of the maze's ways out (10110) is chosen with `RND(-1)`. In HYBASIC that times the disk: it counts reads of
  the controller's sector number until the sector changes.

`doc\Users Guide to North Star BASIC First Edition (Rogers 1978).pdf` is Robert R. Rogers' third-party guide
(Interactive Computers, Houston, 1978) to North Star BASIC Version 6 Release 3. It was downloaded 2026-09-21 from
hartetechnologies.com/manuals/Northstar/, md5 d414b55aefc8a40f4ff58b89b16ed426.

`recovered\strings_0x11000-0x15000.txt` holds printable runs with their offsets (the first survey).
