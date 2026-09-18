# Cave Fun port — notes

Where the adventure system and CAVE-FUN came from, how the interpreter was
built, what the port fixed and what SINTRAN III it needs, and how the port
was checked against SINTRAN III itself.  Octal throughout unless marked.

## Where it came from

The NDFS floppy MIKAEL-1 (`ND-disk-00295`, 156 pages), user DNF — the Swedish
computer club DNF, and its member Mikael Johansson ("MJ"), who also wrote
Mordor and the club library.  Everything adventure-related on it is in
`..\src_original\`:

| file | dated | what |
| --- | --- | --- |
| `CAVE-FUN-MJ:ADV` | 1981-02-27 | the game: 184 rooms, 52 objects, 55 messages, 110 rules |
| `ADV-INTER-CB-MJ:SYMB` | 1982-10-14 | **ADVENTURE V4.2**, the interpreter, in ND BASIC ("CB" = compiled BASIC) |
| `ADV-EDIT-CB-MJ:SYMB` | 1982-10-14 | Adventure Editor V3.0, ND BASIC |
| `ADV-INTER-PL-MJ:SYMB` | 1982-10-14 | Adventure V5.0, the interpreter rewritten in NORD-PL, with Swedish, German and French |
| `ADV-EDIT-PL-MJ:SYMB` | 1982-10-14 | its editor, NORD-PL |
| `CB-LIBRARY-MJ:SYMB` | 1982-10-14 | three BASIC functions of the club library (COMPR, INTCO, COUNT) |
| `MAC-LIBRARY-MJ:SYMB` | 1986-04-16 | the club library's source (MAC assembler and NORD-PL) |
| `DOCUMENTATION-MJ:TEXT` | 1985-02-03 | the club library's manual (the author's street address blanked) |
| `COMPR-LIB-MJ:SYMB` | 1986-02-28 | a fragment of the library |

The dates are SINTRAN's, of the copies on this floppy.  The free pages of
MIKAEL-1 hold older drafts of the NORD-PL interpreter and editor.  No other
game file for the system is on MIKAEL-1 or in the catalogue of 899 ND
floppies (`floppies.json` of nd100x).

**Which interpreter plays CAVE-FUN.**  V4.2 reads exactly the file's layout
(eight numbers, then the words, rooms, objects, messages and rules), and the
file parses to its end with every exit leading to a room that exists.  V5.0
reads nine numbers (a language as well) and its strings another way; it
cannot read CAVE-FUN.  The port runs V4.2.

## The game file

`tools\advlist.py` lists it (words, rooms with exits, objects, messages and
rules in words).  In short: a line of 8 numbers (verbs, nouns, rooms,
objects, messages, rules, most things carried, the room treasures are
brought to); `max(verbs, nouns) + 1` lines `VERB,NOUN` (a leading `*` makes a
word a synonym of the one above it); for rooms 0 to N a description (one
starting with `*` is printed as it is, the others after "YOU'RE IN A ") and
11 numbers: the exits N NE E SE S SW W NW U D, and 1 if the room has light;
objects 0 to N as `DESCRIPTION,WORD,ROOM,VALUE` (room -1 carried, 0 out of
the game); the messages; and a line of 16 numbers per rule: verb, noun (0
any), five (condition, parameter) pairs, four actions.  A condition of type
0 is not a condition: its number is the next parameter of an action.  Verb 0
is AUTO: those rules come first and are tried after every turn, each with the
chance its noun gives in per cent.  The interpreter's REMs (lines 110-900)
list the 20 conditions and 19 actions.

## How the programs were made

On SINTRAN III VSX/500 L under RetroCore, the machine of the Mordor and
Legend ports (user DNF), by `tools\rebuild.py` (commands in `tools\build.cmd`,
output in `build\build.log`):

- **The compiler**: `BASIC:PROG`, "BASIC COMPILER, JANUARY 85", the one the
  club compiled Legend with.  Each program is compiled in a BASIC session of
  its own.
- **DEFAULT-INTEGER**, as the author compiled it.  Compiled with real numbers
  (the default) the program does not fit: `ADV(350,28)` alone is 30537 words
  of 48-bit reals, and SINTRAN answers `BASIC RUN ERROR ... OUT OF MEMORY
  SPACE !!!!!!!!!` before the title.  The source says which it was: it writes
  `100.` wherever it wants a real division (`IF RND>ADV(I,14)/100.`), which
  only matters when numbers are integers by default.
- `TABLE-SIZES 1500,35000` (Legend's): the fixed source is too big for the
  compiler's tables without it ("COMPILER TABLE OVERFLOW").
- **NRL** (`LDR-1935I`): `SIZE 2700` (without it, "LOADER-TABLE OVERFLOW"),
  the program, the club library `LIBRARY-MJ:BRF` as it survives (NILSSON-3,
  `ND-disk-00305`, 1985) and ND BASIC's run-time library `BASLIBR-H00:BRF`
  as the Legend port rebuilt it.  No entry is left undefined.

| program | from | sha1 |
| --- | --- | --- |
| `data\ADV-INTER-CB-MJ.PROG` | `basic\ADV-INTER-CB-MJ.SYMB` (the fixes) | `93a2b26b5ed1` |
| `build\original\ADV-INTER-CB-MJ.PROG` | `..\src_original\ADV-INTER-CB-MJ.SYMB` | `eccdf9456020` |
| `data\ADV-EDIT-CB-MJ.PROG` | `..\src_original\ADV-EDIT-CB-MJ.SYMB` | `22f62c20d3b3` |

All three are one bank from 037200, started at 037200.  NRL's DUMP writes the
memory it loaded into whole, words the program never uses included, and those
hold what ran before it in the same session (the compiler, the programs linked
before it): the second and third programs came out different from build to
build, in those words alone, and every test passes on each build.  The
command line of BASIC takes 71 characters: a longer `COMPILE` line loses its
end, and the compiler waits for ever.

`basic\ADV-INTER-CB-MJ.txt` is the source as text (parity off, LF);
`tools\mksymb.py` makes the `.SYMB` SINTRAN keeps it in (even parity, CR LF,
ETB at the end).  Without fixes it gives back the recovered file byte for
byte.

## The port's fixes

At the user's direction, as for Mordor and Legend: fix what a player cannot
be expected to understand.  `tests\fixtest.py` shows every one on both
builds.

**The interpreter** (`basic\ADV-INTER-CB-MJ.txt`, each marked `Port fix
2026`, `basic\fixes.diff`):

| lines | bug |
| --- | --- |
| 1384-1387 | A direction typed alone (`S`) never reached the rules, which only see `GO SOUTH`: the troll who stops `GO SOUTH` on his bridge let `S` walk past.  Now a direction alone is GO and that direction, as Johansson's own V5.0 does it; and `GO N` ... `GO D` work (`GO S` had even meant GO SCORE: `S` is SCORE's synonym). |
| 1623, 1704 | FLG ("an object of that word exists") was never set back to 0.  After one GET or DROP, an unknown word went on to IDX, left pointing at whatever the rule loops looked at last: `GET` said `I SEE NO XYZZY HERE` instead of `WHAT IS XYZZY??`, and `DROP` dropped IDX 35, the Persian rug, when it was carried. |
| 1615, 1660, 1671 | The load was counted by hand, +1 for GET, -1 for DROP, so it went wrong when a rule took something away (TIE HOOK, PLANT BEANS, FEED BEAR) and after a LOAD (not saved); and the test was `CAR=NADV(6)`, so with the cart put down (20 things carried, 7 allowed) there was no limit at all.  Now it counts what is carried, and tests `>=`. |
| 1570 | The fall into a pit in the dark tested `ADV(I,11)` where the lamp, object 1, was meant (`I` being what the rule loop left): a lit lamp lying in the room lit it for LOOK but did not stop a fall. |
| 1190, 2094, 2095 | A saved game holds only the rooms it had swapped (doors opened): LOAD left any door opened since then open, with its flag saying shut, and opening it again closed it.  LOAD now reads the adventure again first. |

**The game file** (`tools\fix_adv.py` writes `data\CAVE-FUN-MJ.ADV`;
7 bytes differ):

| rule | bug |
| --- | --- |
| 18 | cleared flag 189, which rule 16 had already cleared, where 188 was meant: once the beanstalk was 40 m, rule 17 ran every turn and swapped the empty bottle and the bottle of water on every move |
| 64 | LOCK at the iron door by the pit had action 210 (swap two rooms) where 202 (set flag 192) was meant: it swapped the rooms 192 and 210, which do not exist, and the door stayed open.  Now it sets 192, which rule 7 answers by shutting the door, as rule 63 sets 193 to open it |

Not changed: `INVENTORY` and `SCORE` alone are not commands (the game's own
design, V5.0's too: `GET I`, `GET S`); a pit in the dark ends the game;
`LOAD` of another adventure's saved game ends the game (`WRONG ADVENTURE`).

**Checked.**  `tests\walk-debug.txt` plays the game through with the fixes:
432 commands, all 27 treasures in the house, `YOU MADE IT!`, 628 points,
"RATES A 9" (`tests\winnable.py`).  Its first line is a `--debug` poke that
turns the orc off (rule 29 becomes a verb rule, and the AUTO rules stop
before it); otherwise it plays as typed.  Spoilers: the route is that file.

## SINTRAN III as the programs need it

Added to the emulator the Skattejakt, SVHA, Mordor and Legend ports share
(`src\sintran.c`), each seen on the reference machine:

| | |
| --- | --- |
| file names | **abbreviated** as SINTRAN does, part by part: `CAVE` opens `CAVE-FUN-MJ:ADV` (a name given in full wins; two that fit are `AMBIGUOUS FILE NAME`) |
| new files | a name in quotes is a new file, created, and must not exist; one without must.  The port takes either for either (`easy_files`) unless `--sintran-files` |
| `TERMINAL` | not a file of the user's: SYSTEM's, the user's own terminal (device 1); the editor's LIST writes to it |
| echo | a program that never sets ECHOM gets SINTRAN's log-in echo: printable characters, and Return as a bare CR (the BASIC runtime then starts the line itself).  Legend sets ECHOM 1 at once and sees no CR |
| capitals | the terminal starts in @TERMINAL-MODE's capital letters (the program knows only capitals): SINTRAN echoes a key as it was typed and hands the program its capital (seen with the My World port's sessions) |
| `--debug` | a line typed starting with `#` is the port's: `#peek`, `#poke`, `#find VALUE...`, `#dump FILE`; the program never sees it (`tools\findvars.py` finds ROOM and the arrays with it) |

