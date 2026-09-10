# AMAZON — what survives, and what I wrote

AMAZON was Carl Baltrunas' multiplayer adventure for the Tymshare PDP-10s.
It was never finished. This document is the line-by-line accounting of which
part of the port comes from the tapes and which part does not, so that anyone
who later recovers more material can tell the two apart without reading code.

**The short version: AMAZON survives as design documents only. There is no
compiled game. Nothing here is a decompilation.**

---

## 1. The complete surviving file set

I searched every directory of the Tymshare tape collection and every file
catalogue in `carl/` (`carl.dir`, `allsai.srt`, `700xxx.dir`, `cua646.dir`,
`gs021.dir`, …). The catalogues agree with the tapes: these seven files are
everything AMAZON ever left behind.

| File | Date | Where | What it is |
|---|---|---|---|
| `ROOMS` | 22-Sep-77 | `cb_games/` | 21 rooms: number, name, description, conditional messages |
| `PRTEST.SAI` | 03-Sep-77 | `cb_sail_arc/` | multi-process TTY framework — the multiplayer substrate |
| `CMDLIB.SAI` | 11-Sep-77 | `cb_sail_arc/` | `MAKCMD`: the verb classes and the direction codes |
| `AMAZON.SAI` | 18-Dec-77 | `cb_sail_arc/` | the command loop, `ACQUIRE`, `RELINQUISH` |
| `AMAZON.OFF` | 21-Oct-78 | `cb_games/` | MACRO-10 item control blocks and attribute words |
| `OBJECT.TYP` | 29-Jul-79 | `cb_games/` | ~90 objects and creatures with their behaviour |
| `AMAZON.TXT` | 09-Sep-79 | `cb_games/` | item/property syntax, the HELP text, the Riddle Room, rooms 2–21 |
| `AMAZOT.SAI` | 08-Jan-81 | `cb_sail_arc/` | a 20-line parser test harness |
| `AMAZOT.SAV` | 08-Jan-81 | `cb_games/` | the compiled form of `AMAZOT.SAI` |

### `AMAZOT.SAV` is not a compiled AMAZON

This is worth stating plainly, because the filename invites the opposite
conclusion.

`AMAZOT.SAV` is 50055 bytes = 10011 PDP-10 words in the Tymshare tape
encoding (five septets per word: each byte carries 7 bits, and bit 7 of the
fifth byte supplies the word's low bit — the same encoding as the Lands of
Zarast tapes). Decoded, it is a standard TOPS-10 `.SAV`: a series of IOWD
blocks, `777777,,000113` first.

Its string content is **entirely the SAIL runtime** — `DRYROT` messages,
`String space exhausted unexpectedly.`, `End of SAIL execution`, the
UUO/opcode name tables. The only non-runtime string in all 10011 words is:

```
ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789*-
```

which is the argument to `SETBREAK` on line 15 of `AMAZOT.SAI`. There is no
room text, no item table, no riddle, no bartender question anywhere in the
file. The two files also carry the *same* directory timestamp — `08-Jan-81
05:23` in `CB-GAM.DIR`.

So `AMAZOT.SAV` is `AMAZOT.SAI` compiled: a harness that reads words and
echoes them back between vertical bars. Reverse-engineering it further would
recover the SAIL runtime, not the game.

---

## 2. Verbatim, from the tapes

These are reproduced character for character, original spelling and all.
Where the source has a typo I kept it: *senetnced*, *There ais something*,
*in a tiny Closet with a which*, *Diamond Tiera*, *Phanthom*, *Crockadile*,
*WIld Honeysuckle field*, *recieve*, *Trikster*, *suprises*, *INTELLEGENCE*.

**All 21 room names and descriptions** (`world.c`, `rooms[]`) — from `ROOMS`,
except the Riddle Room's, which is the `Description` property of the `RIDLRM`
item in `AMAZON.TXT`.

