# SVHA Adventure — Norsk Data ND-100, SINTRAN III — native Windows port

    svha.exe

`svha.exe` is an ND-100 minicomputer with just enough SINTRAN III in it to
run `SVHA-ADVENTURE:PROG` exactly as it lies on the eXo SINTRAN pack
(`E:\EXO\SVHA Adventure (1979)\ND-100\SMD0.IMG`, user GAMES), reading its
five data files the way it did there.  The game is not rewritten: every
message and every dwarf is the original NORD FORTRAN program running on an
emulated CPU.  The port adds saving, and fixes two bugs that made the game
impossible to finish and two that undid your moves (below).

              WE'RE STILL DIGGING. PLEASE WAIT!!

                   WITT CONSTRUCTION COMPANY.
    This ADVENTURE is based on the ADVENTURE originally written by Don Woods
    and Willie Crowther, later expanded by Bob Supnik and Kent Blackett, and
    still later expanded by Nils-Morten Nilssen and Svein Hansen. ...
    Welcome to Adventure!!  Would you like instructions?

## The game

Colossal Cave as it grew at NTH in Trondheim: Crowther and Woods' game by way
of Bob Supnik's and Kent Blackett's RT-11 version, expanded by Nils-Morten
Nilssen and Svein Hansen and the "Studio 54 Hobbies Group" there, with parts
from Greg Hassett's Creative Computing article.  360 points.  The files are
dated August and September 1984; Norsk Data later sold it in a games pack.
The program asks for instructions at the start, and `HELP` and `INFO` explain
the rest.  Its vocabulary (421 words) is in `notes\vocabulary.txt`.

The start really does make you wait: one second, then thirty, on "WE'RE STILL
DIGGING. PLEASE WAIT!!", and one scene in the cave pauses for most of a
minute.  `--no-hold` skips the waits.

The random numbers are seeded from the day and the minute you start, as in
Woods' original, so two games are not the same.

## Saving: `SAVE` (or `SUSPEND`, `PAUSE`) and `RESTORE`

The game knows the words but answers "I don't know how (yet)."  The port
takes them over at the command prompt; the game never sees them:

                                      SAVE
    Save the game as (file name, or Enter to cancel): cave
    Saved as cave.SAV.  RESTORE brings it back, or start it with:  svha cave

                                      RESTORE
    Restore the game from (file name, or Enter to cancel): cave
    Restored.

A save is the whole machine at the moment the game asked for that command, so
the game goes on exactly where it was — the same turn, the same dwarves, the
same random numbers.  `svha cave` starts a saved game directly.  `SAVE` typed
as the answer to a yes/no question is left to the game.  Saved games go in the
current directory; `-s` names another.

## Fixed bugs

- **The coffin.**  With the magic ring on, OPEN COFFIN still said "You don't
  have the necessary piece of metal for doing that."  OPEN asked the ring
  question about the coffin where the game only asks it about the lid, and
  then carried on as OPEN VAULT.  (LIFT LID worked all along, but nothing in
  the game mentions a lid.)  Now OPEN COFFIN opens it with the ring on, and
  says the lid is too heavy without it.  Opening it a second time used to
  close it again and put a second metal piece inside; now it just shows the
  room.
- **The soup.**  In the ice pit, POUR SOUP said "You don't carry any soup."
  The game emptied the cauldron before checking whether you were in the pit.
  Now the soup melts the ice and you can climb out.  Everywhere else, pouring
  it works as before.
- **The vault.**  OPEN VAULT (or UNLOCK VAULT) on the open vault said "OK" and
  shut it again.  Now it says "It was already open.", as plain OPEN always did.
  CLOSE shuts it.
- **The left-over object.**  A command that takes no turn — LOOK, INVENTORY,
  GO, HELP, INFO, WASH, BOW, DIG, WAKE, CALM, BLAST, SCORE, HOURS, LOST, REMOVE,
  JUJU and FUCK — kept the object you gave it, and the next command
  used that instead of its own: after REMOVE RING ("Can't!"), OPEN COFFIN
  opened the ring.  Now each command gets its own object.  A noun on its own
  still waits for a verb: "LAMP", "What do you want to do with the lamp",
  "TAKE".

Still as they were: REMOVE answers "Can't!" to everything (TAKE OFF RING takes
the ring off), and CLOSE or LOCK with an object the game doesn't expect there,
such as CLOSE VAULT, says nothing and does nothing (plain CLOSE works).

