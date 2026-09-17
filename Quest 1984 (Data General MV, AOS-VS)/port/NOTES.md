# Notes — the 1984 Quest

`src32/` is a copy of the emulator from the NADGUG Quest port
(`../../Quest (Data General MV, AOS-VS)/port`), whose `NOTES.md` covers the
machine, the two processes, the shared files, IPC and the D200.  Everything
that port learned applied here unchanged; this file is only what the older
build needed on top.  Addresses are hex and routine names are from the
programs' own `.ST` files (`notes/*.sym`).

## The programs

| | blocks | impure | shared | `I.INIT` | user main |
|---|---|---|---|---|---|
| `QUEST_SERVER.PR` | 936 | 3 | 924 → 99000..17FFFF | 17EED5 | 17A3D0 |
| `QUEST.PR` | 1038 | 4 | 1025 → 7FC00..17FFFF | 17EE1D | 167576 |

The start-up vector is found the way the later port finds it.  The server
maps `WORLD_DATA_FILE` at 099000 and `CASTLE_DATA_FILE` at 161000, the player
at 07FC00 and 147C00 — each over its own F77 COMMON, as in the later build —
and both put `SHARED_DATA_FILE` at 180000 in the partition they grow with
`?SSHPT`.  The server calls itself `Q_SERVE` in `QUP.CLI` (the later one
`quest.server`); nothing looks the name up, so that makes no difference.

## Three instructions

**LNDO / LWDO (type 17).**  The long-displacement DO loop: the opcode, an
indirect bit and a 31-bit displacement in two words, then a termination offset
— four words (32-bit *Principles of Operation* 10-79).  The opcode table
derived from MASM gave type 17 a length of 3, which only mattered once the
instruction was met: `QUEST.PR` has `8698 7000024E 0038` at 17CE1A, in
`TERRITORY_MAP`.  AC in bits 1-2, index in bits 3-4, offset counted from the
displacement word, like XNDO.

**The DO loop stores on the way out.**  The manual says the stepped value
goes to memory and to the AC "in either case"; the XNDO code only did it
while the loop continued, which leaves a FORTRAN DO variable one short after
the loop.  Now both do.  The NADGUG port's recorded sessions are identical
before and after.

**103750 — the old runtime's FRDS.**  After LNDO the server stopped on 87E8,
which EBID.SR lists as FLOGD, one of the Eclipse's one-word "floating point
functions".  It occurs in exactly one routine in either program:
`SQR31?3`, the FORTRAN runtime's single-precision square root, three times,
each straight after an `FHLV`.  The NADGUG server's `SQR31?3` is word for
word the same routine except that those three words are `84D8`,
`FRDS 0,0` — round FPAC0 to single precision, which is what a
single-precision square root needs at that point, and an instruction the 1980
MV/8000 *Principles of Operation* does not list yet.  So 103750 is executed
as `FRDS 0,0`.  It is an inference from that comparison, not from a manual.

## The title

`QUEST.CLI` did `WRITE [!ascii 223]`, `TYPE CASTLE`, `WRITE [!ascii 222]`
before `X QUEST` unless `/S` was given: roll disable, the file, roll enable.
`CASTLE` (identical on both tapes) is a D200 animation — cursor addressing
only — that sweeps the screen with diagonal strokes and draws a castle, a
knight and "Welcome to QUEST".  `aosvs32 -c CASTLE` types it through the
terminal at about 19200 baud, because at full speed none of it is seen.

The roll codes matter.  The very first stroke writes the bottom-right corner
(column 79, row 23), and later strokes do again; with roll enabled each of
those wraps off the bottom and scrolls everything up a line, which is how the
first version of `-c` took the picture apart.  With roll disabled the D200
wraps to the top, and `-c` now sends the two codes (and the NEW LINE each
CLI `WRITE` ends with) around the file as `QUEST.CLI` did.

## Not ported: QSET

