# Skattejakt — Norsk Data ND-100, SINTRAN III — native Windows port

    skattejakt.exe

`skattejakt.exe` is an ND-100 minicomputer with just enough SINTRAN III in it
to run one program: `SKATTEJAKT:PROG` exactly as it lies on the eXo SINTRAN
pack (user GAMES).  Nothing is rewritten or patched.  Every message, parser decision and dwarf's knife is
the original NORD FORTRAN program running on an emulated CPU.

    * * N Y H E T * *
    *   E V E N T Y R   *
    N Å  P Å  N O R S K !!!
    ...
    Velkommen til Skattejakt!!  Ønsker du å se reglene?

## The game

Skattejakt ("treasure hunt") is Crowther and Woods' Adventure in Norwegian,
expanded to **500 points**.  The 350-point cave is there in translation —
"Du står ved enden av en vei foran et lite hus bygget i rød mursten", the
snake in Dovregubbens hall (the Hall of the Mountain King), the dwarves with
their knives and axes.  The game's word list also names things the 350-point
game never had: a princess, a throne, a cell door, parchment, a plank, rope
and a hook, an anchor, a sword, a harp, a candlestick, a sceptre, a silver
cup, runes and a suit of armour.  The whole vocabulary is in
`notes\vocabulary.txt`, decoded from the program's own word table.

Useful words: `HJELP` and `INFO` explain the rest; `N S Ø V OPP NED INN UT`,
`TA`, `SLIPP`, `TENN` (light the lamp), `INNHOLD` (inventory), `POENG`
(score — Woods' `SCORE`, and like his it then asks whether you want to quit),
`KORT` (brief), `ÅPENT` (opening hours), `SLUTT` (quit).  Words are
significant to six letters.

The cave is open all day, every day in this copy (`ÅPENT` says so), so there
are no cave hours to lift.

### Saving: `SPAR`, `UTSETT` or `PAUSE`

This is Woods' suspend, and the port keeps it.  The game asks whether a
90-minute wait is acceptable, then says *Husk å dumpe ut memory* — on
SINTRAN you now typed `@DUMP` to save the whole memory image as a program.
The port does that for you:

    Husk å dumpe ut memory............

    Save the suspended game as (file name, or Enter to discard it): hule
    Saved as hule.PROG.  Continue it later with:  skattejakt hule

`skattejakt hule` starts the dumped image, which is what the SINTRAN user did
with it.  The game itself checks the clock: under 30 minutes it says *Selv
trollmenn må vente lenger enn dette!* and stops; under 90 it asks whether you
are a wizard.  A saved file is a genuine one-bank `:PROG` file; it is not
deleted when you resume, just as a dump on SINTRAN was not.

(The wizard's magic word in this copy is `LURVEN`; after it the game wants
the answer to a challenge built from the magic number 11111 and the time of
day, as in Woods' original.)

## Keys

The game echoes and edits its own input, so the keys are the ND ones:

| key | effect |
| --- | --- |
| letters, digits | as typed; lower case is fine |
| Æ Ø Å æ ø å | the Norwegian letters (Ä and Ö count as Æ and Ø) |
| Backspace | deletes a character (the port sends the game Ctrl-A, the ND delete key) |
| Ctrl-Q | throws away the line typed so far |
| Enter | ends the line; an empty line just rings the bell |
| Esc | SINTRAN *user break*: the program stops (Ctrl-C does the same) |

Esc stopping the game is original.  The program tries to switch Esc off at
start-up with the SINTRAN command `DISABLE-ESCAPE-FUNCTION`, but in this copy
the text reads `  SABLE-ESCAPE-FUNCTION` — the first two letters are blanks —
and SINTRAN quietly ignores it.  On the real system Esc prints
`USER BREAK AT   53031B` and returns to SINTRAN's `@`; the port prints the
same line and ends.  Use `SPAR` if you want to come back.

## Options

| option | meaning |
| --- | --- |
| `SAVED-GAME` | resume a game saved after `SPAR` |
| `-s`, `--save-dir DIR` | where saved games are written and looked for (default: the current directory) |
| `-d`, `--data DIR` | where `SKATTEJAKT.PROG` is (default: `data\` next to the program) |
| `--ascii` | show the national characters as the 7-bit codes `[ \ ] { | }` |
| `--raw` | pass the terminal bytes through untranslated (the tests use this) |
| `-Z`, `--clock SECONDS` | fix the clock at SECONDS since 1970, for repeatable sessions |
| `--prog FILE` | run another one-bank `:PROG` file |
| `-v`, `--verbose` | log monitor calls on standard error |
| `-T`, `--trace` | trace every instruction on standard error |
| `-h`, `--help`, `--version` | |

`--help` also lists `--no-hold` and `--vdu`; they belong to the SVHA Adventure
port, which is built from the same source, and change nothing here.

Started from Explorer, the window stays open at the end with
`[press any key]`, so the final score does not vanish with it.

The game sees the date 28 years back, as SINTRAN did on the eXo set-up
(RetroCore does the same to stay clear of the year 2000); the weekday is the
same.

## How faithful is it

The reference is SINTRAN III L itself: the eXo RetroCore set-up booted in
the background, logged in as GAMES over its telnet terminal port, running
`SKAT`.  The same commands typed there and into the port give the **same
bytes**:

- a 52-command walk into the cave, through the dwarves to a knife in the
  back, 6,870 bytes identical;
- the editing keys — Ctrl-A, Ctrl-Q, DEL and Backspace (the game rings the
  bell for those two), empty and blank lines, over-long words, lower case;
- five runs of 300 random commands from the game's own vocabulary, three of
  them after walking into the cave: 151,016 bytes, all identical;
- Esc, and the first session typed by hand.

`python tests\run.py` replays all 15 sessions against the port.

The game's random numbers do not depend on the clock: the generator's state
was dumped with the program, so every new game unfolds the same way for the
same commands.  That is why the transcripts can be compared at all.

`tests\consoleplay.py` plays through a real (hidden) Windows console:
Norwegian letters in and out, Backspace, `SPAR` with the file prompt, resume,
and Esc.

## Building

    make

needs gcc (Strawberry Perl's works).  The emulator is `src\` — the same
source as the SVHA Adventure port, built with `GAME=0`; see `NOTES.md`
for how the machine and SINTRAN were worked out.

## A Norwegian SVHA Adventure?

The eXo SVHA Adventure pack carries Norsk Data's games guide
(`GAMES/INFO-SPILL:TEXT`, kept in the SVHA port's `src_original\`).  It says
the game exists only in English for now: there is a more limited version in
Norwegian, "but it can only run on machines with 48 bit floating point, so we
chose to release the English one."  Skattejakt needs exactly that — it
compares the words you type with its word table as 48-bit floating point
numbers — so it may be that version, though its 500 points are more than SVHA's
360.

## About the eXo entry

eXo's "Skatte Jagt (19xx)" has two versions, and they are different games.
The Nascom one is a Danish adventure, *Skatte Jagt* by Henrik K. Jensen, with
its own robot parser ("Jeg er en robot som du styrer...").  This port is the
ND-100 one, the Norwegian Adventure.
