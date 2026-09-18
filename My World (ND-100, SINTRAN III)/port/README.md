# My World — Norsk Data ND-100, SINTRAN III — native Windows port

    myworld.exe

`myworld.exe` is an ND-100 minicomputer with just enough SINTRAN III in it
to run one program: **MY_WORLD**, an adventure in ND-Pascal by Mikael
Johansson ("MJ") of the Swedish computer club DNF, who also wrote Mordor and
the Cave Fun adventure system.  It is from his floppy MIKAEL-6 (the program
of September 1983, its world of April 1983).  It was compiled from its
source with the club's ND-Pascal compiler on SINTRAN III, and what runs is
that program, instruction for instruction, on an emulated CPU; it reads its
world from the file it read then.

    You are in a forest, with trees all around you.
    Command:

## The game

A forest, a little wooden cottage on a hill, and a road north and south to
a large, rusty iron gate leading into the mountain.  Beyond it: a cobble
crawl, a hall of mists, a chamber full of birds' nests, a sphere room with a
snake in it, a fissure with nothing over it, the long hall, a museum of
stuffed animals, a soft room with pillows ("Have a pillow"), the trolls'
bridge ("STOP. Pay troll!"), and a maze of twisty little tunnels, all alike,
where a pirate lives.  Silver bars, gold, mithril and the pirate's chest are
worth points, twice as much in the cottage.  It is dark inside, and a lamp's
oil does not last for ever; there are orcs.  There is no last treasure and
no end: the score says how far you got, `QUIT` when you have had enough.
You have three lives.

- **Commands** are a verb and what to do it to: `GET LAMP`, `LIGHT LAMP`,
  `OPEN GATE`, `FILL BOTTLE`, `FREE BIRD`, `MAKE BRIDGE`, `THROW BOLA`, with
  `THE`, `A`, `AN`, `ONE`, `AND` left out.  `GET ALL`, `DROP ALL`.
- A direction alone moves: `N` `NE` `E` `SE` `S` `SW` `W` `NW` `U` `D`, or
  `NORTH` ... `DOWN`, or `GO` and a direction.
- `I` (`INVENTORY`), `SCORE`, `LOOK` (`L`), `QUIT` (`END`, `EXIT`, `Q`).
- Several commands on a line, separated by commas: `N,N,GET LAMP`.
- Small letters or capitals: the terminal is in SINTRAN's capital-letter
  mode, as on the machine the port was checked against, and what you type
  reaches the program in capitals (it shows as you typed it).  Moving in the
  dark, where there is no way, you may fall into a pit.
- Backspace rubs out the last character, as SINTRAN's delete keys (DEL and
  Ctrl-A) did; Ctrl-Q rubs out the whole line.  The program has no rubbing
  out of its own: this is the terminal driver's, and `--no-rubout` leaves the
  keys to the program.

## Walkthrough (spoilers)

**The forest.** The game starts you in one of six forest rooms, chosen at
random, and they all say the same thing.  The only way out is one exit of
one of them, onto a road, and wandering takes about 50 moves.  From any of
the six, `NE, SW, W, SE` puts you on the road (`You are at a road in a
forest.`).  Every room seen counts toward the score, so if you want the
forest's points, go round it from the road: `N, N, NE, SE, NE, N, N, S, SE`
brings you back there.  Then `S, S, E` gets you to the cottage.

**The rest**, one command at a time (the moves in brackets only visit rooms,
for their points):

    GET LAMP, GET KEYS, GET BOTTLE, LIGHT LAMP, W, S, S, OPEN GATE, S, S,
    GET CAGE, S, SE, GET BIRD, NW, GET PLANK, SE, SE, D, W, MAKE BRIDGE,
    W, W, N, GET NUGGET, S, S, S, S, GET MITHRIL,
    (SW, W, E, NE, SE, S, N, NW,) N, N, N, E, E, E, E, E,
    FREE BIRD, GET BIRD, NE, GET SILVER, SW, (SW, D, U, NE,) NE,
    N, N, S, SW, S, N, S, N, N, U, S

