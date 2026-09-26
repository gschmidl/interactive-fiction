# ADVENTURE4 - Arnautov's 660-point Adventure, Glaxo 4.3 (Prime, 1984) - Windows console port

`adventure4.exe` is Mike Arnautov's ADVENTURE4, "the complete 660 pt Glaxo
version 4.3, 26th Jul 1984", as the PULSE user group library tape has it.
The game runs under his A-code executive, rev.19.2 ("GGR version 4.3 - 23
Mar 84"). Run `run.bat`.

The game is not written in FORTRAN. It is A-code, Arnautov's own adventure
language, compiled into the four database files ADVINIT1-4.DAT. The
executive, EXECUTIVE.F77, is an interpreter for it. The A-code source was
never released ("only to those who can demonstrate their mastery of this
version"), so the port runs the compiled database, byte for byte as on the
tape, with the executive converted from its F77 source.

## How it is built

`build.sh` (or `build.bat`):

1. `src/convert.py` reads EXECUTIVE.F77 and EXECUTIVE.INS.F77 as PRIMOS
   wrote them (`..\src_original\ADVENTURE4`: top bit set, DC1 + count for a
   run of blanks). It writes gfortran source; every edit is counted.
2. That source is compiled with the port's PRIMOS run-time,
   `src/port/prime.f` and `primec.c`.
3. ADVINIT1-4.DAT are copied beside the program, and checked equal.

## What the source settled

**-Ints -Logs.** The executive was compiled `F77 -Ints -Logs -Big`:
INTEGER and LOGICAL take two bytes. The conversion makes both *2, and
`-fno-align-commons` lays the COMMON blocks out halfword for halfword as on
the Prime. The layout matters: the A-code reads past the ends of arrays.
Its first EVAL is object 500 of 200, which lands on PLACEBIT(98), and the
port reads there too. A build with bounds checking cannot even start the
game. A constant written with six digits (`001000`) is a long one on the
Prime, which keeps the record keys (class x 1000 x 1000 + line) right.

**The database.** OPENDB read the files with SRCH$$ and PRWF$$, the text
in pieces because it is bigger than a segment; the run-time's OPENDB reads
them whole. The four files:

| File | Contents |
|---|---|
| ADVINIT1 | the A-code, halfwords |
| ADVINIT2 | the text, every halfword negated, two characters to a halfword with the top bit set |
| ADVINIT3 | 4142 pairs of long words: a key and its position |
| ADVINIT4 | the 562 vocabulary words, 12 characters each, and their keys |

MOVE, a PMA routine, copied a text record into a CHARACTER variable; PMOVE
unpacks it. RS, LS and RT are Prime's shift intrinsics. RAND, the
executive's own function, is renamed KRAND so that gfortran's intrinsic does
not take its place.

**IOA$**, the PRIMOS routine every text goes out through, was measured on
PRIMOS 23.4 (`tests\prime\ioaprobe*`):
- It never ends the line itself; SAY calls TONL for that.
- `%$` ends the string. `%%` prints `%`, `%/` a new line, `%X` a blank, and
  `%Y` nothing.
- Any other character after `%`, and a `%` at the end, prints `??`. So
  Ralph the elf's "GET OUT!$!!!%!" (text 4386015) comes out as `GET
  OUT!$!!!??`.
- A word the player types reaches IOA$ through SAY's `#` substitution. There
  F77 hands IOA$ a third argument, the text's descriptor (`'5000`, 140). The
  first directive that wants an argument prints it:

  | Directive | Prints |
  |---|---|
  | `%D` | 2560 |
  | `%O`, `%W` | 5000 |
  | `%H` | A00 |
  | `%F` | 320 |
  | `%E` | 3E 02 |
  | `%L` | T |
  | `%P` | 5000(0)/214 |

  `%6D` right-justifies. `%A` and `%C` end the line. `%R` and `%Z` print `??`
  and swallow the next character. A second such directive ends the line.
  Sessions 4-6 check all of this against the machine.

**The glitches are the original's.** The executive complains when the
A-code does something wrong. Three kinds turn up in play, and all were
played on the Prime (sessions 7 and 8):
- WAKE and PROD with a place, a verb, or nothing give "Glitch! Bad WHERE"
  and "Bad BITVAL". The line reads `after loc ****`, because I4 has no room
  for the position.
- After the wizard at the gate turns you into a toad, the A-code asks for
  "Bad EXEC code: -100", and the game ends.

WHERE's error path returns without setting WHERE. On the Prime the caller
got whatever the I/O library left in the A register; in gfortran it would
be the last value any entry point returned. The port returns 0, which
answers WAKE HOUSE as the Prime did.

## RND - measured, not yet reproduced

The executive seeds the dice once, with `RND(I)`: I is 501, left over from
a loop. After that it only calls `RND(0)`. RND is PRIMOS's RND$, reached
through a dynamic link. The tape's EXECUTIVE.SEG therefore uses the running
system's routine, and it was measured on PRIMOS 23.4 (`tests\prime\rndprobe*`):
- A seed is all 32 bits at the argument's address, so it includes the
  halfword after the 2-byte I.
- `RND(x)` for any x other than 0 returns x itself.
- The numbers are 23-bit fractions, unnormalised floats with exponent 128.
- The same seed always gives the same series. **Every game on the Prime
  rolled the same dice.**

The generator itself matched no LCG or shift register tried, so the port's
RND is a stand-in with the same interface. The port is deterministic too:
every game is the same unless `--seed N` is given.

## Options

    adventure4 [-u] [--seed N] [--echo] [-h]

- `-u`, `--unlimited`: after SAVE, a restore within 30 minutes counts as
  one after a long wait. Without it: "I'm sorry - only a wizard can restart
  a game in less than 30 minutes" (EXEC 9).
- `--seed N`: other dice.
- `--echo`: writes each line read back, as the Prime's terminal echoed it.
  It is for comparing transcripts.

Saved games go to `saves\A_<login name>__<name>`, as PRIMOS named them in
the player's directory. The game ends after SAVE; RESTORE asks whether to
keep the image.

## What the port changes

- Terminal units 1 -> 5 and 6.
- At the end of piped input the program stops, where the original read its
  last line again.
- Input is masked to 7 bits and NULs are dropped.
- RECL= on the save file was in halfwords on the Prime; it is dropped.
- The PRWF$$ call that touched a restored file's date is dropped.
- `%V` in a typed word printed 2560 characters of the executive's own
  memory on the Prime (`tests\prime\ioa_percent_v.txt`). Here it ends the
  line.

## Kept as they are

- READIN's comma handling. The text after a comma is moved behind the line's
  first blank, so "get lamp, get keys" is "get get keys" ("Don't be
  ridiculous!"), and after "in," the rest is lost.
- WIZARD is a trap: whatever you say, it costs 10 points ("Oh, pooh"). The
  modern A-code shows the same. The real wizard flag is set some other way.
- The texts' own spelling ("posessions").

## Checked so far (first pass, 2026-09-21)

- **`tests\cmpprime.py`: eight sessions, 649 lines, identical line for line
  with the original.** The original is the tape's EXECUTIVE.SEG and ADVINIT
  files, restored from `pulse_library.tap` onto a scratch copy of the p50em
  pack with MAGRST (logical tape 3) and run on PRIMOS 23.4. The sessions
  cover:
  - the opening and the instructions;
  - the building and the grate, and into the cave to the bird and the top
    of the small pit;
  - BRIEF, the score, and QUIT;
  - the parser: unknown words, capitals, commas, empty lines, questions;
  - IOA$'s `%` sequences;
  - the WAKE/PROD glitches and the toad at the gate.

  The sessions keep off the dice. `tests\prime\primeplay.py` plays a new
  one.
- `tests\regress.py` checks:
  - the eight sessions;
  - the options, and a missing data file;
  - SAVE, RESTORE within 30 minutes, with `-u`, keeping or deleting the
    image, and a name never saved;
  - that the same seed gives the same game;
  - a 200-character line, the comma, and the end of input inside a
    question.
- `tests\fuzz.py`: 300 runs of 400 commands built from the game's own
  vocabulary, about 44,000 prompts read. No crash, no hang, nothing on
  stderr. The glitches seen are those above.
- `tests\consoleplay.py`: a game at a real (pseudo) console.
  - "Would you like instructions?" is answered on the next line.
  - `? ` keeps the cursor after it, and a command shows once.
  - QUIT, Y ends the program with exit code 0.

## Still to do (refine pass)

- **RND$** stays a stand-in (user, 2026-09-21): the game plays with the same
  odds; only deep-cave sessions cannot be compared with the Prime. If it is
  ever wanted, reproducing PRIMOS's generator has two ways in:
  - disassemble its V-mode code (dumped from segment 0603, word AF44, in
    `tests\prime\rnddump.log.txt`);
  - trace it in p50em.

  Then find the halfword that follows I in WEBSTER, which completes the
  executive's seed.
- **Whether 1984's RND was rev 23's.** The tape's EXECUTIVE.SEG links it
  dynamically, so on the Glaxo machine it was that system's.
- **A debug path to prove the game winnable.** The admin/wizard flag is set
  somewhere in the A-code. Decompile ADVINIT1 with the modern A-code
  (`..\reference\ARNA0660`) as a guide.
- **Play to 660 points.**