## The reference machine

`tools\reffuzz.py` boots RetroCore, logs in as DNF and plays games with the
random player `tools\play.py` (moves, taking what is in sight, the game's
words; saved games under new quoted names), typing each answer only once the
program has asked.  `--original` plays the interpreter as recovered (`AIC`)
with the game file as recovered (as `CAVE-ORIG:ADV`: the two game files side
by side make `CAVE` ambiguous there).  `--script tests\walk-debug.txt` types
the winning route without its poke: on SINTRAN the orc is left in, and the
game goes where the dice take it — a long session, whatever happens.

The program never calls RANDOMIZE, so every game plays the same for the
same typing on SINTRAN and on the port.  `tests\run.py` gives each session's
typing to `cavefun.exe --raw --no-hold` on a scratch copy of the game file
and compares every byte.

## What the tests say

| | |
| --- | --- |
| `python tests\run.py` | **17 of 17 sessions identical**, 187,064 bytes: 8 random games of the port's program (`tests\ref\`), 8 of the program and game file as recovered (`tests\ref-original\`), and the winning route typed on SINTRAN with the orc left in (`tests\walk\`: it kills the player early, and the game goes on from there) |
| `python tests\fixtest.py` | 9 of 9 fixes shown on both builds |
| `python tests\winnable.py` | the game won, 628 points |
| `python tools\portfuzz.py 30 200` | 0 of 30 games had trouble (and 0 of 10 with `--original`): no hang, no unimplemented call or instruction, no BASIC run error, no prompt the random player did not know |
| `python tests\consoleplay.py` | 6 checks at a real console (ConPTY): the title, small letters, Backspace, Esc and CONTINUE, SAVE without quotes, END, the editor |