The fixes are small changes to the program in memory; the files in `data\` are
the originals.  `--no-fixes` plays the game with all its bugs.  A saved game
follows the program you resume it with, so older saves get the fixes too.
`NOTES.md` has the details.

## Keys

The game echoes and edits its own input:

| key | effect |
| --- | --- |
| letters, digits | as typed; lower case is fine |
| Enter | ends the line |
| Backspace | the ND delete key (the port sends Ctrl-A); see below |
| Ctrl-Q or Ctrl-K | throws away the line typed so far — only with `--vdu`, see below |
| other control keys | refused with the bell |
| Esc or Ctrl-C | SINTRAN *user break*: the program stops |

**Backspace does not really work, and that is the game.**  Its line editor
blanks the position *after* the last character and then steps back, so the
deleted letter stays in the buffer until something is typed over it: `INX`,
Backspace, Enter is still `INX` ("I don't understand that!").  Typing on
after the Backspace replaces the letter.  The real SINTRAN system behaves the
same way.

The editor asks SINTRAN what kind of terminal it has.  The telnet terminals
of the eXo set-up do not say "screen", so a deletion echoes `^`, and Ctrl-Q
or Ctrl-K echo `^` and do *not* clear the line — that is how the reference
behaves and what the port does by default.  With a screen terminal, as an ND
site would have had, deletion echoes backspace-space-backspace and the kill
keys really clear the line; `--vdu` makes the port report a VT100 and gives
you that.

## Options

| option | meaning |
| --- | --- |
| `SAVED-GAME` | start a game saved with `SAVE` |
| `-s`, `--save-dir DIR` | where saved games are written and looked for (default: the current directory) |
| `-d`, `--data DIR` | where the game's files are (default: `data\` next to the program) |
| `--no-hold` | do not wait where the program pauses |
| `--vdu` | tell the program the terminal is a screen (backspace-space-backspace deletion) |
| `--no-fixes` | play the game with the bugs the port fixes |
| `--ascii`, `--norwegian` | how the 7-bit national characters `[ \ ] { | }` are shown (ASCII by default) |
| `--raw` | pass the terminal bytes through untranslated (the tests use this) |
| `-Z`, `--clock SECONDS` | fix the clock at SECONDS since 1970, for repeatable sessions |
| `--prog FILE` | run another one-bank `:PROG` file |
| `-v`, `--verbose` | log monitor calls on standard error |
| `-T`, `--trace` | trace every instruction on standard error |
| `-h`, `--help`, `--version` | |

The game sees the date 28 years back, as SINTRAN did on the eXo set-up; the
weekday is the same.  Started from Explorer, the window stays open at the end
with `[press any key]`.

## How faithful is it

The reference is SINTRAN III L itself: the eXo pack booted under RetroCore in
the background, logged in as GAMES over its telnet terminal port, `SVH`.  The
same keys typed there and into the port with `--no-fixes` give the **same
bytes** (`python tests\run.py`):

- a 52-command walk into the cave, to the snake and the dwarves' knives and
  axes, 13,665 bytes; the capture records SINTRAN's clock and the test replays
  that minute;
- the editing keys: DEL, Ctrl-A, Backspace, Ctrl-Q, Ctrl-K, a delete at the
  start of the line, other control characters, an over-long line, empty and
  blank lines, lower case;
- three runs of 300 random commands from the game's own vocabulary, two of
  them after walking into the cave: 162,813 bytes, all identical;
- the first session, typed by hand.

With the fixes in, 14 of the 15 are still identical.  The third random run
catches the left-over object on the real machine — GO PILL, INVENTORY LINTEL,
then EAT EMERALD answered nothing, because it was EAT PILL — and the fixed game
parts from it exactly there.  `tests\fixtest.py` checks that and every fix (and
`--no-fixes`); `tools\fixscan.py` types every vocabulary word, alone and with
seven objects, in five places (16,360 commands) into the game with and without
the fixes and confirms the two differ only where a fix is.  `tests\savetest.py` checks SAVE and
RESTORE, and `tests\consoleplay.py` plays through a real (hidden) Windows
console.

## Building

    make

needs gcc (Strawberry Perl's works).  `src\` is the same ND-100 and SINTRAN
emulator as the Skattejakt port, built with `GAME=1`; see `NOTES.md`.

## The Norwegian version

The games guide on the same pack (`GAMES/INFO-SPILL:TEXT`, in
`..\src_original\`) introduces this game in Norwegian: it exists only in
English for now; there is a more limited version in Norwegian, "but it can only
run on machines with 48 bit floating point, so we chose to release the English
one."  *Skattejakt*, on the other eXo pack, is a Norwegian Adventure that does
need the 48-bit floating point processor; it may be that version.
