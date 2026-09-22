# Adventure "thru a Cave" - TI 990 / DX10 GAMES library

**Status: PORTED (2026-09-21, plan step 24).**
- **What the port is:** the original program run on an emulated TI 990/10 with the DX10 supervisor calls it makes. See
  `port\README.md`; play with `port\play.bat`.
- **Verified against the real system:**
  - The program images equal DX10's own SPI listings.
  - Three sessions recorded on sim990 3.3.0 + DX10 3.7 replay identically, dwarves and knives included.

Folders:
- `archive_original\`: verified copies (md5) of the originals named below.
- `src_original\`: what was cut out of the tape by DX10 itself, with notes on how:
  - the program's two segments;
  - its data file CAVE;
  - the SCI procedures;
  - the program-file map.
- `port\`: the port.

Source: `bitsavers.org/bits/TI/990/9trkTapes/orig/ti990_games.tap.gz` (+ `titapes.txt`; `DX10_system_tapes\` = `.../orig/DX10/`,
the OS tapes an emulator would boot).

The tape is a DX10 backup of `GAMES` (`.SYN GAMES=GAMES.GAMES`, `.USE @GAMES.PROC`).
- **Games on it:** NIM, PACMAN, FOOTBALL, HANGMAN, MIND (COBOL), STARTREK, CALENDAR, TTT, ADVENTUR, LANDER, BIO, GUESS,
  LUNAR, MINE (Tim O'Connor, 990 FORTRAN, 4/79), BBOX, SUB, BLACKJAC, TREK.
- **Adventure's menu text:** "ADVENTURE Wandering adventure thru a Cave". The proc `ADVENTUR(Adventure thru a Cave)` asks
  "Will/did you save your game?" and takes `SAVE/RESTORE PATHNAME=ACNM(@$CAVE)`.
- **What Adventure is on the tape:** the linked task ADVEN in `GAMES.PROG`, plus the data file `FILES.CAVE`, whose texts
  are scrambled. There is no FORTRAN source for it; other games on the tape do have source.

The reference machine is Dave Pitts' sim990 3.3.0 for Windows (`sim990win-3.3.0`)
with its DX10 3.7 disk; the working copy and the tools are in `..\_TI990_work` (`run370\`, `tools\`, `docs\`).
- The console is on telnet port 2099. Log on with Esc `!`, then run `IDT` with a 4-digit year.
- `RD MT01 .GAMES` restores the tape.
- `AS GAMES=.GAMES.GAMES` and `.USE .S$PROC,.GAMES.GAMES.PROC` set up the procedures; then `ADVENTUR` runs.
