# Mystery Mansion - HP 1000 RTE FORTRAN version

**Status: PORTED 2026-09-21 (first pass).** `port\play.bat` runs `port\mmm.exe`, the game compiled from its own
FTN4 source; see `port\README.md`. Revision 9, compiled only, RUNS since 2026-09-22 on the site's own RTE-IVB system
in SIMH: `port\rev9\play.bat`, see `port\rev9\README.md`.

Bill Wolpert's Mystery Mansion for the HP 1000 (RTE-IVB/6VM), "MYSTERY 2. REVISION 16 - 23 JUL 81", as submitted
to the INTEREX CSL/1000 contributed library ("MMM - Mystery Mansion Simulation"). The HP 3000 version, also
revision 16, runs as the original program under our MPE emulator in `..\Mystery Mansion (HP 3000, MPE)`; the two
differ in wording here and there.

Sources (verified copies, md5, in `archive_original`):

- `F:\bits\HP\HP_1000_software_collection\specials\CSL-1000_Startup-Tape.zip` - the CSL/1000 startup tape of
  27 Aug 1986, files @00500-@00508: the whole program - source `&MMM` (9920 lines: main program MMM, BLOCK DATA,
  MMRI, MMRL and the twelve segments MMSA-MMSL), its relocatable `%MMM`, the LOADR command file, the segment
  restore/delete transfer files, the softkey file and the submission form. `src_original` has them as files
  (`MANIFEST.txt`; the RTE names `"MMM /MMM \MMM *MMM` are `_MMM.info/.restore/.delete/.softkeys`).
- `F:\bits\HP\Crisis_Computer_Tapes\ccc_9trkTapes_20050826\f1_dskup_f2_rte2250sys.tap.gz` - an RTE system backup
  with the game compiled, twice (about 21.05 and 22.0 MB into the uncompressed tape), and a games menu on the
  264x softkeys (Mystery Mansion, Star Trek, Shoot, Othello, Master Mind, Lunar Lander). It is another revision:
  its banner prints the revision number at run time, credits plain "BILL WOLPERT" and the room list spells
  "BULTER'S ROOM". It is revision 9 ("MYSTERY n REVISION 9", n the mystery drawn from the clock) and runs
  (2026-09-22) on the site's own RTE-IVB system in SIMH: `port\rev9\play.bat`, see `port\rev9\README.md`.

The port builds from the startup tape image itself (`port\src\hptape.py`).