**The room conditional messages**, the lines beginning `....` and `....*`.
Each is printed under the condition its wording implies — the desk's
`Locked`/`Unlocked` pair on the desk's lock state, the bartender's five lines
across the Bar Exam, the honeysuckle's four lines on whether you carry honey,
and so on.

**The HELP text** — `AMAZON.TXT`, `Item 'HELP'`, the `Description` property.

**The Amazon River Bank sign** — the long "Welcome to the Amazon River
Valley" paragraph, `ROOMS` room 19.

**The Padded Cell book** — `Yellav Nozama Eht Sdraziw Dnarg` and
`Nozama :drow cigam,  Xnihpfs Cxx -- Enasni Eht Fo Yraid.` Read backwards:
*Grand Wizards The Amazon Valley*, and *Diary of the Insane — … magic word:
Amazon*. The magic word is recoverable from the title alone, which is visible
while the book is still fastened shut, so the puzzle is solvable.

**The command vocabulary and the direction codes** — `CMDLIB.SAI`'s `MAKCMD`,
including the "strength" column, which is 1 for `GET`/`TAKE`/`DROP` and 2 for
`STEAL`/`ROB`/`THROW`/`TOSS`, and which for the `DIRECTION` class is the
direction ordinal: `N`=0 `NE`=1 `E`=2 `SE`=3 `S`=4 `SW`=5 `W`=6 `NW`=7 `U`=8
`D`=9. `parse.c` carries the table in CMDLIB's own order.

**The command scanner** — `SETBREAK(1,"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789*-",
null,"KXNS")` with SAIL's `N` (complement the break set), `S` (skip
separators), `K` (keep) and `X` (no error) modes, giving a scanner that
returns successive runs of `[A-Z0-9*-]`. Input is upper-cased, as a 1977 TTY
would deliver it.

**The stock replies** — `AMAZON.SAI` prints `<word> WHAT?`, `<word> Taken.`,
`<word> DROPPED.` and `What?`, and prompts with `CR LF >`. All four, and the
prompt, are exact.

**The data model** — `amazon.h` is `AMAZON.OFF` transcribed. The item control
block is `IT$TYP / IT$CTL / IT$IDP / IT$LOC`; the control bits are the four
2-bit `CT$SLF / CT$GRP / CT$CMP / CT$OTH` fields; the attribute words are
`AT$PRM` (`STR INT WIS CHA FLT`, five 5-bit fields), `AT$PHY`
(`CON SIZ AGI DEX`), `AT$LNG` (36 language bits), `AT$HTS`
(`AT$DIE AT$HIT AT$HPL`, three 9-bit fields) and `AT$EXP`. The live game
really stores its numbers in those bit positions, which is why stats cap at
31 and hit points at 511. The 36 `P$xxxx` player types are carried over
complete, including the unused bit 9, under the note `AMAZON.OFF` itself
puts on that page: *THIS PAGE IS OUTMODED — HERE FOR HISTORICAL NOW.*

**The statistics block** — `PPN`, `CODE`, `TURNS`, `TIME`, `DEATHS`, from the
worked `Item 'Fred'` example in `AMAZON.TXT`. `STATS` prints them.

**The object and creature notes** — every `EXAMINE` prints that object's
`OBJECT.TYP` line verbatim. So do many of the game's responses, which quote
the document directly ("Allergic to water", "must have GOOD-MARK to pass",
"Requires a truck to deliver them").

**The Riddle Room machinery** — `AMAZON.TXT` gives it as an action table:

```
(ENTRY  (SET (RIDDLE RAND(RIDLEN))) (SET (RIDANS FALSE)))
(READ   (DISPLAY  RIDTXT(RIDDLE)))
(ANSWER (EQUAL (RIDCOD(RIDDLE) INTEXT) (INCREMENT LOCATION(CRETIN))))
```

Entry picks a riddle at random and clears the answered flag; `READ` displays
it; `ANSWER` compares your word against the riddle's code. All three are
implemented as written. *(One discrepancy: the action says `INCREMENT
LOCATION(CRETIN)` — move the player to the next room number — while `ROOMS`
says you leave "the way you came in". I followed `ROOMS`, because incrementing
the room number would drop you somewhere arbitrary. The literal action table
is preserved in the comment at that spot in `game.c`.)*

