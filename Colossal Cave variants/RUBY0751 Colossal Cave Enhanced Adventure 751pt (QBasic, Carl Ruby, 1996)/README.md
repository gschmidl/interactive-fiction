# RUBY0751: Colossal Cave Enhanced Adventure (QBasic, Carl Ruby, 1996)

Carl Ruby played David Long's 751-point *New Adventure* (LONG0751) on CompuServe between 1982
and 1993. After tries in Apple II BASIC he rebuilt what he had seen of it in Microsoft QBasic in
the mid-1990s. The game's own history says "As of November, 1996, 223 rooms and six dead ends
have been discovered." It covers only the parts of the cave he visited, and it takes its score
ratings from his 350-point QBasic version.

In March 2014 Arthur O'Dwyer found Ruby through a 1999 forum post, and Ruby sent him the
program. O'Dwyer mirrors the files at
<https://www.club.cc.cmu.edu/~ajo/in-search-of-LONG0751/qbasic-adv751/> and quotes the
correspondence in `ruby-emails.txt` one folder up. `src_original/qbasic-adv751` holds the eight
files as downloaded on 2026-09-28 (server dates 2016-01-06):

| file | what it is |
|---|---|
| `HELLO.BAS` | start: offers the instructions, writes the lamp's life to `LGHT`, runs the game |
| `ADV751.BAS` | the game |
| `QUITS.BAS` | final score and rating (from the 350-point game) |
| `REFILL.BAS` | writes `VAR`, the data file the game loads: travel table, short room names, object names, some messages |
| `rooms.txt`, `texts.txt`, `1timers.txt` | the game's message files, merged (see below) |
| `history.txt` | "The History of Adventure", the book in the safe |

Below, the rebuilt `C:\ADV751` folder, the changes to the program, and the tests. The eXo
collection runs the game compiled with QuickBASIC 4.5. The same folder also runs under QBasic
1.1, and both give identical transcripts.

## What was missing, and how the folder was rebuilt

`ADV751.BAS` runs in `C:\ADV751\` and reads about 240 small text files from it: `RMS\ROOMn.TXT`
(long room descriptions), `1TIMERS\*.TXT`, `*.TXT`, `CHANGE\*`, `HINTS\*.TXT`. Each `.TXT` ends
with a `//` line (the reader at line 8900 stops there). Ruby first sent the messages "for easy
readability" as three listings, each made of files pasted one after another. Hotmail would not
take the program; it came later. The listings kept the `//` lines but lost the names.
`port/make_tree.py` cuts them apart and names every piece.

- **rooms.txt** is in room order, and each alternative description follows its room. The rooms
  were matched through the short names `REFILL.BAS` gives them (`B$`, e.g. "in plover room.")
  and through the travel table. `ALT27` shows after BLOW HORN opens the north wall, `ALT139`
  after the powder, and `ALT199` after PHUCE. Three entries are never read by the program; they
  keep the same kind of name:
  - `ALT38`: the east pit without its oil;
  - `ALT157`: the flooded ravine;
  - `ROOM230`: the steep tunnel, whose one-line description the program prints from `A$(34)`.

  Ruby's placeholders fill the rooms he had not found: "THIS SPACE RESERVED FOR SECRET GARDEN"
  is 166, "…ABOVE-GROUND ROOM NOT YET FOUND" twice is 167 and 168. The two identical "…CAVE
  ROOM NOT YET FOUND" are 178 and 179. Room 178, "Over the Rainbow", can be reached south of the
  Gothic Cathedral, so it shows that placeholder.
- **texts.txt** and **1timers.txt** are in the alphabetical order of their names. The names the
  program uses (`BILBOARD`, `BRIDGE0`, `BRIDGE1`, `BRSHAKES`, `CARD`, …) fall in that order one
  for one. `CASABLNC.TXT`, the piano's two `PLAY` strings, has no `//` and runs straight on into
  `CAVE.TXT`. Four entries are read by no line of the program, so their names are unknown and
  they are left out of the folder:
  - the troll shrugging off blows;
  - the boat without oars;
  - the bear eating the food;
  - the pumps overheating.
