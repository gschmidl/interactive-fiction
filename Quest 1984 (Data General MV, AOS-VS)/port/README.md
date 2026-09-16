# Quest (1984) — Data General ECLIPSE MV, AOS/VS

An earlier version of the multi-player AOS/VS role-playing game that was
ported from the NADGUG library tape (`../../Quest (Data General MV, AOS-VS)`).
This one comes from its own tape, `AOS-VS_QUEST_game__1984.9trk`, dumped on
14 November 1984 from `:UDD:PIPER:QUEST` — the game as it was then being
written, with its authors' own characters still in the user file.

The unmodified `QUEST.PR` and `QUEST_SERVER.PR` run on the same kind of
ECLIPSE MV emulator, with the server started automatically and a Dasher D200
terminal on the console:

    quest.bat          the CASTLE title, then the game
    quest.bat /s       straight into the game (QUEST/S did the same)

    Welcome to quest!

    What are your initials : GS
    Player name ? GERHARD
    Password ?
    That USERNAME/PASSWORD does not exist
    Do you wish to create this character? Y

Any name and password creates a character.  Then:

      ______________________      INVENTORY           Strength 20
     |      ff      ffff    |     Nothing             Thy purse is empty
     |                  ff  |                         Experience level 5
     | |              ff    |                         Intelligence level 10
     |    |  |      ff      |                         Vision 4, Perception 1
     |          GS  ff      |                         Quest level 1
     |  |  |                |                         You have no castles yet
     |                      |                         You havn't slain a dragon yet
     |mm            |  |    |                         Rings:
      ----------------------
        Tarton
        City of Tartonne
                                                      Wind direction from the East

"To prove your heroism, your first quest is to slay a dragon."

## Playing

Commands are single keys.  **H** is help, with topics (type the number and
NEW LINE): symbols, moving, commands, attack, spells, fire, siege.

| key | | key | |
|---|---|---|---|
| N S E W, arrows | move | **M** | move automatically |
| **A** | attack (cast, fight or ignore) | **F** | fire a bow or crossbow |
| **C** | cast a non-attack spell | **D** | spell status |
| **T** | take an object from your castle | **R** | release an object |
| **G** | use a spyglass | **L** | list players or castle status |
| **Q** | quit a siege | **X** | rename your castle |
| **K** | kill off a player | **?** | territory maps |
| **ESC** | leave — the character is saved | | |

Log in again with the same name and password to carry on where you left
off.  The world lives in `save\`; `data\` is never written.  **Delete
`save\` to start the world afresh.**

## What is in this version

- **A smaller game.**  `QUEST.PR` is 2.1 MB against the later 3.0 MB; the
  world file is two-thirds the later size, the castle and shared files well
  under half.  The character panel has no class yet, and there is no
  `NEW_CASTLES` or `SAVE_USER` program.
- **Eight user slots** of 266 bytes.  On the tape: DAVE, BERT (dead —
  strength 0), russ, AC, a 32-character test name, GLEN and two blank
  records.  A new character took BERT's slot.
- **The authors' tools**: `SETDAVE.CLI`, `SETJEFF.CLI` and `SETBERT.CLI` run
  FED with `DAVECOM`/`JEFFCOM`/`BERTCOM` to set strength 1024 and wealth
  20000 in slots 1, 3 and 2; `QUP.CLI` started the server (`Q_SERVE`);
  `QSET` (tape file 2, FORTRAN source and program) displays a user's record.
  QSET is not ported — its runtime start-up is one this emulator does not
  know — but its source documents the record layout.

## Checking it

    make                  # rebuilds aosvs32.exe
    sh tests/run.sh       # the title, a new player, and the same player back

## Files

- `quest.bat`, `aosvs32.exe` — the emulator.  `-c <file>` types a file on
  the D200 first; the rest of the options are the later port's
  (`aosvs32 -h`).
- `data/` — the first tape file exactly as dumped; `../src_original` has
  both tape files and `../tape` the tape image and its listing.
- `src32/` — the emulator: the NADGUG Quest port's, plus what this build
  needed.  `NOTES.md` has those details; the later port's `NOTES.md` has
  everything else.
- `notes/QUEST.sym`, `notes/QUEST_SERVER.sym` — the linker symbol tables.