**The 16-player limit and the message ring** — `PRTEST.SAI` sizes every table
`[1:16]` and `user!message[0:40]`. Both numbers are used. Its arrival
announcement, `New User: <name> added by Federal Marshal`, is reproduced
exactly, and `WHO` follows its `User # n is [name]` format.

---

## 3. Written for this port

### 3.1 The 36 riddles and the 36 Bar Exam questions

Their *count* is documented and nothing else is:

- `ROOMS` room 1: "You must answer a riddle to leave (the way you came in)
  ....there are 36 different riddles."
- `ROOMS` room 10: "....*The bartender asks you a question. (There are 36
  different questions)"

`RIDTXT` and `RIDCOD` were never written to any file that survives. The 72
items in `src/riddles.c` are new writing. They are drawn only from AMAZON's
own material — every riddle answer and every exam answer is a room, object or
creature named in `ROOMS` or `OBJECT.TYP` — and nothing is taken from any
other game. `data/riddles.txt` and `data/barexam.txt` hold the same text in
plain form so they can be swapped out if the originals ever turn up.

The Bar Exam is a law exam because `OBJECT.TYP` says it is: *"Bar Exam Diploma
- Qualifies person as lawyer, must answer questions to recieve"* and *"Bar and
Grill - Place to take bar exam, etc. Lawyers & policemen"*.

### 3.2 The map

**No surviving file records a single room connection.** `ROOMS` is a flat
list. Every exit in `world.c` is mine. Those with a textual basis:

| Connection | Basis |
|---|---|
| Elm Grove → W → Entrance to Skull Cave | the sign in room 12: "This way to the Skull Cave, Beware!" |
| Entrance → N → Skull cave | room 13: "The entrance leads through the Mouth of the Skull" |
| Amazon River Bank → U → Top of Cliff | room 19 is below, room 4 is the top |
| Top of Cliff → catapult → The Shipyard | "Airplane,Paper - Necessary to fly across the AMAZON river" |
| The Shipyard → N → Wendy's Room | Hook, Wendy and Peter Pan are one cluster in `OBJECT.TYP` |
| Jail ↔ Court Room ↔ Execution Chamber | rooms 5, 6 and 7 read as one sequence |
| Broom Closet ↔ Witch's Castle | room 11's "*Wicked Witch outside in the hallway" |
| Padded Cell ← Asylum halls | room 21: "wandered through the halls of an Asylum" |
| Skull cave → D → Shadow Room → Riddle Room / Amazon Room | the Shadow "lurks in dark passages"; rooms 1 and 2 have no daylight |
| Swamp ↔ Marsh | rooms 14 and 15 are adjacent terrain |

The rest — Bar/Shop/Honeysuckle/Elm adjacency — is mine with no textual
basis at all. `docs/MAP.txt` has the whole graph.

### 3.3 Verbs beyond CMDLIB

