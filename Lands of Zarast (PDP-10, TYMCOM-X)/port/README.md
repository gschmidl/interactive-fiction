# Lands of Zarast — native Windows port

A homebrew Dungeons-&-Dragons-style adventure written in Tymshare BASIC for
a PDP-10 running TYMCOM-X, dated 27 December 1984, and signed by its author:

>          IF YOU HAVE ANY QUESTIONS, SEND MAIL TO STAFFORD.
>
>                  SAURON THE FEARED

It was recovered from the `mpl` directory of a Tymshare tape, where it
survives as five compiled programs and its world file, with the author's own
instructions alongside them. Other people's characters — `GIJOE.ADV`,
`SHERA.ADV`, `KARATE.ADV` and half a dozen more — turned up in a different
user's directory on the same tape, so it was being played across accounts.

    bin\zarast.exe      the December 1984 game
    bin\zarast87.exe    the later one, 1987-88

## Two versions

The tape kept the game twice. `mpl` has the December 1984 programs, and
`novafield` -- the same directory the other players' characters were in --
has a later build of the same game, `PUB.SHR` and `B.SHR`, with its
character generator `CR.SHR`, and, unlike the 1984 set, **the BASIC
source**: `PUB.TBA`, 134KB of TYMBASIC dated 1987-02-12, and `CHARC.TBA`.

The content is the same game. Every one of the 516 long strings in the two
images -- room descriptions, monsters, objects, messages, the vocabulary,
the spell and rank tables -- is identical; what differs is the code, some
220 words of it out of 51,000. The 1987 set also has only the one character
generator where 1984 had a long form and a quick form.

They share everything else, and this port lets them: characters and the
world file work in either, in either direction, and the characters that
came off the tape in 1984 load into the 1987 game.

## What is actually running

Nothing was rewritten. The five original `.SHR` files are embedded in the
executable word for word as they came off the tape, and a DECsystem-10
emulator runs them, answering the TYMCOM-X monitor calls the Tymshare BASIC
runtime makes. The dice, the combat maths, the room text, the wandering
monsters and the mis-spellings are the 1984 program's own.

Characters this port writes are byte-compatible with the ones on the tape,
and the ones on the tape load and play here.

## Playing

    zarast

The first run does the two things the game needs and cannot do for itself.
It builds the world file, `NEWADV.DAT`, with the author's own `FILER`
program, and it rolls you up a character with his `CHARC`, because DUNGEN
will not start without one and offers no way to make one — answering YES to
*DO YOU WANT TO STOP AND ROLL UP A CHARACTER* stops the program, since on
Tymshare that is where you typed `RUN CHARC`. After that, `zarast` goes
straight into the game. Add more characters at any time with
`zarast newchar`, and the dungeon will let you take a party of them in.

Type in whatever case you like: the Tymshare monitor folded terminal input
to upper case before a program saw it, and so does this. (`--lower` turns
that off, but the game compares against `Y` and `N`, so you will not enjoy
it.)

| command | what it runs |
|---|---|
| `zarast` or `zarast dungeon` | `DUNGEN`, the dungeon crawl — the game |
| `zarast overworld` | `VENTUR`, an older stand-alone adventure |
| `zarast newchar` | `CHARC`, character creation, quick |
| `zarast character` | `CHARAC`, character creation, the long form |
| `zarast filer` | `FILER`, rebuild the world file |
| `zarast87` or `zarast87 dungeon` | `PUB`, the later game (Sept 1988 build) |
| `zarast87 older` | `B`, the May 1987 build of it |
| `zarast87 newchar` | `CR`, its character generator |

| option | effect |
|---|---|
| `-d DIR` | keep characters and the world in `DIR` (default: the current directory) |
| `-q` | do not pause where the game pauses |
| `-e` | echo what you type, the way the Tymshare monitor did |
| `--lower` | pass lowercase through instead of folding it to upper |
| `-t MIN` | freeze the clock at `MIN` minutes past midnight |
| `-v` | report monitor calls the port does not implement (`-v -v` for more) |
| `-T` | trace every instruction |

Everything the game keeps lives in one directory, as it did on Tymshare:

| file | what it is |
|---|---|
| `NEWADV.DAT` | the world — rooms, monsters, objects |
| `<name>.ADV` | one character |
| `SCENAR.IO` | your game in progress. Delete it to start the scenario over; **never** delete `NEWADV.DAT` |
| `TRACK.ADV` | the game's record of who has played |

The author's own instructions are in [docs/INSTRUCTIONS.txt](docs/INSTRUCTIONS.txt)
and his command list in [docs/COMMANDS.txt](docs/COMMANDS.txt). Both are
worth reading first: thieves can `CLIMB WALL`, `PICK POCKET` and backstab,
magic-users `CAST SPELL`, and `*STATUS` shows your character.

### Playing the 1984 world

`data\newadv-1984.dat` is the world file as it stood in the `mpl` directory
in December 1984, five values different from a freshly built one — the world
as people left it. Copy it over `NEWADV.DAT` in a save directory to play
that one, and drop in the characters from `dump_original\novafield\` to play
as the people who were playing it.

## Building

Needs a C99 compiler; built and tested with the mingw-w64 gcc 15.2 that
ships with Strawberry Perl.

    make

or `build.bat`, or directly:

    gcc -std=c99 -Wall -Wextra -O2 -o bin/zarast.exe \
        src/main.c src/cpu.c src/monitor.c src/load.c src/images.c -lm

`src/images_84.c` and `src/images_87.c` are generated from the tape files
by `tools/mkimages.py`, which also carries each version's program table and
banner — everything that differs between the two binaries. Both `.exe`s are
self-contained and need no runtime files.

## Notes

How the images were decoded, why every Tymshare BASIC program on that tape
is one word short, and where the monitor-call semantics came from are in
[docs/REVERSE-ENGINEERING.md](docs/REVERSE-ENGINEERING.md).

One thing that looks like a bug in the port is the program's own: the first
command of a dungeon session is always answered `WHAT?`, whatever it is —
a blank line gets the same reply, and the command works if you type it
again. The game is answering; it just does not take the first one.
