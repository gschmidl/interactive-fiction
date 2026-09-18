# Cave Fun — Norsk Data ND-100, SINTRAN III — native Windows port

    cavefun.exe

`cavefun.exe` is an ND-100 minicomputer with just enough SINTRAN III in it
to run one program: **ADVENTURE V4.2**, Mikael Johansson's table-driven
adventure interpreter in ND BASIC, playing **CAVE-FUN**, the adventure he
wrote for it (the game file is dated 27 February 1981).  Both are Swedish
computer club DNF material, from his floppy MIKAEL-1.  The interpreter was
compiled from its source with the club's BASIC compiler and linked with the
club's library on SINTRAN III, and what runs is that program, instruction for
instruction, on an emulated CPU; it reads the game file as it did then.

        - ADVENTURE V4.2 -

    GIVE THE NAME OF THE ADVENTURE YOU WANT TO PLAY: CAVE
    YOU'RE IN A HOUSE IN A FOREST
    OBVIOUS EXITS ARE: NORTH EAST SE SW WEST

     >

Type `CAVE` (or `CAVE-FUN`, or the whole name `CAVE-FUN-MJ`) at the first
question: SINTRAN takes the start of a file name.

## The game

A house in a forest, a road to a mountain, and behind a locked iron door
three levels of an abandoned fortress — guard rooms, officers' lodgings, a
maze, a mine, Hobbit doors, stores, a troll's bridge and a bear — with 27
treasures to bring back to the house.  It is dark inside: the lamp needs the
matches to light.  A little orc comes out of the dark now and then, and it
kills; mail helps, a weapon helps more.  The dead come back to life in the
forest, with everything they carried left where they fell (the lamp goes back
where it was found).  Falling in the dark, where there is no way on, is the
end.  The locked doors want words, and the walls and doors say which.

- **Commands** are a verb and a noun: `GET LAMP`, `ON LAMP`, `UNLOCK`,
  `READ MESSAGE`.  A direction alone moves: `N` `NE` `E` `SE` `S` `SW` `W`
  `NW` `U` `D`, or `NORTH` ... `DOWN`, or `GO NORTH`.
- `GET I` (or `GET INVENTORY`): what you carry.  `GET S` (`GET SCORE`): how
  much of the cave you have seen and your points.  `INVENTORY` alone is not a
  command in this game.
- `GET ALL`, `DROP ALL`; `LOOK` (`L`).
- At most 7 things carried, 20 with the transport cart.
- Treasures count double once they lie in the house.  The game is won when
  every treasure is there: `YOU MADE IT!`
- `SAVE` and `LOAD`, with a file name after them or asked for; `END` ends the
  game (it asks whether to save first).
- Several commands on one line, separated by commas: `N,N,GET LAMP`.

## Keys

| key | effect |
| --- | --- |
| letters | in any case: the terminal is in capital-letter mode, as the game knows no small letters |
| Enter | ends the line |
| Backspace | rubs out the last character (ND BASIC's DEL) |
| Ctrl-Q | rubs out the whole line |
| Esc | SINTRAN's user break: `USER BREAK AT ...` and SINTRAN's `@`; `CONTINUE` goes back into the game, `LOGOUT` ends it |

## Saving

`SAVE` writes the game to a file in `data\` (type `SYMB`).  On SINTRAN a
new file had to be named in quotes (`"MYGAME"`) and an old one without them;
the port takes the name either way.  `--sintran-files` asks for the old rule.

## The port's fixes

The interpreter as recovered had bugs a player could not be expected to
understand, and the game file two wrong rules.  The port fixes them
(`basic\` and `tools\fix_adv.py`; details in `NOTES.md`):

- a direction typed alone (`S`) walked straight past the troll on his bridge,
  who only stopped `GO SOUTH`; `GO N` ... `GO D` were not understood;
- after a `GET`, `GET` or `DROP` of a word the game does not know answered
  for another thing: `DROP` of it dropped the Persian rug;
- the load was counted by hand and went wrong (things a rule took away, a
  `LOAD`), and with the cart put down there was no limit any more;
- a lit lamp lying on the floor lit the room but did not stop a fall;
- `LOAD` left doors opened since the save open, while the game thought
  them shut;
- once the beanstalk was full grown, the bottle filled and emptied itself
  on every move;
- `LOCK` at the iron door by the pit said the door was locked and left it
  open.

`cavefun --prog build\original\ADV-INTER-CB-MJ.PROG` runs the interpreter as
recovered; it plays whatever game file it is given (`src_original\` has the
game as recovered).

## The adventure editor

    cavefun --editor

Johansson's "Adventure Editor V3.0" (also ND BASIC, also on MIKAEL-1): `LOAD`
an adventure, `LIST` it, `MODIFY` its words, rooms, objects, messages and
rules, `SAVE` it for the interpreter.  `HELP` lists the commands.  Output
goes to a file, or to the screen as `TERMINAL`.

## Options

| option | meaning |
| --- | --- |
| `-d`, `--data DIR` | the game's files and saved games (default: `data\` next to the program) |
| `--editor` | run the adventure editor instead |
| `--sintran-files` | SINTRAN's rule for file names (quotes for a new one) |
| `--raw` | pass the terminal bytes through untranslated (the tests use this) |
| `--no-hold` | do not wait where the program pauses |
| `--prog FILE` | run another one-bank `:PROG` file |
| `-v`, `--verbose` | log monitor calls on standard error |
| `-T`, `--trace` | trace every instruction on standard error |
| `-h`, `--help`, `--version` | |

## Files

| | |
| --- | --- |
| `cavefun.exe` | the port (`make`, gcc) |
| `data\ADV-INTER-CB-MJ.PROG` | the interpreter: `basic\ADV-INTER-CB-MJ.SYMB` through the BASIC compiler, linked with the club library and ND BASIC's run-time library (`build\`) |
| `data\CAVE-FUN-MJ.ADV` | the game, with the two rules fixed (`tools\fix_adv.py`) |
| `data\ADV-EDIT-CB-MJ.PROG` | the editor, compiled as recovered |
| `basic\` | the interpreter's source with the port's fixes, and `fixes.diff` |
| `..\src_original\` | MIKAEL-1's adventure system as recovered: interpreter and editor in ND BASIC (V4.2, V3.0) and in NORD-PL (V5.0), the game, the club library's source and its documentation |
| `build\` | the compilers' listings and object files, the libraries; `build\original\` the interpreter as recovered |
| `src\` | the emulator (shared with the other ND-100 ports) |
| `tests\` | replays of sessions recorded on SINTRAN III, the fixes shown, a winning game, a console test |
| `tools\` | the build on the reference machine, the random player, the game-file lister |

How the programs were built and checked against SINTRAN III: `NOTES.md`.
