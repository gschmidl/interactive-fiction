# Colossal Cave Adventure, 500 points — Data General AOS

The original `ADVENTURE.PR` from the *AOS Games 2 (1977–1984)* tape, run
unmodified on a Data General Eclipse emulator that answers its AOS system
calls.  Nothing is patched: the program, its overlays and its data files are
the untouched files off the tape.

    adventure.exe                  # a new game; data\ is found automatically
    adventure.exe MYGAME           # resume the game you suspended as MYGAME
    adventure.exe -s <dir> ...     # where suspended games go (default: .)

On the DG this was `X ADVENTURE` or `X ADVENTURE MYGAME`; the tape's BASIC
games menu (`GAMES/BASIC/ADV_MENU.LS`) ran it with `CLI CHAIN ADVENTURE
[ADV_RESTART]`, asking first for a restart file name.

## The game

Crowther and Woods' Adventure with 150 more points: a FORTRAN 5 program in
13 overlays, "Welcome to Adventure!!", "…score 30 out of 500 points".  The
350-point cave is all there, plus new treasures (a Cloth-of-Gold seashell, an
ivory snuff box, an ebony scarab, a geode, a bag of money, a jade carving, a
ruby, rare stamps), a monstrous spider and a rusty can with a nozzle, and new
places — a gossamer passage and a spider's web, a coral passage, a sandy
"South Seas" beach, a stalagmite room, the "Marquis De Sade Memorial Maze" and
a torture chamber.

Woods' cave hours are switched off by the data: `hours` says *"Colossal Cave is
under new management… the Cave is now open all day, every day!"*

- **Commands** are one or two words.  Type in either case; the program itself
  only understands capitals, and the emulator folds input the way a DG console
  set without `/ULC` did (`-L` passes lower case through, and then the game
  answers "no" with "Please answer the question.").
- **`save`**, `suspend` or `pause` asks for a file name, writes the game there
  and stops, telling you to resume with `X ADVENTURE <name>` —
  here `adventure.exe <name>`.  **Resuming deletes the file**, exactly as the
  original did, so a suspended game can be continued once.
- `help` and `info` explain the rest; `score`, `quit`, `brief`, `inventory`
  and `look` work as they always have.

## One line of the cave is missing, on the DG itself

The first room reads *"Around you is a forest.  A small stream flows out of the
building and down a gully."* — the opening line *"You are standing at the end
of a road before a small brick building."* is not there.  That is the DG data,
not this port: the line is absent from this game's `ADVENTURE.TXT`, from the
1977 AOS build's `ADVENTURE.TXT` and from the database source `ADVENTURE.DB` on
the *Games 1* tape, where the section header `1` is followed directly by
`1 Around you is a forest.`  The AOS/VS 350-point port uses the same database
and shows the same room.  It has been left as it was.

## Options

| option | |
|---|---|
| `-s <dir>` | where suspended games are written and looked for (default `.`) |
| `-d <dir>` | where `ADVENTURE.PR` and its files are (default: found beside the exe) |
| `-L` | do not fold input to capitals |
| `-Z` | freeze the clock, for repeatable transcripts |
| `-v` | log every system call, overlay load and file transfer to stderr |

## Files

- `adventure.exe` — the emulator (identical to `aosvs16.exe`).
- `data/` — the files the game reads, straight off the tape and never written:
  `ADVENTURE.PR` (the program), `ADVENTURE.OL` (its 13 overlays),
  `ADVENTURE.TXT` (957 fixed 72-byte message records) and `ADVENTURE.DATA`
  (the tables it reads at start-up and maps as shared pages).
  `ADVENTURE.CLI` and the linker symbol table `ADVENTURE.ST` are in
  `../src_original` with the other three.
- `src/` — the emulator source; `make` builds it, `make test` (or
  `sh tests/run.sh`) replays the recorded sessions in `tests/`.
- `NOTES.md` — how an original AOS program is run, and what had to be learned.
- `notes/ADVENTURE.sym` (the decoded symbol table), `notes/root.dis`;
  `tools/st16.py` reads an AOS `.ST`, `tools/ovlimg.py` builds per-overlay
  images for `dis.exe`.
