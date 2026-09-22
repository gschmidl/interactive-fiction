# Mystery Mansion revision 9 - on the site's own RTE-IVB system

**Status: RUNS 2026-09-22.** `build.bat` once, then `play.bat`.

The other revision of the game, "REVISION 9", compiled, as it was on the
machine of the site whose backup is
`..\..\archive_original\f1_dskup_f2_rte2250sys.tap.gz` (tape file 3, labelled
"RTE IV B  7906 DISC SYS. REV E; LEN ROSE 14 APR 83 REMOVEABLE PLATER"). There
is no source for it, so this is not a translation: the site's whole RTE-IVB
system boots in SIMH's HP 1000 (`hp2100`) off its own disc, and FMGR runs the
site's own procedure `GOMMM` (restore the segments MMSA-MMSI, `RU,MMM,1`).

    WELCOME TO MYSTERY MANSION.  MYSTERY 697    REVISION 9
    COMPLIMENTS OF BILL WOLPERT

"MYSTERY n" is the number of the mystery, which the game draws from the
clock (revision 16 prints a fixed "MYSTERY 2. REVISION 16" instead).

## Build and play

- The tape is kept on disk only, not in the repository (a whole site's disc
  save). It came from bitsavers.org `bits/HP/Crisis_Computer_Tapes/
  ccc_9trkTapes_20050826/`; `mkdisc.py` says so if it is missing.
- `build.bat` (or `build.sh`): `src\mkdisc.py` makes `.build\disc0.img`, the
  7906 image (the tape's 411 tracks are the removable platter's first 411
  tracks; SIMH keeps that platter first, words little-endian); copies
  `src\rte.sim` and SIMH's `hp2100.exe` (V3.12) into `.build` - the one the
  environment variable `HP2100` names, else the one on `PATH`. (eXo's
  385-point Adventure has one, in its `HP-2100` folder.)
- `play.bat` (`src\term.py`) runs SIMH hidden, in a console of its own (SIMH
  will not run on a pipe), tied to the terminal by a job object so that
  closing the window stops it too. `term.py` is the HP 2645 on SIMH's BACI
  port. It answers the 2645 driver's status requests (ESC ^ -> ESC \ and
  seven status bytes at the next DC1) and types a line only when the
  driver's DC1 asks for one. It drops the driver's echo, folds the keyboard
  to capitals, and turns the display escapes (cursor address, clear,
  inverse/half-bright fields) into ANSI.
- The session: the boot asks SET TIME, and `term.py` answers as the site's
  operator did, with this computer's date and time (`SYTM,year,day,h,m,s`
  through FMGR). It then types `TR,GOMMM` at FMGR's prompt. When the game
  has ended and FMGR prompts again, it stops the machine; it leaves out
  GOMMM's softkey games menu, which would be drawn over your screen. End of
  input or Ctrl+C also stops it. Only one session at a time can run (one
  machine, one disc).
- SUSPEND and RESTORE keep games on the machine's disc, `.build\disc.img`.
  The first session copies it from `disc0.img` and it stays; delete it to go
  back to the tape's disc. The cartridge to give is **2**, the system
  cartridge with the game and the site's files. Cartridges 10-14 are mounted
  but empty, because those tracks are not on the tape. The saved game is the
  file `MMxxxx`.
- Options: `--fixed-clock` leaves the clock at its boot value (1978, day 217,
  08:00) - always mystery 2; `--show-boot` shows the boot (SET TIME, WELCOM
  programming the softkeys, the site's banner "RTE IV-B DISC SYSTEM REV. E
  LAST BACKUP SEP 18,1980") and leaves FMGR to you after the game; `-h`,
  `--help`. Unknown options exit 2.

Nothing in revision 9 limits who plays or when: no hours, no codes. The
creator question of Wolpert's DISPLAY debug command ("nn ARE YOU THE
CREATOR?") guards only that command. Revision 16's answer ("YES xx",
xx = 99-nn) is refused here.

## What it took

- The generation maps `.ENTR`, `..MAP`, `.XFER`, `.GOTO` and the other FORTRAN
  library entries onto Fast FORTRAN Processor instructions (1052xx; the
  answer file on the disc lists them). SIMH's 1000-E has no FFP by default.
  Without `set cpu FFP`, FMGR hangs at the boot and WHZAT dies with an MP
  error.
- The boot's first words ("SET TIME") come before a Telnet client could
  connect, so `rte.sim` idles in a loop for a moment before `boot ds`.
- Select codes as generated: TBG 10, disc 11 (the boot loader takes it from
  switch register bits 11-6: `S=001100`), 2645 on the BACI at 12, the
  teleprinter at 20; 128K words (the generation asked for 96K).

## Tests

- `tests\play9.py`: unknown option exits 2. Then three sessions on a scratch
  disc (the player's disc is kept aside):
  - A (fixed clock): mystery 2, the gate, the note, SUSPEND onto cartridge 2
    as TEST, the score, QUIT.
  - B (fixed clock): RESTORE TEST, the same score, "THE NOTE SAYS THE SAME AS
    IT DID BEFORE" (it was read before the SUSPEND), QUIT.
  - C (this computer's clock): the banner, QUIT.

  Each session is checked for exit 0 and no simulator left running.
- By hand:
  - in a pseudo console: typed lower case, one echo, QUIT ends the session;
  - a second session is refused while one runs (exit 1);
  - killing the terminal's process stops the simulator.

## Still to do (refine pass)

- The answer to DISPLAY's creator question in this revision (it is not
  revision 16's; finding it means reading the check in segment MMSF).
- Left as on the site: "YOU ARE TAKING TOO LONG" is the game's own real-time
  turn clock.
