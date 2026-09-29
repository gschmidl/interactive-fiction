# Adventure (ENB) — Norsk Data ND-100, SINTRAN III — native Windows port

    advenb.exe

`advenb.exe` is an ND-100 minicomputer with just enough SINTRAN III in it to
run one program: **ADVENTURE-ENB**, a Swedish princess quest in ND BASIC by
someone who signed their files ENB, from the floppy TROTYL (written in the
spring of 1983).  It was compiled from its source with the BASIC compiler of
the Swedish computer club DNF and linked with the club's library on SINTRAN
III, and what runs is that program, instruction for instruction, on an
emulated CPU.

    Vill du ha instruktioner?N
    Du är vid en bro

    ORDER:

The instructions (and a title picture) were files of their own, on another
user's area of the machine the game was played on, and they are lost: `J`
at the first question, and `H` in the game, show nothing.  What they would
have said is below.

## The game

The land is 85 by 85 squares, new every game: fields, forests, mountains,
towns, castles, old women, dragons, swords and shields and sacks of gold
lying about, a river with two bridges, a road, and somewhere the cave where
the princess is kept.  Thirty monsters (orcs, robbers, trolls, dwarves,
giants) roam it, and every one of them makes for you.  With a dagger and
leather armour you will not survive the first of them: go to a town first
and buy arms.

Down in the cave, a maze of 15 by 15 passages and rooms, the princess waits
behind two locked doors.  The rooms have things lying in their corners, but
in the dark you only see a lamp.  Bring her up to a castle.  How she rewards
you depends on how kind you were to her on the way (gifts count; weapons,
dead rats and rotten wood do not please her).

Walking in the open tires you, and in the cave every move tires you a great
deal: rest (`V`) often, or you die of it.

## Keys

The game reads single keys at `ORDER:`; its answer shows what it took the key
for.

| key | | |
| --- | --- | --- |
| ↑ ↓ → ← | Norrut, Söderut, Österut, Västerut | walk north, south, east, west (north is up on the map) |
| Home | Karta | the map, 9 by 9 squares around you |
| `?` | | where you are, what you carry, your gold, your hired men, how far the river and the road are, your defence, armour and strength |
| `T` | Ta | take: a sword, a shield, a sack of gold; water at a river; in the cave, the thing in the corner |
| `K` | Köpa | buy, in a town or a castle: `SV` sword, `SK` shield, `MA` magic sword, `RU` armour |
| `L` | Leja | hire men, in a town or a castle (they may desert if paid little) |
| `F` | Fråga | ask an old woman or a dragon the way to the cave; in the cave, ask the princess to follow |
| `G` | Ge | give: type the thing as `?` lists it (`en lampa`, `vatten`), then to whom (`PR` the princess, `GU` an old woman, `DR` a dragon) |
| `D` | Döda | kill an old woman or a dragon; in the cave, fight what stands before you (an arrow key tries to get away) |
| `N` | Ner | down into the cave, at the cave |
| `U` | Upp | up out of the cave, where you came down |
| `V` | Vila | rest |
| `S` | Sluta | end the game |
| `@` | | a SINTRAN command, run for you (the port only knows a few) |

Map signs: `. ` field, `T ` forest, `##` mountain, `St` town, `Sl` castle,
`Gu` old woman, `Dr` dragon, `Sv` sword, `Sk` shield, `Gs` sack of gold,
`==` river, `!!` road, `II` bridge, `Gr` the cave; `Or` `Rö` `Tr` `Dv` `Jä`
the monsters; `Du` you.

Letters work in either case for the commands; what you type after a
question is taken as typed (the game wants gifts in small letters).  å ä ö Å
Ä Ö are typed and shown as themselves: the port turns them into the Swedish
7-bit characters the program uses and back (`--ascii` shows `} { | ] [ \`).

## The port's fixes

The program as recovered did not start: with its instructions and title files
missing, its error handler went round in a loop for ever.  It also had bugs a
player could not be expected to understand.  The port fixes them (`basic\`,
each marked `Port fix 2026`; details in `NOTES.md`):

- with the instruction files missing, the game hung at the start, at `J` and
  at `H` (it now goes on without them);
- the princess, who lives at 15,1 in the cave, would only follow someone
  standing at 20,1, which does not exist: no one could win;
- about one game in sixteen stopped before it began (`DIMENSION OUT OF
  RANGE`), and one in 85 put the cave off the map, where no one could reach
  it: the river, the road, the cave and the player were placed up to 86, and
  the land ends at 85;
- a road takes two steps at a time, and the second one could walk off the
  edge of the land, into nowhere or a `DIMENSION OUT OF RANGE`;
- the 13th thing taken or bought in a game stopped it (`DIMENSION OUT OF
  RANGE`: water from a river twelve times was enough); now what is taken goes
  into the first empty place, and when all twelve are full, you cannot carry
  more;
- hiring more than ten men stopped the game (it asks again now, as it says
  1-5);
- in a fight above ground with men hired, the first blow killed a man, and
  every blow after it killed the same man again, taking defence and armour
  down each time, until no fight could be won;
- once the seven things in the cave's rooms were all taken, the next room
  hung the game;
- the program calls the club library's PATCH without the number it takes,
  so PATCH made an instruction out of whatever lay at address 0 and put it
  into BASIC's input routine (on the reference machine the questions came to
  end in an invisible DEL); given 0, they end in nothing, as they looked.

## Options

| option | meaning |
| --- | --- |
| `-d`, `--data DIR` | where the program is (default: `data\` next to the port) |
| `--ascii` | show the Swedish letters as SINTRAN stored them: `} { \| ] [ \` |
| `-Z`, `--clock SECONDS` | a fixed clock: the uptime stays 0, and every game is the same land (the tests use this) |
| `--uptime UNITS` | start the uptime at UNITS: one land for each value, the same every time |
| `--raw` | pass the terminal bytes through untranslated (the tests use this) |
| `--no-hold` | do not wait where the program pauses |
| `--prog FILE` | run another one-bank `:PROG` file |
| `-v`, `--verbose` | log monitor calls on standard error |
| `-T`, `--trace` | trace every instruction on standard error |
| `-h`, `--help`, `--version` | |

## Files

| | |
| --- | --- |
| `advenb.exe` | the port (`make`, gcc) |
| `data\ADVENTURE-ENB.PROG` | the program: `basic\ADVENTURE-ENB.SYMB` through the BASIC compiler, linked with the club library and ND BASIC's run-time library |
| `basic\` | the source with the port's fixes, and `fixes.diff` |
| `..\src_original\` | TROTYL's `ADVENTURE-ENB:SYMB` as recovered |
| `build\` | the compiler's listings and object files, the libraries; `build\original\` the program as recovered (it hangs), `build\start\` the program as recovered with only its error handlers mended |
| `src\` | the emulator (shared with the other ND-100 ports) |
| `tests\` | replays of sessions recorded on SINTRAN III, the fixes shown, a game won, a console test |
| `tools\` | the build on the reference machine, the random player, the memory finder |

How the program was built and checked against SINTRAN III: `NOTES.md`.
