# Unnamed NorthStar BASIC adventure (volcano / headhunters / lava tube)

**Status: STAGED 2026-09-19 - not ported, nothing built.** Files here are verified copies (md5) of the originals named below.

Source: `F:\bits\NorthStar\NorthStar_Horizon\101DISK.NSI` (179200 bytes, 256-byte blocks, NorthStar DOS directory in block 0-3).

The disk is a lab/data-acquisition work disk (WDIG, DIGITIZE, CATASM...). From about byte 72000 to 79000 there is a
tokenised NorthStar BASIC program that the directory does not describe (the entries covering those blocks - WDIG, WDIGB,
WDIGX, 5+1+5 blocks - are far too short), i.e. an overwritten or deleted file. Text: "YOU ARE ON THE SLOPES OF A VERY OLD,
POSSIBLY EXTINCT VOLCANO. BEHIND YOU THE CRIES OF THOUSANDS OF SCREEMING HEADHUNTERS...", lava tube, lantern-shaped object,
gold brick, "MAZE NUMBER", and "'DEAD END, DUNGEON UNDER CONSTRUCTION, PLEASE COME BACK NEXT WEEK" - so probably unfinished.
Input is single letters (`A$="U"`, "D", "B", "R", "F", "T").
`recovered\` = printable runs with offsets. Not yet done: detokenise the BASIC lines, decide how much of the program survives.
