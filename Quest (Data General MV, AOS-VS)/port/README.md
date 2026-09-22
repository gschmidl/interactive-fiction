# Quest — Data General ECLIPSE MV, AOS/VS

A multiplayer role-playing game for AOS/VS, written in Data General FORTRAN 77
around 1983–84 and distributed on the NADGUG library tape.  It is two
programs: `QUEST_SERVER.PR` holds the world, and each player runs `QUEST.PR`,
which finds the server, connects to it and then plays almost entirely through
memory the two share.

On the original machine it was started the way `QUP.CLI` and `QUEST.CLI` say:

    dir :games:quest
    delete/2=ignore quest.out
    cre quest.out
    proc/def/out=quest.out/name=quest.server quest_server
    quest

on a terminal set to a Dasher D200 (`set tti d200` / `set tto d200`).

## It plays

`aosvs32.exe` is an ECLIPSE MV emulator with an AOS/VS shim and D200
terminals.  It runs the unmodified `QUEST.PR` and `QUEST_SERVER.PR` exactly as
they came off the tape, starting the server itself.  From a Windows console:

    quest.bat

and it is the multiplayer game it was — see *Playing together* below.  (By
hand: `aosvs32.exe --port 4040 -d data -s save data\QUEST.PR`; without
`--port` it is one player and no network, the way the port began.)

    Welcome to quest!

    What are your initials : GS
    Player name ? GERHARD
    Password ?
    That USERNAME/PASSWORD does not exist
    Do you wish to create this character? Y

