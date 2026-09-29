# My World port — notes

Where the program came from, how it was recovered and built, what the port
fixed, and how the port was checked against SINTRAN III itself.  Octal
throughout unless marked.

## Where it came from

The NDFS floppy MIKAEL-6 (`ND-disk-00299`), user DNF, Mikael Johansson's:

| file | dated | what |
| --- | --- | --- |
| `ADVENTURE-MJ:SYMB` | 1983-09-05 | `PROGRAM MY_WORLD`, ND-Pascal, 850 lines |
| `ADVENTURE-MJ:DATA` | 1983-04-25 | its world: 47 rooms, 22 things, 27 verbs, 24 nouns |

Both are in `..\src_original\` as they are on the floppy.  The program opens
its world as `(DNF)MY-DATA-FILE-MJ:ADV`; the port keeps the file under that
name (`data\`, byte for byte MIKAEL-6's `:DATA`).  The only library routine
it calls is `RAN`, from the club's `EXTRA-PAS-LIB`.

**The bad sector.**  The 512 bytes at file offset 16896 (the file's page 8,
its second sector) are not text: 191 of them have odd parity.  They are the
sector's own bits read 53 bits late.  Shifted back, 505 bytes come out as the
source that belongs there, every byte with even parity, joining the sectors
on either side; the late read lost the last 7 bytes, and the 3 bits left over
are the start of a space, which the indentation on either side wants 7 of
(`tools\fix_sector.py`, which says it all).  `pascal\recovered\` is the
source with the sector read again; it is the source as recovered for
everything here.

## How the programs were made

On SINTRAN III VSX/500 L under RetroCore, the machine of the other ND-100
ports (user DNF), by `tools\rebuild.py` (commands in `tools\build.cmd`,
output in `build\build.log`): ND-Pascal J, the club's compiler, the one the
Mordor port built with, each source in a session of its own; NRL with `SIZE
1500`, the program, `EXTRA-PAS-LIB` and `PASCAL-LIB-J`.  All three compile
with no errors.

| program | from | sha1 |
| --- | --- | --- |
| `data\ADVENTURE-MJ.PROG` | `pascal\ADVENTURE-MJ.SYMB` (the fixes) | `c7304d93dcc1` |
| `build\original\ADVENTURE-MJ.PROG` | `pascal\recovered\ADVENTURE-MJ.SYMB` | `1169c12d60ed` |
| `build\start\ADVENTURE-MJ.PROG` | the same with only lines 263, 289 and 741 of the fixed source (`startable()` in `tools\rebuild.py`) | `7681cb718db0` |

NRL's DUMP writes words the program never uses along with the rest, holding
what ran before it in the same session: the recovered program's bytes changed
from build to build as the fixed source did.  The fixed source keeps the
recovered one's lines, one for one, so that ND-Pascal's run-time errors name
the same line numbers.

**The program as recovered stops at its first command**, `ARITHMETIC
OVERFLOW IN LINE 226`, on the reference machine as on the port.  `STRING`,
the command, is filled out with the character 0, and 0 is one of the
separators in `BRYT`: once the command's words are used up, `WHILE
STRING[1] IN BRYT DO REMOVE_FIRST` (lines 263 and 289) never ends, and
`REMOVE_FIRST` counts `LEN` down past -32768.  Mended there, it stops at the
same command in line 741, the pirate's chance `(SKATT/150)>RAN`: with no
treasure carried the dividend is 0, and the floating divide takes a dividend
of 0 for an error, on the reference machine as on the port.  Johansson's
program cannot have done either when he played it: the compiler he had in
1983 cannot have been this one, or not in these.

## The port's fixes

At the user's direction, as for the other ND-100 ports: fix what a player
cannot be expected to understand.  Each is marked `Port fix 2026` in
`pascal\ADVENTURE-MJ.txt` (`pascal\fixes.diff`), and `tests\fixtest.py`
shows each on the program as recovered (or `build\start\`) and on the fixed
one.

| line | bug |
| --- | --- |
| 263, 289 | The separator loops ran on past the end of the command (above).  Now they stop there, `AND (LEN>0)`. |
| 741 | The pirate's chance divided 0 by 150 (above).  Now `SKATT>150*RAN`, the same chance. |
| 237 | `NEXTW` copies a word into 16 characters until a separator, with no end: a word of 17 letters or more stopped the game, `SUBSCRIPT OUT OF RANGE IN LINE 238`.  Now it copies 16 at most (`WORDNR`, beside it, always stopped at 16). |
| 511-513 | `GET BIRD`, with the bird in its cage on the floor: the word turned into the cage-and-bird, in the branch that only asks for the free bird, and nothing at all was done or said.  (`GET CAGE` took them.)  Now it takes them. |
| 557 | The plank laid over the fissure (`MAKE BRIDGE`, or dropped there) left the load, but its weight (10 of 50) and its place (1 of 10) stayed counted: with the treasures, `You can't carry that much` with 9 things.  Now they go with it. |
| 754 | The pirate's chest came and went (`47-RUM`, between his den and nowhere) at 98 turns in 100, `RAN>0.02`, and between the room's description and the next command: seen with `LOOK`, gone for `GET CHEST` (`I see no Treasure chest here!`); not seen, there.  Now 2 turns in 100: it stays a while, in the den or away. |

