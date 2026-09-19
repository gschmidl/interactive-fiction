# SVHA Adventure port — notes

How SVHA-ADVENTURE runs on the ND-100/SINTRAN emulator built for Skattejakt,
what it needed beyond that game, and how the added SAVE works.  Octal
throughout.  The machine itself (CPU, floating point, the terminal) is
described in the Skattejakt port's `NOTES.md`; the source is shared, since
18 September 2026 with all the ND-100 ports (`_ND100_work\emu2\src`, copied
into each).  For SVHA that changed Esc, which now leaves SINTRAN's `@` with
`CONTINUE` and `LOGOUT` as the other ports do, and the debug commands (below).
A saved game keeps the names of the game's open files, as the program gave
them (`(GAMES)SVHA-STLOTEXT`) before and as found (`SVHA-STLOTEXT:SYMB`) now;
either build reopens either, and a game saved by one plays on in the other
byte for byte.

## Where it came from

The eXo SVHA Adventure ND-100 pack `SMD0.IMG`, directory `PACK-ONE`
(SINTRAN III VSX/500 L), user GAMES — the same six files are also on the
eXo `floppy.img` (directory `BACKUP-GROUP-6`), byte-identical:

| file | bytes | created | sha1 |
| --- | --- | --- | --- |
| SVHA-ADVENTURE:PROG | 87040 | 1984-08-14 08:55 | 5f31357e9e2e |
| SVHA-DATAFIL:SYMB | 35932 | 1984-08-14 08:56 | 092f387b8f47 |
| SVHA-STLOTEXT:SYMB | 35496 | 1984-08-14 08:56 | 1e4e10f397a6 |
| SVHA-KOLOTEXT:SYMB | 31824 | 1984-08-14 08:55 | a9b569ae3614 |
| SVHA-OBJETEXT:SYMB | 40536 | 1984-08-14 08:55 | 8a42c3b89c88 |
| SVHA-TILFTEXT:SYMB | 43272 | 1984-09-11 14:04 | c89b5dba7e9f |