`CMDLIB.SAI` defines exactly three classes: `ACQUIRE`, `RELINQUISH`,
`DIRECTION`. `AMAZON.SAI` dispatches only `GET`, `TAKE`, `DROP`, `THROW` and
`done`. Everything else in `parse.c` below the marked line — `LOOK`, `READ`,
`ANSWER`, `OPEN`, `UNLOCK`, `EXAMINE`, `ATTACK`, `KISS`, `EAT`, `DRINK`,
`SLEEP`, `RIDE`, `LAUNCH`, `TIE`, `LIGHT`, `BUY`, `ORDER`, `DELIVER`,
`PLEAD`, `STUDY`, `HINT`, `SCORE`, `STATS`, `WHO`, `TELL`, `SOURCE` — is
mine, added because the `ROOMS` and `OBJECT.TYP` situations are otherwise
unplayable. (`WHO` and `TELL` are at least `PRTEST.SAI`'s commands.)

### 3.4 Every number

`OBJECT.TYP` never gives a price, weight, hit point total or reward. All of
them are mine: shop prices, carrying capacity, hit dice, damage, reward
values, the Shadow's nightfall countdown, the Grue's odds, the five-in-a-row
pass mark on the Bar Exam.

The starting attribute roll is mine too. `AMAZON.TXT`'s worked example shows
`STRENGTH 100, INTELLIGENCE 47, WISDOM 14 …`, but that is the earlier text
format; `AMAZON.OFF`'s 5-bit fields cannot hold those values, and the binary
format is the later document. I used the binary format and rolled 8–17.

### 3.5 Object placement

`OBJECT.TYP` lists objects without locations. A few place themselves — the
broom and the black hat are in room 11's text, the White Stallion and the
purple costume in room 16's, the desk and drawers in room 9's — and the rest
I distributed by theme. Notably mine:

- The Dog starts in the Marsh, not with the Phantom. `OBJECT.TYP` says the
  Dog is *"Looking for … Phanthom"* and that the Phantom *"Gives GOOD-MARK
  for helping him"*, and the Wolf *"Must have GOOD-MARK to pass"*. Bringing
  the Dog back is the way in to the lower caves. If the Dog started beside
  the Phantom, the caves would be sealed.
- The shadow is in Pandora's Box, not Wendy's box. `OBJECT.TYP` says both
  contain it — *"Pandora's Box - Hidden in wendy's room, contains Peter Pan's
  shadow"* and *"Wendy's box - Hidden in wendy's room, contains 'the
  shadow'"*. There is one shadow object, so one box is a decoy.
- Tarzan and Jane are in the Swamp ("surrounded by trees on all sides")
  rather than the open field of the Elm Grove.

### 3.6 The Bar Room as the collectors' drop

`OBJECT.TYP` gives most reward items an owner to return them to. The
jewellery and the five key sets have none. I made the Bar Room their drop,
reading the second word of its sign — *"Examiners & Collectors"* — as the
counterpart to the Bar Exam, which accounts for the Examiners. This is an
inference, not a document.

### 3.7 Vehicles

`OBJECT.TYP` says the carrots "Require a truck to deliver them", which only
works if a truck is something other than a thing you pick up. So the Delivery
Truck, the White Stallion and the White Pony are vehicles: their own weight is
not counted against you, and the truck adds to what you can haul. The flag and
the numbers are mine; the requirement is the document's.

### 3.8 The fine

`OBJECT.TYP`: *"Policeman - Can arrest people, request fine etc."* That "etc."
is doing a lot of work. I used the fine as the second way out of the Jail,
because the first — *"Law Book … necessary to get out of Jail"* — leaves a
player who is arrested before ever reaching the Bar Room with no way out at
all. The three exits from the Jail are now: the Law Book or an Examiner's
name (a proper defence), a 50-point fine, or standing trial undefended, which
ends in the Execution Chamber. Only the first is documented.

### 3.9 Sharing a valley

Sixteen players and a forty-line message ring are `PRTEST.SAI`'s. The
mechanism is not: it sprouted a SAIL process per TTY on one KI10, and there
are no TTYs here. Instead every copy of `amazon.exe` opens the same world
file and takes an exclusive lock for the length of a turn. Turns take seconds
and locks take microseconds, so the contention profile is about what it was.

---

## 4. Things I deliberately did not do

- I did not fill any gap with material from another game. Where AMAZON's own
  documents are silent, the replacement is either new writing derived from
  AMAZON's own world or an explicitly flagged inference.
- I did not "restore" the `Item 'Fred'` stat magnitudes from `AMAZON.TXT`
  into the `AMAZON.OFF` bit fields, because they do not fit and the two
  documents are two years apart.
- I did not invent a rooms 22+. `ROOMS` ends `22 []`, an empty stub. The
  valley has 21 rooms because that is how far Baltrunas got.