- Files go where the program opens them, not where the listing had them. `VOICE2` and
  `VOICE2A` (in texts.txt) go to `1TIMERS\`. `NOPOINT` and `POWDER` (in 1timers.txt) go to the
  top folder. `ALT139` is read from both `RMS\` and the top folder. `history.txt` is `BOOK.TXT`.
- Blank lines between one listing entry's `//` and the next entry stay after that `//`, where
  the program never reads.

`tests/check_tree.py` checks two things:
- the pieces, put back together, give each listing byte for byte;
- every file the program can open exists. That covers the names in the code, the names it
  builds at run time, and the long description of every room the travel table and the
  program's own moves can reach.

### Stand-ins

Two folders never reached O'Dwyer: `CHANGE\` and `HINTS\`.
- `CHANGE\` holds one-line object descriptions that replace an object's own after something
  happens to it.
- `HINTS\` holds the offers of help and the hints.

The program cannot show even the first room without them: line 503 reads `CHANGE\GENTLE` on
every room display. So each gets a stand-in built only from this game's own words, with nothing
from any other version of Adventure:

| file | stand-in |
|---|---|
| `CHANGE\GENTLE` | There is a cave bear here. |
| `CHANGE\CONTENTE` (Ruby's program says `CONTENTED`: MS-DOS keeps 8 letters, but DOSBox finds no such file, so the fixed program says `CONTENTE`) | There is a contented cave bear here. |
| `CHANGE\CHAIN` | There is a golden chain here. |
| `CHANGE\OLDBTTRY` | There are some old batteries here. |
| `CHANGE\POSTER` | There is a faded poster here. |
| `CHANGE\SHARDS` | There are shards of a ming vase here. |
| `CHANGE\AXE` | There is a little axe here. (the program's own line 504) |
| `CHANGE\DRAGON` | There is a dead dragon here. / There is a persian rug here. |
| `CHANGE\MASSIVE` | There is a massive iron door here. |
| `CHANGE\PLANT1`, `PLANT2`, `PLANT3` | There is a tiny little plant / a 12-foot-tall beanstalk / a gigantic beanstalk in the pit. |
| `CHANGE\CLOAK` | There is a velvet cloak here. |
| `HINTS\ASK2`…`ASK6` | Do you need help? |
| `HINTS\HINT2`…`HINT6`, `HINTS\ALT1` | The text of this hint has been lost. |
| `BB$(18)` in ADV751.BAS (with the fixes only; line 27) | First open the (Ruby's line 2115 adds "cloth bag.") |

## Changes to the program

`make_tree.py` makes three kinds of change. Each old text must occur exactly once.

### To run it here (always)

| where | original | now | why |
|---|---|---|---|
| `HELLO.BAS` 1300 | `RUN "C:\WINDOWS\ADV\ADV751.BAS"` | `RUN "C:\ADV751\ADV751"` | HELLO.BAS still started the game from an older folder. Without the extension QBasic runs ADV751.BAS and the compiled HELLO.EXE runs ADV751.EXE |
| `ADV751.BAS` 7890 | `END` | `SYSTEM` | after SAVE, leave QBasic instead of stopping in its editor |
| `ADV751.BAS` 1630 | `STOP` | `GOTO 9989` | when the cave closes, show the score (`QUITS.BAS`) instead of breaking into the editor; the endgame was never written |
| `QUITS.BAS` 5135 | `END` | `SYSTEM` | the top rating leaves QBasic like every other rating |
| `RMS\ROOM5.TXT` | `//q` | `//` | the stray "q" made the reader run past the end of the file at the slit |

### Ruby's slips (fixed unless `--no-fixes`)

Ruby renumbered his objects more than once. They are now treasures 1-44, other things 45-100
and fixed features 101-132, with `X` placeholders for treasures he had not found. Some lines
kept an old number, and each era had its own: 21 is the keys in UNLOCK but the bottle in DRINK,
and 24 is the bird in GET CAGE but the pillow in DROP VASE. So each of the 119 fixes follows its
routine's own message or logic and Ruby's current tables (`K$`, `J`, `I$`, `P$`, `CO`); none is
a blanket renumbering. `FIXES` in `make_tree.py` lists the 116 in the program, `REFILL_FIXES`
the 3 in REFILL.BAS.

