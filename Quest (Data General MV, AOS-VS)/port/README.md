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

`aosvs32.exe` is an ECLIPSE MV emulator with an AOS/VS shim and a D200
terminal.  It runs the unmodified `QUEST.PR` and `QUEST_SERVER.PR` exactly as
they came off the tape, starting the server itself.  From a Windows console:

    quest.bat

or by hand, `aosvs32.exe -d data -s save data\QUEST.PR`.

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

## Checking it

    make                  # rebuilds aosvs32.exe
    sh tests/run.sh       # replays the scripted sessions against recorded screens

The tests freeze the clock with `-Z` — Quest seeds its random numbers from the
time of day, so where a new player lands and what shoots at them only repeat
once the clock stops.  They cover creating a character, moving, the observe
menu, leaving with ESC, and the save: a character that played and left comes
back in the next session with its class, strength and position.

## Layout

    quest.bat   start the server and one player in a console
    src32/      the emulator
      mv32.c        the machine, the loader, main
      qdefs.h       processes, tasks, shared files, IPC
      sched.h       the cooperative scheduler
      quest.h       the AOS/VS calls Quest needs beyond the single-process set
      d200.h        the Dasher D200
      syscall32.h   the AOS/VS shim shared with the Zork and Ferret ports
      mvops.h wide.h wideexec.h mvfpu.h mvdec.h   decode and execution
    tools/      q.py (disassemble with symbols), callers.py, scancalls.py,
                annot.py, mvdis.py, st.py, symmatch.py, loadg.py (the tape)
    tests/      scripted sessions, recorded screens, run.sh
    notes/      QUEST.sym, QUEST_SERVER.sym and working listings
    data/       the files as they came off the tape

Both programs shipped their `.ST` linker symbol tables, so every routine named
in the source comments and in `NOTES.md` is the authors' own name for it.
Debugging switches: `-v` system calls, IPC and process switches; `-t`
instruction trace; `-B <hex>` log each arrival at an address; `-BS <hex>` the
same plus the byte strings a string instruction is about to use; `-W lo hi`
watch memory; `-D lo hi` dump a range from every process at the end;
`-Z <seconds>` freeze the clock; `-q` no server.

`NOTES.md` is the thing to read before changing anything.