The last `S` is the pirate's den, deep in the maze.  The pirate's chest is
only sometimes there, so `LOOK` until it is, then `GET CHEST`, and take
anything else lying there.  Then home: `N, U, D, NW, N, XYZZY` (the magic
word takes you to the cottage), drop the treasures there (they count double),
`FREE BIRD`, and `SCORE`.  This comes to 213 points, "an expert adventurer";
the two rooms beyond the troll's bridge are left unseen, because crossing it
costs a treasure.

Two things the dice may bring:

- **The pirate** may take every treasure you carry, on any move, and hide
  it in his den: the last room above, where you then pick it up.
- **An orc**, in the dark, first throws a bola at you and runs.  Later it
  comes back with a battle-axe and follows you from room to room.  Pick up
  the bola and `THROW BOLA`: one throw in four kills it; if it misses,
  pick the bola up again.  If the orc kills you, you can be patched up.
  You then wake in the forest (use the route above), with the lamp back in
  the cottage and everything you carried left where you died.

## The port's fixes

The source as recovered had one bad sector; with it read again
(`tools\fix_sector.py`), the compiler takes it.  The program it makes stops
at the first command, on SINTRAN III as on the port, and it had bugs a
player could not be expected to understand.  The port fixes them (`pascal\`,
each marked `Port fix 2026`; details in `NOTES.md`):

- every command stopped the game with `ARITHMETIC OVERFLOW`, in two places:
  reading past the end of the command (the character 0 is one of the
  separators the reader skips), and the pirate's chance with no treasure
  carried (0 divided by 150);
- a word of more than 16 letters stopped the game (`SUBSCRIPT OUT OF RANGE`);
- `GET BIRD`, with the bird in its cage on the floor, did nothing and said
  nothing (`GET CAGE` took them);
- the plank laid over the fissure was still counted in the load, and 9
  things were then as many as could be carried;
- the pirate's chest came and went almost every turn: seen with `LOOK`, it
  was gone for `GET CHEST`, and not seen, it was there.

`myworld --data data --prog build\original\ADVENTURE-MJ.PROG` (in the
port's folder) runs the program as recovered, and so compiled.

## Options

| option | meaning |
| --- | --- |
| `-d`, `--data DIR` | the program and its world (default: `data\` next to the port) |
| `-Z`, `--clock SECONDS` | a fixed clock: the uptime stays 0, and the same commands meet the same dice (the tests use this) |
| `--uptime UNITS` | start the uptime at UNITS: one game for each value |
| `--raw` | pass the terminal bytes through untranslated (the tests use this) |
| `--no-hold` | do not wait where the program pauses |
| `--no-rubout` | do not rub characters out while typing (the program cannot do it itself) |
| `--prog FILE` | run another one-bank `:PROG` file |
| `-v`, `--verbose` | log monitor calls on standard error |
| `-T`, `--trace` | trace every instruction on standard error |
| `-h`, `--help`, `--version` | |

## Files

| | |
| --- | --- |
| `myworld.exe` | the port (`make`, gcc) |
| `data\ADVENTURE-MJ.PROG` | the program: `pascal\ADVENTURE-MJ.SYMB` through ND-Pascal J, linked with the club's `EXTRA-PAS-LIB` and `PASCAL-LIB-J` |
| `data\MY-DATA-FILE-MJ.ADV` | the world, MIKAEL-6's `ADVENTURE-MJ:DATA` under the name the program opens |
| `data\PASCAL-ERR.SYMB` | ND-Pascal's run-time error texts |
| `pascal\` | the source with the port's fixes, and `fixes.diff`; `pascal\recovered\` the source as recovered, its bad sector read again |
| `..\src_original\` | MIKAEL-6's `ADVENTURE-MJ:SYMB` and `:DATA` as they are on the floppy |
| `build\` | the compiler's listing and object file; `build\original\` the program as recovered |
| `src\` | the emulator, shared by all the ND-100 ports; built with `GAME=7` |
| `tests\` | replays of sessions recorded on SINTRAN III, the fixes shown, a game played through |
| `tools\` | the build on the reference machine, the random player, the sector reader |

How the program was built and checked against SINTRAN III: `NOTES.md`.
