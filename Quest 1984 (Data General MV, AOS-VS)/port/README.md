# Quest (1984) — Data General ECLIPSE MV, AOS/VS

An earlier version of the multi-player AOS/VS role-playing game that was
ported from the NADGUG library tape (`../../Quest (Data General MV, AOS-VS)`).
This one comes from its own tape, `AOS-VS_QUEST_game__1984.9trk`, dumped on
14 November 1984 from `:UDD:PIPER:QUEST` — the game as it was then being
written, with its authors' own characters still in the user file.

The unmodified `QUEST.PR` and `QUEST_SERVER.PR` run on the same kind of
ECLIPSE MV emulator, with the server started automatically and a Dasher D200
terminal for every player:

    quest.bat              the CASTLE title, then the game
    quest.bat --no-title   straight into the game (QUEST/S did the same)

It is a multiplayer game, and it plays as one: **start `quest` in a second
window and that window joins the same world** as another player — see
*Playing together*.

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

## Playing together

The server has room for ten players, each at a terminal of their own, all in
one world.

    quest               start the world and play -- or join it, if it is already running here
    quest --lan         the same, and players on other computers may join too
    quest --join HOST   play in the world running on computer HOST (a name or an address)
    quest --server      run the world with nobody playing at this window
    quest --god         play at full strength and never die -- see God mode
    quest --help        every option

* **The window that started the world keeps it running.**  When its player
  leaves with ESC, it stays open until everyone else has left, then writes
  the world back and closes.  Closing it early, or Ctrl-C in it, saves every
  player's character and ends the game for all of them.  Any other window
  can simply be closed: that player's character is saved as if they had
  pressed ESC.
* Every player's terminal is sent the CASTLE title first, at the speed of a
  fast line, unless the world was started with `--no-title`; any key skips it.
* **From another computer**: start the world with `quest --lan` (Windows
  Firewall asks once), then either copy this folder to the other computer and
  run `quest --join HOST`, or use a telnet client such as PuTTY (connection
  type *Telnet*, port 4084).  Only on a network you trust — the game's own
  passwords are the only lock.  The later Quest uses port 4040, so both can
  run at once.
* `USER_DATA_FILE` holds eight characters.  The authors' own take five of
  the slots and dead BERT's is free, so three new characters fit.
* Logons take turns: a player who connects while someone else is still
  logging on waits a moment, with a note on the bottom line.  The server
  would otherwise give both the same player slot; the later port's
  `NOTES.md` has the details.

## God mode

    quest --god

For trying things out.  The player at that window plays with strength and
maximum strength 1024, intelligence and experience 10000, vision 4,
perception 5 and wealth 20000 — set again every time the game reads a
command — and cannot die: a blow that would kill simply passes, and the
game carries on.  The numbers are the authors' own.  Their FED scripts
(`SETDAVE.CLI` and friends) gave their characters strength 1024 and wealth
20000, and the game sets up its operator with the rest; vision 4 is also the
most the world allows.  This build has no classes, so nothing caps the
intelligence.

`--god` belongs to the window it is given at: `quest --god` in a second
window makes that player a god and nobody else.  `quest --server --god`
makes everyone who joins that world one, telnet players included.

**A character saved while playing as a god keeps those numbers**, so use one
made for testing.

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

    make                  # rebuilds aosvs32.exe (gcc; links ws2_32)
    sh tests/run.sh       # needs Python for the last two checks

The title, a new player and the same player back, against recorded screens;
two scripted players in one world; `god`, which walks a new player south from
Southwest Dragonia to their death, then again with `--god`, then with `--god`
and strength 1 at every DEFEND — living through the last means DIED itself
is skipped.  Then `tests/netplay.py`: two players over the network logging on
at the same moment, playing at once, one leaving with ESC and one hanging up,
one coming back through `--join --god`, and a `--server --god` world.  And
`tests/consoleplay.py`, the same through invisible consoles as `quest`
windows: the title, a window that hosts, a window that joins, `--god`, and
Ctrl-C.

## Files

- `quest.bat`, `aosvs32.exe` — the emulator.  `--title <file>` types a file
  on each player's D200 first, `--port <n>` makes the game multiplayer,
  `--god` a god; `aosvs32 --help` lists everything.
- `data/` — the first tape file exactly as dumped; `../src_original` has
  both tape files and `../tape` the tape image and its listing.
- `src32/` — the emulator: the NADGUG Quest port's, plus what this build
  needed.  `NOTES.md` has those details; the later port's `NOTES.md` has
  everything else.
- `notes/QUEST.sym`, `notes/QUEST_SERVER.sym` — the linker symbol tables.