| thing | what went wrong | fixed lines |
|---|---|---|
| grate | UNLOCK tested object 21 (the opal sphere) for the keys, LOCK 16 (the lyre); both expected the grate as 43, a placeholder. 1008 marked the grate with 58, so the axe lay there | 1008, 2718, 2725, 3230, 3260, 8405 |
| snake | the check used 31, a placeholder never in the Hall of the Mountain King, so the snake barred nothing | 120, 125, 2122 |
| bird, cage | the bird was 19, 24 and 48 in different lines (19 made the flowers vanish), the cage 23. GET KNIFE removed the bird (49) instead of the knife. DROP BIRD tested a flag `CAGE` that nothing sets; the cage is container 2 and open when `CI(2)` is 1 (it starts at 2, closed but see-through like the bottle), so now OPEN CAGE lets the bird out | 2055, 2057, 2059, 2120, 2125, 2128, 2390, 2820 |
| pillow | the vase checked for the pillow as 24 | 2111, 2112 |
| dwarf | marked as 54 (the pillow, which then followed the dwarves around) and 42 (which printed "42" in the room) | 1409, 1420, 2260, 2290, 2330 |
| bear | 29, 42, 44 and 97 (the Wumpus: carrying it printed "You are being followed by a very large, tame bear."). KILL BEAR now prints `BB$(5)`, the "*HIS* bear hands" line REFILL.BAS defines and nothing printed. GET BEAR, once the bear is tame, read `CHANGE\CONTENTED`, a name of 9 letters that MS-DOS cuts to 8 but DOSBox answers with "File not found", which ended the game; it reads `CONTENTE` now | 129, 300, 2052, 2350, 2920, 2930, 2945, 3075 |
| dragon | KILL's test was the wrong way round ("I see no dragon here." when it was there). Its question `WW$` was never set; it is `BB$(4)`, "With what? Your bare hands?". The dead dragon's description went to 30, a placeholder | 2300, 2360, 2385, 8967 |
| troll | KILL TROLL printed `BB$(8)`, "oil.", instead of texts.txt's troll message, which no line read (now `FENDOFF.TXT`, this project's name for it: alphabetically it lies between ENGSTART and FIND). He took only treasures below 20. Walking onto the bridge, SW from its far side or NE from the near one, crossed it whatever the troll did, and after the bridge had fallen too; only CROSS BRIDGE checked. Line 129 sent the walk to CROSS's lines only for the bear with the troll gone; now every crossing on foot goes there, so the troll stops it until he is paid or chased off, and the bear still brings the bridge down | 2310, 2230, 129 |
| clam | the clam was 47 (the lamp) and 27/28, so OPEN CLAM made the sword (27) vanish; it is 52, the oyster 53 | 2716, 2717, 2755 |
| safe | GET POSTER showed the safe but, unlike GET ALL (1921), left it out of the room | 2007 |
| LOOK | tested object 0 when none was named ("I see no  here."), and `K3` was never cleared. It repeated the long description only from a room's third view (`NU(P) > 1`), so the first LOOK after arriving gave the short one and the next room shown got the long one | 100, 4801, 318 |
| powder | DROP POWDER stopped the program with "Subscript out of range". `CLOTH`, like `CAGE`, is an open flag from before the container table, and nothing sets it: the cloth bag is container 4, open when `CI(4)` is 1. Line 2103 tested the powder as 43 in an old container code (252); it is 57, in the bag (304). `BB$(18)`, the start of "... cloth bag.", was never written and lay outside `BB$(15)`; it is a stand-in now (see above). So: open the bag, and the powder does what POWDER.TXT and ALT139 describe | 0, 27, 2103, 2115 |
| OPEN | its container search went on from where the last one had stopped (`OC`), so opening a second container could miss it and answer with the clam or the keys instead | 2701 |
| bridges | the bridge and fissure were 39 and 40 (they are 110 and 111), so CROSS BRIDGE found none. `CB$`, never set, is `BB$(11)`, "A drawbridge now spans the fissure." | 1017, 1050 |
| cave closing | waits for the magazine at Witt's End: 25 (the chalice) instead of 55 | 1113, 1150, 1205, 1408 |
| batteries | 31 and 26 instead of 56. The coins bought them in room 124, a forest, instead of 295, the vending machine DIMWRAP sends you to | 1310, 2113 |
| words | FOOD was 25 (now 50), PHONE 125 (128), PLANT 32 (107), CAVE 50 (121), TABLET 33 (104); the bottle 21 (51), the shovel 72 (66). The slippers now also click when worn | 2887, 3460, 3505, 4210, 4300, 4520, 6910, 4905 |
| limits | taking treasures 16-19 back from the building ran off the end of `V()`, which holds 15 values: "Subscript out of range". Carried things now go up to 86, the matches | 2095, 1900, 3005, 3055 |
| jumps | the plover tunnel went to 4750, a subroutine ("RETURN without GOSUB"), not 4725. ORIENT set `Q` and skipped the move. PIT moved without describing the room | 124, 1820, 6610, 6620, 6630 |
| messages | "Please answer the question first." is `BB$(1)`; `PL$` was never set | 7760, 8250, 8340, 9956 |
| EAT | with nothing named, a misplaced bracket gave `J(-1)`: "Subscript out of range" | 4620 |
| stack | QBasic 1.1 holds about 310 return addresses (measured). The room display (a subroutine) printed the safe, the grate and the tiled door through the commands' own lines, which end in `GOTO 100`, and the hint dialog, the dim lamp's warning and the lamp dying at Y2 left it the same way: each such room shown left one address behind. In a stress test (`walk8.txt`, 400 trips through the grate) the program as sent stopped after 186 commands; fixed, it goes the whole way. The displays now print and carry on, the rest return (`RETURN 100` where the original went on to the prompt) | 1003, 1008/1009, 1048, 1049, 9215/9216, 8220, 8330, 8420, 1318, 1390 |
| LOOK *word* | an unknown word after LOOK went to the "don't understand" answers by `GOTO`, so three times in four their `RETURN` had nowhere to go: "RETURN without GOSUB". They are called now, and their `GOTO 100` is `RETURN 100` | 4816, 4830, 2884, 6530 |
| health | the drain had no floor; below 0 the health messages index `H2$(-1)`. It now stops at 0 | 105 |
| IN | kept going through its table after a match: below the grate it went on from 10 to 47, into the dark | 2435 |
| tiled door | the tiles are `TILE$(4-12)` but only 1-9 were looked at, so ORANGE, NACRE and BLACK went unrecognised. A wrong tile used the light list without its first entry: the second skipped yellow, and the third fell through to "Done." as if right. The light now stops at red | 5731, 5740/5741 |
| BRIEF | LOOK in BRIEF mode skipped the "Sorry" and with it the long description it promises | 4881 |
| EXAMINE, PUT | shared BUILDING's line, so outdoors they took you into the building; they now get the answer OFFICE, WALL and TERSE get | 205 |
| debugging | OIL on anything but the door printed "88" (the routine's own answer elsewhere is 2915); LIGHT printed the lamp's state ("Your lamp is now on. 1"); LOOK IN printed the container's number | 3905, 4020, 4825 |
| spacing | `TH$` and `NH$` end in a space in REFILL.BAS, which `INPUT #` drops ("There is nothing hereto eat."); they get it back, and the two lines that had made up for it lose theirs | 29, 30, 2875, 3860 |
| LOCK GRATE | printed `GA$(0)`, which nothing sets, as a blank line; now the game's own "The grate" + " is locked." | 3260 |
| SAVE | kept the score, the places of things and the flags of the 350-point game (7860-7870), not the rest. After RESUME the grate was locked again, the concrete gone, the bag, cage and safe shut; the cave could not close (`JT`), and every treasure room paid its 2 points again (`NU`). The rest (27 variables, `CI()` and `NU()`) now follows `J()` in the file. RESUME reads it and rebuilds what the flags imply, the travel-table changes and changed descriptions, in a new routine at 27000. A name that could not be opened (none, or a mistyped one) stopped the program; now the question that led to it is asked again, SUSPEND's or RESUME's | 7840, 7850, 7871-7873, 7896/7897, 7975, 7984-7990, 7998/7999, 27000-27150 |
| names | three object names in REFILL.BAS kept the working number Ruby gave each before naming it, as the treasures he never placed are still just "28" to "43": the inventory showed "23ingot", "24rose" and "58coil of rope". Now "ingot", "rose" and "coil of rope", in `VAR` too (made again by `make_var.py`) | REFILL.BAS 25023, 25024, 25068 |

### This project's addition: the debug command `#BEAR` (with the fixes)

Nothing in Ruby's game tames the bear. FEED BEAR with the food gets `SANDWICH.TXT` ("All you have
are watercress sandwiches. The bear is less than interested."), and no line sets `BE` to 2, the
tame bear. The chain, GET BEAR (`BE` 3, following you), the troll and the bridge are all written
for it. In NEW ADVENTURE, a relative of the 751-point game, the bear is fed a honeycomb from an
apiary (a PC-SIG walkthrough of 1990, in O'Dwyer's collection). Ruby's cave has no apiary.

So `#BEAR`, typed in capitals like every command, turns the fierce bear tame. The line sits next
to Ruby's own debug commands `L` (the lamp's counters) and `DDD` (the containers). It prints
Ruby's message for the bear calming down, which no line read: "The bear eagerly wolfs down your
food, after which he calms down considerably and even becomes rather friendly." That text is now
`1TIMERS\FEDBEAR.TXT`, this project's name for it, since alphabetically it lies between DIMWRAP
and FIRSTAXE. Any other time `#BEAR` gives Ruby's "Nothing happens.".