Free pages of the pack still hold older copies of a GAMES directory page and
of program and text pages; they point at the same blocks and hold the same
bytes, so there is no second version.  `GAMES/INFO-SPILL:TEXT`, the games
pack's guide (in Norwegian, with an English SVHA user's guide), is kept in
`..\src_original\`.

## The program

:PROG header 010000 010000 010000 134006 177777 0 0: one bank, loaded at
010000, started at 010000.  The game is NORD FORTRAN code up to about
116200; the FORTRAN library follows, to 134006.  Memory above 134006 holds
COMMON and must be there, zeroed.  The runtime shares its call handler and
several stubs with Skattejakt's but has its own OPEN/CLOSE, formatted I/O and
line editor.

`notes/monitor-calls.md` is the call-by-call analysis made from the image
before the port existed (register conventions, which of the MON words in the
image are code, which the game reaches).  Running the game confirmed it; the
two places it guessed are settled below.

## What SVHA added to the emulator

| call | use in SVHA | port |
| --- | --- | --- |
| MON 50 OPEN | the five files, `(GAMES)SVHA-...`, default type SYMB; access 1 (DATAFIL) and 3 | NAME.TYPE in the data directory; read-only; file numbers from 100B |
| MON 1 INBT on a file | DATAFIL is read byte by byte | raw 8-bit, EOF = error 3 |
| MON 117 RFILE | the text files, 36 words (a 72-byte record) at block * 36 words | A -> (file, flag, buffer, block, words) |
| MON 76 SETBS | block size 36 words after each text OPEN | per file |
| MON 43 CLOSE | DATAFIL after loading; each unit and then -1 at the end | |
| MON 16 MGTTY | T = 0 at every input line | skip return, type word |
| MON 4 BRKM | A = 0 (every key) at every input line | recorded |
| MON 104 HOLD | 1 s and 30 s at start-up; 5+30+10+10 s in one scene | Sleep, or nothing with --no-hold |
| MON 113 CLOCK | once, at start-up | as for Skattejakt |

MON 33, 34, 41, 62 and 73 are in the library but not reached; they are
implemented anyway (no-op, zeroed object entry, file size, refusal to set a
write size).  The game never writes a file.

**The terminal type.**  The analysis could not tell what SINTRAN answers for
MGTTY with T = 0.  The editor turns it into a flag: screen-style editing only
when bits 14 (VDU) and 13 (handles BS) are both set.  On the reference the
editor echoes `^` for a deletion and Ctrl-Q, so the answer there is not a
screen terminal; the port answers type 0 by default, and 166006B (VT100) with
`--vdu`.

**The editor's bugs.**  Delete (DEL or Ctrl-A) stores a blank at the current
position and *then* decrements it (116365-116372), so the character it means
to delete stays in the buffer.  In hard-copy mode Ctrl-Q and Ctrl-K only echo
`^` (116454 jumps to the `^` echo and back to reading without resetting the
buffer); in screen mode they reset it at 116335.  Both are faithful to the
reference.

**Output.**  Formatted records go out as LF, text, then CR with bit 7 set
(215B); OUTBT to the terminal masks the parity.  An empty or blank command
gets "I don't understand that!", not a bell; other control characters get BEL.

## Random numbers and repeatable sessions

At start-up (020524) the game calls CLOCK and computes the day number and
the minutes since midnight; Woods' RAN is seeded from them.  So a session
repeats only in the same minute of the same day.  The reference captures
(`tools/svhacap.py`) read SINTRAN's time from the login banner, start SVH
early in a minute and record it; the tests replay with `-Z` set to that
minute 28 years on.

## SAVE and RESTORE (added by the port)

The runtime reads every input line through the editor at 116275: MON 16,
MON 3, MON 4 at 116334, then MON 1 at 116350 for each key.  The game's input
subroutine GETIN has its frame at 017047; its return link, the word at
016647 (frame - 200B), is 013011 when the main loop called it for a command and 022411 when
the yes/no routine did.

- When MON 4 at 116334 is followed by MON 1 at 116350 and the link is
  013011, the port copies the machine: registers (P = the MON 1), all
  memory, the SINTRAN state (echo, break, escape), the open files (name,
  access, block size), and the prompt already printed on the line.
- When Enter is typed at that prompt, the port reads the line from the
  editor's own buffer (base, end and start at 116430-116432) as the game
  would.  If the first word, to six letters, is SAVE, SUSPEN, PAUSE or
  RESTOR, the port handles it and puts the machine back to the copy, so the
  game is at a fresh prompt and never sees the word.
- SAVE writes the copy (`SVHASAVE` file, `.SAV`).  RESTORE reads one, reopens
  the files and puts it in place.  `svha NAME` does the same at start-up.

None of this changes the program, and a session without those words is
byte-for-byte the reference (`tests/run.py`).

## The bug fixes

Two bugs made the game impossible to finish (the coffin, the soup); two more
undid what the player had done or gave a command the wrong object (the vault,
the left-over object).  The port fixes them in memory when the program is
loaded, and again whenever a snapshot is put back (so a game saved by an
earlier `svha.exe`, or with `--no-fixes`, gets them too).  A fix is applied
only where the original words are found; `--no-fixes` puts the original words
back.  The table is `svha_fixes` in `src/main.c`.  The `data\` files are
untouched.

Numbers below: the player's location is 177776; objects are numbered in the
order of the vocabulary (LID 97, GATE 103, COFFIN 104, VAULT 105, the cauldron
24, the soup 25, the key-shaped metal piece 33); the per-location table is
144505 + (location - 1) * 70 + attribute.

**The coffin.**  The LIFT routine (035150, called with an object) has a branch
for the lid: at the round room (186) the tomb's lid needs a crane; at the crypt
(31) it opens the coffin if the ring flag (134351) is set, otherwise the same
crane message; anywhere else "No lid to lift around here!".  So LIFT LID with
the ring on always worked, but no text in the game mentions a lid.  The OPEN
routine does, at 073137:

    IF (OBJ .EQ. COFFIN) CALL LIFT(COFFIN), then GOTO the per-location OPEN code
    IF (OBJ .EQ. GATE)   CALL LIFT(GATE),   RETURN

LIFT(COFFIN) matches none of LIFT's branches, and the per-location code at the
crypt (073714) is the vault's, which wants the metal piece: "You don't have the
necessary piece of metal for doing that." whatever the ring.  The fix passes
the lid (the constant at 073207, 150 -> 141) and returns like the gate's
branch (073147 `JMP I 073201` -> `JMP 073107`, an existing RETURN).  Nothing
else reads that constant.

LIFT's coffin branch also toggles: it negates the coffin's table entry (-104
closed, 104 open) and puts the metal piece in the coffin again, so a second
LIFT LID closed the coffin and showed a second metal piece, while the player
held the first.  With OPEN COFFIN working that is easy to hit.  The branch
computed the same table index twice (035202-035213); the second copy is now
`LDX -113,B; LDA I 035350,X; JAP 035237; LDA -113,B`, which goes to the
routine's closing "describe the room" call when the coffin is already open.

**The soup.**  POUR SOUP at 113463:

    IF (.NOT. (carrying the cauldron .AND. soup flag 134322 .EQ. 4)) "You don't carry any soup."
    soup flag = 0
    IF (LOC .EQ. 195) CALL POUR(SOUP), RETURN        ice pit
    IF (LOC .EQ. 25) ...
    "Okay. The soup is all over the ground, and your cauldron is empty."

POUR (116065) makes the same test, finds the flag already 0 and says "You
don't carry any soup."  Its pit branch is the one that melts the ice (sets
134404, and makes the pit's table entry 144505 + 32437 lead to 194, the top of
the pit) and empties the cauldron itself.  The fix reorders the words at
113500 so that the location is compared first and the flag is cleared only
when it is not the pit:

    LDT 195; LDA I LOC; SKP IF EQL; STZ I 134322; SKP IF EQL; JMP I (not pit)

(STZ leaves A and T alone, so the second SKP repeats the first test.)

**The vault.**  OPEN, UNLOCK and OPEN or UNLOCK with an object the routine does
not single out go to the per-location code at 073321.  It looks at the
location's OPEN entry (attribute 4: -10 at the crypt while the vault is shut,
+10 while it is open): an open door gets "It was already open.", a shut one
the location's case, at the crypt 073714 — the metal piece, then the door
routine 074304, which negates the entries on both sides of the door, and "OK".
The branch for the VAULT object (073156) skipped the test and went straight to
073714, so OPEN VAULT on the open vault shut it again, with "OK".  073714
itself says "There's no vault here!" away from the crypt, so the fix keeps
that, and sends VAULT at the crypt through 073321.  The 11 words from 073150
(the GATE and VAULT tests and their jumps, one of them dead) become

    LDA I OBJ; SAT 103; SKP IF UEQ; JMP I (GATE: 073706)
    SAT 105; SKP IF EQL; JMP 073163 (not VAULT: 073321, as before)
    SAT 31; LDA I LOC; SKP IF EQL; JMP I (073714); 073163: JMP I (073321)

No other branch of OPEN jumps into a location's case past 073321 (OPEN GATE
calls LIFT).  CLOSE and LOCK are a different routine and never toggled.

**The left-over object.**  The main loop keeps the object of the command in
-115,B (015154).  The word after the verb is only looked up if that is 0
(013063), so that a noun on its own can be followed by a verb: "LAMP", "What
do you want to do with the lamp", "TAKE".  After a command that takes a turn
the loop clears it (012773-012776: `SAA 0; STA -112,B; SAA 0; STA -115,B`)
before reading the next one at 012777.  The handlers of the commands that take
no turn jump to 012777 directly, through five pool words, and so kept whatever
object they had been given: after REMOVE RING ("Can't!"), OPEN COFFIN was OPEN RING, and the
second word of the next such command was not even looked up.  The recorded
session fuzz3 shows it on SINTRAN: GO PILL, INVENTORY LINTEL, then EAT EMERALD
answered nothing, being EAT PILL.  The five pool words now point at 012775,
which clears the object only:

| pool | exits of |
| --- | --- |
| 013641 | LOOK/EXAMINE/DESCRIBE, INVENTORY, GO and its synonyms |
| 014252 | WASH, BOW, HELP, INFO, BLAST, CALM (and the unused action 35) |
| 014474 | WAKE, DIG, SCORE, HOURS |
| 014704 | LOST, FUCK |
| 015066 | REMOVE, JUJU ("Can't!") |

The noun-on-its-own exit (013332 and 013346, pool 013410) is left alone.
Found with a scan of every vocabulary word, alone and with each of five
objects, peeking at 015154 at the next prompt (`tools/fixscan.py` is the
broader version of that scan).

**Left as they were**, all present before the fixes:

- REMOVE and JUJU answer "Can't!" to anything; the ring comes off with TAKE
  OFF RING or PUT OFF RING.
- CLOSE and LOCK with an object the CLOSE routine does not list (074723: only
  a handful are) print nothing and do nothing: CLOSE VAULT, LOCK VAULT, CLOSE
  LAMP.  CLOSE and LOCK on their own work.
- OPEN with any object the OPEN routine does not single out, or none, is the
  location's door: at the crypt OPEN LID opens the vault.
- A noun the game has something to say about but no question ("TREES": "I see
  no trees around here.") keeps its object like "LAMP" does.
- OPEN COFFIN away from the crypt now says "No lid to lift around here!" where
  it said "It's not here!" and went on to whatever OPEN does in that place.

`tests/fixtest.py` plays each case, with the fixes and with `--no-fixes`.  It
uses `--debug`, which the port keeps for this kind of work and does not list
in `--help`: at the command prompt `#peek ADDR [COUNT]`, `#poke ADDR VALUE...`,
`#find VALUE... [in FROM TO]` (the words in a row) and `#dump FILE` (all of
memory), octal, COUNT too; a trailing `.` makes a number decimal.  They are
the shared ones of all the ND-100 ports; SVHA reads its lines itself, so the
port takes a `#` line from the game's buffer, puts the machine back as it was
at the prompt, and keeps what was poked.  A `#` line is limited by the game's
editor to the width of the line, so a long poke, or a long file name, goes in
pieces.

