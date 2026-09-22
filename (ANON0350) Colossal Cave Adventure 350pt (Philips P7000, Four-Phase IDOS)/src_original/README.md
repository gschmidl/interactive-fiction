# src_original - the site's IDOS disc pack

`P7000.PACK` (2,457,600 bytes, md5 `1c3f08442a9f80d8fb491528488f7882`) is the disc part of
`../archive_original/DTUX_QUEST_ADV_HGHSEC.TAP` (md5 `d12617056fd166a34bdce768c9630be9`), cut out unchanged by
`_FourPhase_work/tools/fptape.py`: the 400 tape records of 6150 bytes after the first tape mark, each without its
6-byte header. It is 3200 sectors (200 cylinders of 16) of 256 24-bit words, 3 bytes a word, high byte first - an
8231 pack as the site's DTUX utility copied it ("COPY FROM DRIVE 000 TO DECK 0 SECTORS 00000000 TO 00006177").

The pack boots IDOS S-AD33 (`$BATCH`, "SERIES IV INTERRUPT DISC OPERATING SYSTEM"), whose start-up screen carries the
site's banner "QUEST AND ADVENT" / "QUEST UNDER MFE" / "ADVENT UNDER IDOS". The game's files, from the IDOS directory
(`_FourPhase_work/tools/idosdir.py`; sectors in octal):

| file | sectors | what |
|---|---|---|
| ADVENT | 02235-02422 (118) | the program, a FORTRAN load file, loaded at 02500 ("ADVENTURE 07 JUNE 1978") |
| ADVIRT | 02073-02234 (98) | the message text, read as the game goes |
| ADNEW | 02423-02622 (128) | the game's state for a new game |
| ADSAVE | 05001-05200 (128) | the game's state as ADVENT loads it at every start; SUSPEND writes it back. Word 0 is 1 while it holds a game to play; the end of a game clears it. On the pack it equals ADNEW. |
| NEW | 02623 | job file: `// COPY` `/I=ADNEW,O=ADSAVE.` `//` then `// ADVENT` `/. INITIALIZING...` `//` |
| OLD | 02624 | job file: `// ADVENT` `/. INITIALIZING...` `//` |
| ADCOPY | 02625 | job file: copies ADVIRT, ADVENT, ADNEW, NEW, OLD and ADCOPY to another pack (`O=@1`) |

The same pack holds QUEST under MFE (see `Quest (Philips P7000, Four-Phase MFE, multiplayer)`), MFE itself, and the
site's utilities.