**Any name and password makes a new character.**  (`USER_DATA_FILE` also
holds RANDY / ZYZLVORK, who is dead.)  Then:

      ______________________      INVENTORY           Strength 17 [fighter]
     |                      |     Food rations        Thy purse is empty
     |        ............  |     Food rations        Experience level 5
     |        :             |     Water skin          Intelligence level 10
     |        : /  \        |                         Vision 4, Perception 1
     |        : GS          |                         Quest level 1
     |        :   ff  """"  |                         You have no castles yet
     |        :   ff  "     |                         You havn't slain a dragon yet
     |        :   ff  "     |                         Rings:
     |                      |
      ----------------------
        Arcolius
        City of Xenobia
                                                      Wind from North - Rainy
    -> Move to the WEST

    Hit space bar to attempt a seige on the citadel

`GS` is you (your initials, underlined on the real terminal), `ff` forest,
`mm`/`MM` mountains, `:` a road, `/  \` a citadel; terrain is dimmed.  Commands
are single keys — N S E W to move, O to observe an item, A attack, C cast,
F fire, T take, R drop, B backpack, D spells, ? maps — and the arrow keys move
the selection cursor in the menus that use one.  **ESC at the command prompt
leaves the game and saves the character**; logging in again with the same
name and password brings it back where it was, class, strength and all.

The world lives in the save directory (`save\` for `quest.bat`).  `data\`
holds the files exactly as they came off the tape and is never written to;
anything the game updates is copied into the save directory the first time it
is touched.  **Delete the save directory to reset the world** — which is what
`QUP.CLI` did by deleting `QUEST.OUT` and letting the server build everything
again.  The server's own console output goes to `QUEST.OUT` in there, as
`/out=quest.out` sent it.

## Playing together

Quest was written for many players at once, each at a D200 of their own, all
in one world run by one server.  That is how it runs now.  **Start `quest` in
a second window and that window joins the world the first one is running** —
every window is another player.  QUEST_SERVER has room for ten.

    quest               start the world and play -- or join it, if it is already running here
    quest --lan         the same, and players on other computers may join too
    quest --join HOST   play in the world running on computer HOST (a name or an address;
                        HOST:PORT, and [ADDRESS]:PORT for IPv6)
    quest --server      run the world with nobody playing at this window
    quest --god         play at full strength and never die -- see God mode
    quest --help        every option

Everyone shares the map, the castles, the weather and the beings; the title
bar of each window counts the players and, with `--lan`, says what the others
should type to join: `quest --join` and this computer's name.

* **The window that started the world keeps it running.**  When the player
  there leaves with ESC, the window stays open until everyone else has left,
  then writes the world back and closes.  Closing it early, or Ctrl-C in it,
  saves every player's character and ends the game for all of them.
* Every other window can come and go as it likes: closing one is that player
  leaving, and the character is saved as if they had pressed ESC — the way
  AOS/VS told the server about a terminal that hung up.
* **From another computer**, start the world with `quest --lan` (Windows
  Firewall asks once whether `aosvs32.exe` may accept connections), then on
  the other computer either copy this folder and run `quest --join HOST`, or
  use any telnet client — PuTTY with connection type *Telnet*, port 4040.
  With `--lan` the world listens for IPv6 as well as IPv4. A computer's name
  usually resolves to IPv6 addresses first, and with only IPv4 listening the
  firewall would drop those, so a join by name would wait about 20 seconds
  for each one (2026-09-22).
  Use `--lan` only on a network you trust: anyone who can reach the port can
  play, with nothing but the game's own passwords.
* Logons take turns.  Someone who connects while another player is still
  logging on — making a character, say — waits a moment, with a note on the
  bottom line.  QUEST_SERVER would otherwise give both the same player slot
  and they would share one character; `NOTES.md` has the details.

## God mode

    quest --god

For trying things out.  The player at that window plays with strength and
maximum strength 1024, intelligence and experience 10000, vision 4,
perception 5 and wealth 20000 — set again every time the game reads a
command — and cannot die: a blow that would kill simply passes, and the
game carries on.  The numbers are the authors' own.  Their FED scripts
(`SETDAVE.CLI` and friends) gave their characters strength 1024 and wealth
20000, and the game sets up its operator with the rest; vision 4 is also the
most the world allows.  The game still caps intelligence by class, so a god
fighter shows 3000 (a wizard's cap is 10000).

`--god` belongs to the window it is given at: `quest --god` in a second
window makes that player a god and nobody else.  `quest --server --god`
makes everyone who joins that world one, telnet players included.

**A character saved while playing as a god keeps those numbers**, so use one
made for testing.

## The terminal

On a console the D200's screen is redrawn with ANSI sequences, changed cells
only, and the game's pauses are real (a message such as *"You have been hit by
an arrow from an elven archer"* stays up for the second it was meant to).
When output is not a console — a pipe, a test — the screen is printed as plain
text each time the game waits for a key, and pauses are skipped:

    aosvs32.exe -T snap -d data -s save data\QUEST.PR < script.txt

`-T ansi|snap|raw|off` forces a mode.  A PC keyboard is mapped onto the D200's:
arrows to its cursor keys, Home to HOME, Enter to NEW LINE, Backspace to
RUBOUT.  In a script, one byte is one keystroke and a line feed is NEW LINE.
`-p <keys> <screens>` adds a scripted player (up to fourteen), and the
scripted players take their keystrokes in turn.

A telnet client or `quest --join` gets the same screen as ANSI over the
connection, and its keys come back the same way: arrow keys and Home as
escape sequences, Enter as CR, Backspace or Delete as RUBOUT, and Esc on its
own as ESC.

## Checking it

    make                  # rebuilds aosvs32.exe (gcc; links ws2_32)
    sh tests/run.sh       # all of the below; needs Python for the last two

The recorded sessions freeze the clock with `-Z` — Quest seeds its random
numbers from the time of day, so where a new player lands and what shoots at
them only repeat once the clock stops.  They cover creating a character,
moving, the observe menu, leaving with ESC, and the save: a character that
played and left comes back in the next session with its class, strength and
position.  Then two scripted players share one world, and two Python scripts
play the multiplayer game for real, in throwaway save directories:

* `tests/netplay.py` — two players join a world over the network at the same
  moment (the second waits for the first one's logon), play at the same
  time, one leaves with ESC and the other hangs up; the world stops by itself
  with both characters saved, and a player comes back through `--join`
  asking for `--god`; then everyone in a `--server --god` world is one.
* `tests/consoleplay.py` — the same through real (invisible) consoles, as
  two `quest` windows would: the first hosts and plays, the second joins the
  running world, the host's player leaves while the other plays on, and
  Ctrl-C at a hosting window saves the player and stops the world.  On the
  way, `quest --god` at the hosting window plays at strength 1024.

Not in `run.sh`, because it opens a world to the network:

* `tests/lanplay.py` — a `--server --lan` world. It says to join with
  `quest --join` and this computer's name, and a second world on its port is
  refused. One player joins by that name over IPv6. Another joins from WSL2's
  virtual machine over IPv4, which is another computer as far as the network
  goes; without WSL she is left out. The world names both addresses as they
  join, and a player comes back through `--join [::1]:PORT`.

`god` in `run.sh` walks GERHARD up and down beside Xenobia's tower until the
tower guards kill him, then again with `--god` (he lives, at strength 1024),
then with `--god` and his strength set to nothing at the very check that
calls DIED — he lives, because DIED is skipped.

## Layout

    quest.bat   start or join the world in a console
    src32/      the emulator
      mv32.c        the machine, the loader, main
      qdefs.h       processes, tasks, shared files, IPC
      sched.h       the cooperative scheduler
      quest.h       the AOS/VS calls Quest needs beyond the single-process set
      d200.h        the Dasher D200s, one per player
      host.h        the multiplayer host: players joining and leaving
      net.c net.h   sockets, the console, Ctrl-C, and --join
      syscall32.h   the AOS/VS shim shared with the Zork and Ferret ports
      mvops.h wide.h wideexec.h mvfpu.h mvdec.h   decode and execution
    tools/      q.py (disassemble with symbols), callers.py, scancalls.py,
                annot.py, mvdis.py, st.py, symmatch.py, loadg.py (the tape)
    tests/      scripted sessions, recorded screens, run.sh, netplay.py,
                consoleplay.py, lanplay.py
    notes/      QUEST.sym, QUEST_SERVER.sym and working listings
    data/       the files as they came off the tape

Both programs shipped their `.ST` linker symbol tables, so every routine named
in the source comments and in `NOTES.md` is the authors' own name for it.
Debugging switches: `-v` system calls, IPC and process switches; `-t`
instruction trace; `-B <hex>` log each arrival at an address; `-BS <hex>` the
same plus the byte strings a string instruction is about to use; `-W lo hi`
watch memory; `-D lo hi` dump a range from every process at the end;
`-Z <seconds>` freeze the clock; `-q` no server; `--server --exit-when-empty`
a world that stops once its last player has left.

`NOTES.md` is the thing to read before changing anything.

## Still to do (refine pass)

- `--join` has not run on a second physical computer: the user has one, so
  `tests/lanplay.py` uses WSL2's virtual machine as the other computer
  (2026-09-22).
- Players land in random cities, and two players meeting on the map has not
  been seen in a test.