The second tape file holds `QSET` (FORTRAN source, `.OB`, `.PR`, `.ST`), a
tool that shows one user's record.  Its UST extension has no `LJMP I.INIT`
for the emulator to find (words 0x180.. hold a table of ring-7 addresses
instead), so it starts at the user main without the runtime's set-up, runs
with no stack and loops before its first output.  Its source is still the best
description of `USER_DATA_FILE`: 266-byte records of `VDATA(1)` (the name's
length), the name (32 bytes), `VDATA(2)` (the password's length), the
password (32), `VDATA(3..100)` — position in 3-4, intelligence 85,
experience 86, strength 87, vision 89, wealth 91, quest 100.  `DAVECOM` & co.
patch words 0166 and 0172 of their slot (133 words each): strength and wealth.

## Verification

`sh tests/run.sh`: the title screen, a new player walking and reading the
command help and leaving with ESC, and the same player logging in again in
the same save directory — back at the square he left, with the first-quest
reminder.  The NADGUG port's four checks also pass on this build of the
emulator.  Since the multiplayer work: two scripted players in one world,
and `tests/netplay.py` and `tests/consoleplay.py`, which play the network
and console game for real (the later port's README describes them).

## Multiplayer (2026-09-17)

`src32/` is again the same emulator as the NADGUG port's, which now runs the
game the way it was played: `--port <n>` starts QUEST_SERVER, listens, and
gives every player who connects — or who starts `quest` in another window —
a D200 and a QUEST process of their own.  The later port's `NOTES.md` has the
whole of it: terminals, waiting on keys and the wall clock, leaving and
hanging up, Ctrl-C, the shared files at a switch, and the twenty-fold speed-up
from remembering `mvfind`'s answers.

What is particular to this build:

* **The same logon, and the same race.**  The client sends requests 9, 1
  and 2 with the same meanings, and `IPC_TASK` (17BAFE) answers 9 with the
  same count-then-reuse at 17BB3A — a player's number is the count in the
  shared data plus one while that is at most ten, then the first slot whose
  in-use bit is clear, without setting it.  Only the slot size differs: 434
  words (`NLDAI 0,434` at 17BB5B) against the later 686.  The emulator's
  one-logon-at-a-time gate therefore applies unchanged.
* **The title for every player.**  QUEST.CLI typed CASTLE at the terminal
  that ran it, so the host types it on each player's terminal as they
  connect, paced at 19200 baud and without holding anyone else up (`host.h`,
  `host_intro_step`); any key skips it, and `--no-title` at the window that
  starts the world leaves it out for everyone.
* **Eight characters.**  `USER_DATA_FILE` is eight 266-byte slots; the
  authors' five live characters and dead BERT's leave three for new ones.
  Scripted players found them in slot 2 (BERT's) and then 7.
* **No class prompt**, so a new player goes from "Hit any character to
  continue" straight to the map; the shared test scripts allow for both
  games.  The port is 4084, the later game's 4040, so both worlds can run on
  one computer at once.

## God mode (2026-09-17)

`--god` works here as in the later port, whose `NOTES.md` has the method and
a table of both builds.  This build's own addresses: `SD_PTR` 1F4 and
`PLAYER_NUM` 1FA; 434-word slots with the in-use bit in slot - 339 (IPC_TASK's
`WNADI 1,60112` at 17BB61 is 16 x -339); intelligence, experience, strength,
maximum strength, vision, perception at slot - 226 .. - 221 and wealth at
slot - 219; DIED's WSAVS at 16DD4A and its WRTN at 16E000.  Its
DISPLAY_INVENTORY keeps no copy of what it showed, so the slot was dumped
mid-game and each value confirmed by setting it.  No classes here, so a god's
intelligence stays at 10000.

`tests/run.sh god`: with the clock frozen at 1000003600 a new player lands in
Southwest Dragonia, and walking south is done away with.  With `--god`
GERHARD walks on at strength 1024; with `--god` and strength set to 1 every
time DEFEND begins, he walks on too — DIED skipped 11 times.