Not changed, as the author's own game:

- There is no end but `QUIT`: the rooms beyond the troll's bridge are "the
  temporary dead end", and the bear (`BEAR`, which would chase the troll
  off) is never set.  The troll is paid with a treasure (`THROW`), and he
  takes one that is not carried as well.
- `FILL LAMP` only fills it in the room with the pool of oil, from the pool:
  the branch that would fill it from the bottle of oil is never reached (it
  would also have left the bottle of oil in the load, `SAK[12]` for
  `SAK[13]`).
- A lit lamp keeps the player out of the pits wherever it is, carried or
  not.
- `FREE BIRD` leaves the cage's weight 1 more than it is.
- An answer of 80 characters to `patch you?` would stop the game (`STRING[81]`).
- Patched up, the player starts again in the forest, and the lamp in the
  cottage.
- `Thad had no effect!`; `LOCK` away from the gate says `Open what?`.

## SINTRAN III as the program needs it

**The terminal is in capital-letter mode**, as the reference machine's (user
DNF) is: SINTRAN echoes what is typed as it is typed, and hands the program
capitals.  The reference sessions show it: `a get the cage` is answered as
`A GET THE CAGE` (`I see no Wicker cage here!`), where the program's `NEXTW`,
which drops the articles, compares them as typed.  The program makes its
commands capitals itself (`LARGE`), so small letters must have reached it
where Johansson played, and there `a` before a word, and `y` for "patch you?"
(`STRING[1]='Y'`), would not have been understood; on this terminal they are,
and they are left as they are.

The ND-Pascal program model of the Mordor port (`src\`, `nd_pascal`), with
the uptime running with the instructions done, as the other ports now have
it (the Adventure ENB port's NOTES); `RAN` reads the uptime.  The Mordor
options (`-u`, `--new-map`) are Mordor's alone.

Since the ND-100 ports were brought onto one source (18 September 2026,
`_ND100_work\emu2\src`), My World also has Mordor's rubbing out: SINTRAN's
terminal driver holds the line while it is typed, and DEL and Ctrl-A rub a
character out of it, Ctrl-Q the whole line, before the program is given it
(the Mordor port's NOTES, *Rubbing out*).  With `--raw` a rubbed-out
character shows as `^`, as on the reference machine; at a console it is
erased.  `--no-rubout` passes the keys to the program as before.  The
recorded sessions, the fixes and the game played through are unchanged by
it.

Which echo capital-letter mode gives depends on the break characters.  A key
that is not one is echoed as it comes in, as typed, and made a capital only
when the program reads it; a break character is echoed then, after it is
made one.  My World leaves the break characters as at log-in (the control
characters), so its letters echo small; LEGEND makes every key a break
character (BRKM 0), and its letters echo as capitals.  Both are what the
reference machine showed.

## The reference machine

`tools\reffuzz.py` boots RetroCore, logs in as DNF and plays games with the
random player `tools\play.py` (moves, taking what is in sight, the game's
words, in either case, two or three to a line, dying and being patched up or
not), typing each answer only once the program has asked.  It plays
`AMJF-REF`, the port's program with `RAN` not reading the uptime (its MON 11
and the `COPY SD DA` after it made `SAA 0`), so that the same typing meets
the same dice, and the port plays it with `-Z` (its uptime stays 0);
`--original` plays `AMJ-REF`, the program as recovered.  `--script` types a
file's lines (`tests\scripts\`).  SINTRAN's terminal buffer holds 72
characters typed ahead: the 80-character command, sent all at once, lost its
last 8 and its Return there, so a long line is typed 16 characters at a
time.

## What the tests say

| | |
| --- | --- |
| `python tests\run.py` | **14 of 14 sessions identical**, 49,476 bytes: 8 random games of the port's program (`tests\ref\`, recorded while line 803 had a `LARGE` of the port's since taken out, which on this terminal changes nothing: the program as it is gives the same bytes), 2 of the program as recovered (`tests\ref-original\`: `LINE 226` at the first command), and the long word and the 80-character command in each (`tests\walk\`) |
| `python tests\fixtest.py` | 6 of 6 cases (the 7 fixed lines) shown on both programs |
| `python tests\playthrough.py` | the game played through: every treasure in the cottage, 45 rooms of 47, `You have explored 96 % of the cave, and reached 213 points! This makes you an expert adventurer!` |
| `python tools\portfuzz.py 30 200` | 0 of 30 games had trouble (each its own dice, `--uptime`): no hang, no unimplemented call or instruction, no ND-Pascal run-time error, no question the random player did not know |