`tools/fixscan.py` types every vocabulary word, alone and with each of eight
objects, and then TAKE, OPEN and LOOK, from five places (the building, the
grate, the crypt with the ring, the crypt with the metal piece, the ice pit
with the soup), into the port with and without the fixes, and lists the
commands whose output differs: 16,360 commands, about three minutes.  Every
difference is one of the fixes — OPEN or UNLOCK COFFIN, POUR in the pit, a
turnless command with an object — and the script says so (exit status 0).
The vault fix needs a second OPEN VAULT to show, which `tests/fixtest.py`
plays.

## Checking it

`tests/run.py` replays sessions captured on SINTRAN III under RetroCore,
running a copy of the eXo pack.  Captured with `tools/svhacap.py` (scripts,
with the clock), `tools/svhakeys.py` (editing keys, typed one key at a time —
a burst longer than SINTRAN's type-ahead buffer loses characters on the real
machine) and by hand (`first_session`).  `tools/svhafuzz.py` makes the random
scripts (QUIT and SCORE are left out: SCORE offers to quit and a random YES
takes it).  `tools/mktests.py` turns captures into `tests/ref`.

The captured sessions are the game with its bugs, so `run.py` replays them
with `--no-fixes`.  With the fixes, 14 of the 15 are still identical; fuzz3
parts at the left-over object described above, and `tests/fixtest.py` checks
that it parts exactly there.

`tests/savetest.py` checks SAVE and RESTORE through pipes, `tests/consoleplay.py`
through a hidden Windows console, `tests/fixtest.py` the bug fixes.
