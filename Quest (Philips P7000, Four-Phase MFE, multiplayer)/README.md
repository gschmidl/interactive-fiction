# QUEST version 1 - Philips P7000 (Four-Phase Systems IV/90 family), MFE

**Status: STAGED 2026-09-19 - not ported, nothing built.** Files here are verified copies (md5) of the originals named below.

Source: `F:\bits\Philips\P7000\DTUX_QUEST_ADV_HGHSEC.TAP` (the only file in that folder).

## What it is
A SIMH-format tape written by the site's disk-to-tape utility ("DTUX UTILITY --09 APR 80", "COPY FROM DRIVE 000 TO DECK 0
SECTORS 00000000 TO 00006177"); tape label text reads `QUEST UNDER MFE` / `ADVENT UNDER IDOS`. Danish site (job card
`/C=DAMHUS`). 24-bit words, three 8-bit characters per word, bit 7 sometimes set - mask to 7 bits to read text.
Records: 267, 12, 24 x 1536, 522, tape mark, then 400 x 6150 bytes (disc sectors), 6 tape marks in all.

QUEST strings (offsets into the concatenated record payloads, 7-bit masked):
- 1606452 `WELCOME TO QUEST VERSION 1.` / `HOW MANY PLAYERS WILL THERE BE?` / `THE LIMIT IS TWENTY PLAYERS.` /
  `YOU ARE NOT EVEN CONFIGURED FOR THAT MANY TERMINALS.`
- 1584465.. world builder: `READING GRIPES. READING PHRASE TABLE. READING PARSER TABLE. NUMBER OF OBJECTS: READING OBJECT
  PLACEMENT ARRAY. QUESTNUM := NUMBER OF QUESTS: READING THE SAD STORY. READING DICTIONARY. CHOOSING REGION CENTERS.
  PUTTING DOORS BETWEEN REGIONS. DEAD ENDS. ADDING EXTRA PATHS WITHIN REGIONS. FIXED DEAD END INTO NEW REGION`
- 1574334 direction table `DN UP NW W SW S SE E NE N`; 1636287 debug text `GENERATED SENTENCE IS: NCHARACTERS= NPLAYERS=
  ROOMNUMBER=`, `ENTER CHARACTION FOR CHARACTER`, `VALUES OF BITE AND CLAW`, `BEST WEAPON FOUND WAS`
- 1272291 `THIS IS THE 'HARD' QUEST VERSION 1 LIBRARY` followed by data that is not plain text (scrambled or word tables)
- 1675998 `PASCAL INIT ERROR!!` - the game is Pascal; directory entries near 57993: `QLHRDS QLIB QLIBSV QUEST SCREEN SIMED`
- 1645143 `A P7000 MODEL 40/45 IS REQUIRED`, MFE system initialisation messages (MFE itself is on the tape)

## What a port needs
No source. The P7000 is Philips' badge for Four-Phase hardware; nothing in F:\bits documents the CPU. Work order:
find the instruction set (bitsavers pdf/fourPhase), write the emulator, boot MFE from this dump, run QUEST with N terminals.
The same tape holds ADVENT under IDOS - see the sibling folder `Colossal Cave Adventure 350pt (Philips P7000, Four-Phase IDOS)`.

## Documentation check (2026-09-19)
- Confirmed: the Philips P7000 series is the Four-Phase Systems IV/90 as sold in Europe (Wikipedia "Four-Phase Systems",
  "Maestro I"); IDOS = "interrupt driven disk operating system", MFE/IV = Multifunction Executive.
- This very tape is discussed in the VCFed thread "Four-Phase Systems IV/90" (forum.vcfed.org, thread 1240089, pages 7-8):
  donated to Datamuseum.dk (https://datamuseum.dk/wiki/Bits:30006859), read by Poul-Henning Kamp, uploaded to bitsavers by
  Al Kossow; described there as a backup of an 8231 disk pack holding MFE/IV, IDOS, DTUX, NP-80 and IV/90 Model II support
  packages and "QUEST - multiuser Adventure game (dated 7 June 1978)". People in that thread own working IV/90 hardware.
- bitsavers.org/pdf/fourPhase/System_IV70/: `systemIV70_computerRef_Oct72.pdf` (9.9 MB, the IV/70 computer reference =
  the base instruction set), `systemIV70_dosRefMan_Apr73.pdf`, `systemIV70_periphUnitPgmg_Feb73.pdf`, two 1970 brochures.
  Also `fourPhase/Schematics`, `schematics_set1`, `schematics_set2`, `history`, `ForeWord`, and a Datapro report
  (pdf/datapro/.../Four_Phase/M11-435-10_7908_Four-Phase_System_IV.pdf). None of these PDFs has been opened yet.
- Not found: any IV/90-specific processor manual (the tape demands "A P7000 MODEL 40/45" and initialises "MAPPING RAM", so
  the IV/90 adds memory mapping and probably instructions over the 1972 IV/70), any MFE or IDOS internals manual, and any
  existing emulator or simulator (searched; only an SMD disk-controller project exists).
Verdict: feasible but the hardest item staged - base ISA documented, extensions to be inferred from the binaries and the
schematics, with real-hardware owners available to ask.