After that the rest plays as Ruby wrote it, and `walk11.txt` tests it:
- UNLOCK CHAIN, GET CHAIN, and GET BEAR make the bear follow you.
- At the troll bridge, crossing is refused, and DROP BEAR sends the troll off with a shriek.
- Crossing with the bear brings the bridge down, and after that there is no way across.

Lines 142 and 27200-27220 (`ADDITIONS` in `make_tree.py`); `--no-fixes` leaves them out.

Everything else is Ruby's, including the typos ("hurtlqe", "blaank", "Crystaal").

## What still does not work

These parts of Ruby's program were unfinished, not mis-numbered, so they are left as he sent
them:

- There is no endgame. When the cave closes, the score is shown.
- The bear cannot be tamed in play, because nothing sets `BE` to 2. Feeding it the food gets
  the sandwich message. The debug command `#BEAR` tames it (see above).
- Carrying the soiled paper drains health, down to 0, with nothing more happening: no death,
  and no antidote. The drain may have belonged to the glowing stone, but that is a guess, so the
  number was left alone.
- A dwarf's first axe scores 52 points.
- A few messages were never written and print as blank lines: GET RUG while the dragon lives,
  FEED and WATER on the wrong things, and the lamp dying at Y2.
- Only treasures 1-15 score and can be stolen, and the ratings end at "a perfect 350 points".
- Things that are not meant to be carried can be: the Wumpus, the dog, the orks, the helicopter
  (GET refuses only numbers above 100). The helicopter flies when you get out, whether or not
  the button was pushed.
- Being eaten by the Wumpus still leaves two return addresses behind (rare).

The program only understands capital letters, as its instructions say.

## Building and running

```bat
python port\make_tree.py
python port\make_var.py DOSBOX.EXE QBASIC.EXE
python port\make_exe.py DOSBOX.EXE QB45
python port\tests\check_tree.py
python port\make_collection.py GAME_FOLDER
```

- `make_tree.py` rebuilds `port/ADV751/` and keeps its `VAR`; the EXEs go, since they have to be
  compiled again. `make_tree.py OUT --no-fixes` builds the program as Ruby sent it, plus only
  the changes that make it run here.
- `make_var.py` makes `VAR` the way Ruby did, by running `REFILL.BAS` under QBasic, again
  whenever `REFILL.BAS` changes. It uses DOSBox 0.74 (eXoDOS's copy), whose SDL 1.2 runs with no
  window; DOSBox Staging 0.82 always opens an OpenGL window. `QBASIC.EXE` is MS-DOS QBasic 1.1
  (194,309 bytes, 1993). `make_var.py DOSBOX.EXE QBASIC.EXE OUT` makes the `VAR` of another
  tree, such as a `--no-fixes` one, whose `REFILL.BAS` keeps Ruby's object names.
- `make_exe.py` compiles HELLO, ADV751 and QUITS with QuickBASIC 4.5 in the same windowless
  DOSBox, making HELLO.EXE (44 KB), ADV751.EXE (147 KB) and QUITS.EXE (39 KB).
  - It runs `BC /O /E` and then `LINK` with `BCOM45.LIB`. `/O` makes stand-alone programs that
    need no BRUN45.EXE; `/E` is for the ON ERROR in SAVE and RESUME.
  - ADV751 gives 13 warnings, all arrays Ruby used without DIM, which QuickBASIC sizes by
    default as QBasic does.
  - Neither QBasic nor QuickBASIC is in this repository.
- `make_collection.py` makes the eXo collection's copy:
  - `menu.txt`;
  - `MS-DOS\dosbox.conf` (= `port/dosbox.conf`);
  - `MS-DOS\drives\c\ADV751\`: the three EXEs and the data. It leaves out the .BAS files,
    `REFILL.BAS` and the seven room files the program never opens.

  The .BAS files go into the collection's `Sources\RUBY0751 Sources.zip`. The eight originals
  are at its root, and `port\` holds the three the EXEs are compiled from and the `REFILL.BAS`
  that made `VAR`.

  eXo's `launch_if.bat` starts it with DOSBox Staging 0.82. `dosbox.conf` runs `HELLO` in
  `C:\ADV751` and then `pause`, so the final score stays on screen. Saves go to
  `C:\ADV751\GAMES\`. `make_collection.py GAME_FOLDER --update` copies only new or changed game
  files into an existing copy. It deletes nothing, so saves stay, and it lists what is no longer
  part of the game.
- The port's tree also runs under QBasic 1.1, Ruby's own interpreter (`QBASIC /RUN HELLO.BAS`
  in `C:\ADV751`).
- `tests/smoke.py DOSBOX.EXE QBASIC.EXE walkN.txt [--qb45 QB45] [--tree DIR]` plays a list of
  commands with no window, starting from HELLO.
  - It runs under QBasic, or compiled as the collection runs it with `--qb45`.
  - It prints the screen before each command, and any error with its line number.
  - A command line starting with `#P`, `#J` or `#H` sets up a test instead: `#P 60` moves the
    player, `#J 9 300` puts object 9 somewhere (300 = carried), and `#H 2` sets how many things
    are carried. Every other line is typed, `#BEAR` too.
  - `walk1.txt` goes through the grate and into the cave, then saves.
  - `walk2.txt` resumes that save and quits.
  - `walk3.txt` exercises the poster, the safe, the dwarf and the dragon, and waters the plant.
  - `walk4.txt` plays the grate and the snake by hand: keys, cage, bird, OPEN CAGE, DROP BIRD.
  - `walk5.txt` checks the other fixes where they happen: the clam and the sword, the vending
    machine, the troll and both bridges, ORIENT, PIT, the tablet, the phone, DIG, DRINK, the
    vase, the knife, the plover tunnel, the slippers, the match, the safe and the book, the
    dragon, the bear, EAT.
  - `walk6.txt` does the powder: GET BAG, DROP POWDER with the bag shut, OPEN BAG (three times),
    DROP POWDER on the wet sand, LOOK, north to the ravine and the statue, and powder dropped
    elsewhere.
  - `walk7.txt` checks the rest:
    - LOOK AROUND six times, LOOK IN FOO, GET FOO, EXAMINE, DRINK, EAT, KILL and OIL;
    - LOCK and UNLOCK GRATE, and IN below it;
    - BRIEF and LOOK;
    - the tiled door, wrong tiles included;
    - the safe on display;
    - the health drain down to 0.
  - `walk8.txt` is the stack test: 400 trips through the grate. With `--tree` and a
    `--no-fixes` build it shows the program as sent stopping after 186 commands.
  - `walk9.txt` saves with no name (the question comes again). It then sets up a state that
    touches every flag SAVE now keeps and saves it as TEST2.
  - `walk10.txt` resumes a name that does not exist (the question comes again), then TEST2. It
    checks the grate, the concrete and its way north, the drawbridge, the horn's opening, the
    dead dragon, the oyster, the open cage, BRIEF, the worn slippers and the score, and that the
    nugget room pays no second 2 points.
  - `walk11.txt` is the bear, tamed with `#BEAR` (see above): the chain, the bear following, the
    troll chased off, the bridge crossed on foot and brought down by the bear, then no way across.
  - `walk12.txt` shows the ingot, the rose and the rope by name, and walks onto the troll's
    bridge from both sides: he stops it until he is paid, then it goes on foot and with CROSS.

  All twelve run without an error under QBasic. Compiled with `--qb45`, they give the same
  transcripts character for character. Walks 1-3 were first run, also clean, on the program as
  sent, which `--no-fixes` rebuilds byte for byte (walk 1 has since gained the U that climbs
  back out of the now open grate).
