# ABENTEUER - Windows console port

`abenteuer.exe` is ABENTEUER, the German Colossal Cave of a Harris VULCAN
site: Gary M. Palter's portable Adventure (MIT) as Harris Computer Systems
Division adapted it to the Harris ("HCSD VERSION 01.112777 DICK
REYNOLDS"), its messages translated into German. It is the original
FORTRAN program, compiled from the job stream `J.ADV` on the site's FAST
save, and it goes on from the site's own `*NEUSPIEL`, the game as players
got it. Run `run.bat`.

The cave keeps the site's hours. On weekdays from 8:00 to 18:00 only
wizards may play; the program shows the opening hours and asks. A saved
game must also rest 90 minutes before it goes on. `run.bat -u` lifts both.

## Playing

German commands of one or two words; only the first five letters count.
Lower case is fine.

- **Moving:** NORD SUED OST WEST (N S O W), NO SO SW NW, HOCH RUNTER
  (H R), REIN RAUS, ZURUECK; XYZZY and PLUGH.
- **Actions:** NIMM, LEG, BESTAND (inventory), SCHAU, LIES, OEFFNE,
  SCHLIESSE, LICHT AN, LICHT AUS, ISS, TRINK, WIRF, TOETE, FUETTERE.
- **Game:** PUNKTE (score), AUFGABE (quit), HILFE, INFO, ZEITEN (the
  hours), KURZ.

`SICHR name` saves the game in `saves\NAME.SAV` and ends it. `BRING name`,
typed as the first command of a game, goes on with it. A name that isn't
found gives a new game, as on the Harris. The game starts where the site's
NEUSPIEL left it: at the road, turn 1, with the question about
instructions already answered.

**Wizards:**
1. Type `MAGIE MODUS` as the first command, answer JA, and give the magic
   word DWARF.
2. Answer NEIN to "DO YOU KNOW WHAT I THOUGHT IT WAS?". The program then
   shows ten octal digits.
3. Reply with the magic word exclusive-or those 30 bits. Take the
   magic word as CODE1 codes, six bits a character: a character's code is
   its ASCII code minus 32. Type the result back as characters.
   `wizard_reply` in `tests\regress.py` does the sum.

A wizard can change the hours, the holidays, the short game, the magic
word, the restart latency and the message of the day. The maintenance
saves a new NEUSPIEL.DAT, just as the wizard's NEUSPIEL became the site's
`*NEUSPIEL`. `build.sh` makes the site's again.

## How it is built

`build.sh` (or `build.bat`):

1. `src/convert.py` turns the FORTRAN part of `J.ADV` (lines 5-3538; the
   rest is Harris assembler) into gfortran source. Every edit asserts how
   often it matches, and its docstring lists what the Harris FORTRAN needs:
   INTEGER*6 is INTEGER*8, `nD` constants, MAX2/MIN2/MOD2, `.AND.`-style
   bit operators on integers, `'nnn` octal constants, characters as codes
   one to a word, carriage control dropped from the 38 output FORMATs, and
   the renames AND/OR/XOR/RAN to KAND/KOR/KXOR/KRAN.
2. The port's Harris run-time is `src/port/pharris.f` and `pharrisc.c`:
   SHIFT, LOWSIX, ABORT, ADDR (PADDR) and SIZE (SIZEOF), JDATE, IRANP and
   IRAN, and the site routines IOINIT, LDCOMN and SVCOMN.
3. `ADV.DATA` is the section files TAPE1 ... TAPE1012 from the same save,
   in section order. `src/edition1980.py` writes `ADV1980.DATA`, the same
   with the five later edits undone (below).
4. `src/neuspiel.py` cuts NEUSPIEL out of `archive_original\fast.tap` and
   writes `NEUSPIEL.DAT` in this port's layout.

A bounds-checked build goes to `.build\checked` for `tests\fuzz.py`.

## What the tape settled

**The FAST save.** A file is a 25-word header followed by 2688-byte
segments. The header holds the name in 6-bit code in words 0-1 and the
size in 112-word sectors in word 14. Each segment is followed by 14 bytes
that carry the sum of its words. Text files are 672-byte blocks of numbered
lines (see `..\..\..\_work\_Harris_VULCAN_work\tools\harris_text.py`).

**NEUSPIEL** holds the eleven stretches of COMMON the main program lists
(CMADRS, CMSZES), each from a sector boundary, in its declared layout
without padding:
- An INTEGER*6 is two 24-bit words, value hi × 2²³ + the low word's low
  23 bits. In 1149 double words the low word's top bit is set, and it is
  not part of the value.
- An INTEGER*3 or a LOGICAL is one word. A LOGICAL is true when its sign
  bit is set: `.TRUE.` is all ones, and WZDARK, which the program sets to
  DARK(0) every turn, holds 000001 on the lit road.
- The Harris's terminal units TTYI=0 and TTYO=3 are saved with the game;
  the port puts in its own, 5 and 6.

The site did not make NEUSPIEL with a wizard's maintenance. It is a game
saved with `SICHR MEIN` as its first command, on Friday 13 June 1980 at
14:34, then renamed. Its SETUP is −1, so the program goes on from it
(label 8305) instead of reading ADV.DATA.

**The section files on the tape are a later edition** than the one
NEUSPIEL was set up from. Five edits:
- **TAPE1:** "FEE FIE FOE FOO" [SIC]. and [WITTS ... GESELLSCHAFT]" became
  `\SIC!.` and `\WITTS ...!"`. The same file's real exclamation marks
  stayed, so this is an edit, not a character code.
- **TAPE4:** gained a line `0`, a blank vocabulary entry 0.
- **TAPE5:** every `!` became `[`.
- **TAPE6:** "... GENUG.", MEINT ER. gained its comma.

NEUSPIEL is what players got. `--fresh` sets the game up from the files as
the tape has them, garbled punctuation and all; `--fresh=1980` sets it up
from `ADV1980.DATA`, the files with those five edits undone - the 1980
edition, from which the port makes the site's NEUSPIEL exactly.

**The clock.** JDATE gives the year × 4096 + the day of the year, and the
time in tenths of seconds. DATIME divides that by 600 to get minutes. Its
day count puts Saturday at D mod 7 = 0, which makes Saturday and Sunday the
weekend (WKEND).

**The dice.** RAN seeds the Harris library's generator (IRANP) with the
minute of the day at a game's first draw, then draws with IRAN. That
generator is not on the tape, so the port has a stand-in (Lehmer's minimal
standard). `--seed N` fixes the dice.

## Bugs in the original, kept

- **The rank messages.** The translation gives KLASSE C, B and A all 300
  points, and lines with the same number make one message. So a score of
  251 to 300 is told all three at once, and 301 to 349 is already
  ALTMEISTER.
- **Two out-of-range subscripts** land in COMMON /PLACOM/, reading its
  neighbouring words just as they did on the Harris:
  - `IF(ATLOC(LOC).EQ.0.OR.LINK(ATLOC(LOC)).NE.0)` does not stop at the
    .OR., so it reads LINK(0), which is ATLOC(150);
  - in a game set up from ADV.DATA, LOC is still −1, the number that ends
    the last section read, so the first move's FORCED(LOC) reads COND(−1),
    which is FIXED(99), 0.
- **SICHR without a name** ends the game unsaved: "TUT MIR LEID, ABER
  DEINEN FILE KANN ICH WEDER GENERIEREN NOCH FINDEN."

## What the port changes

- **Fix 1.** BRING and MAGIE MODUS are only taken when TURNS is 0, and the
  site's NEUSPIEL is at turn 1, so on the site nobody could go on with a
  saved game. Here they are taken at a game's first command.
  `--no-fixes` gives the site's game as it was.
- **The terminal** is gfortran's units 5 and 6. Before IOINIT, when
  "INITIALIZING..." is written, TTYO is already 6; on the Harris it was
  0, which also meant the terminal. Input is masked to 7 bits, NULs are
  dropped, and the end of piped input ends the program (exit 0).
- **Files.** `ADV.DATA`, `ADV1980.DATA` and `NEUSPIEL.DAT` sit beside the
  program. A
  player's game is `saves\NAME.SAV`, named by up to eight characters, and
  must be exactly as long as the eleven stretches.

## Options

    abenteuer [-u] [--seed N] [--fresh[=1980]] [--no-fixes]
              [--date YYYY-MM-DD] [--time HH:MM] [-h]

- `-u`: no prime time, and no wait before a saved game goes on.
- `--fresh`: set up from ADV.DATA, as when there was no NEUSPIEL. The
  wizard question and the instructions come first.
- `--fresh=1980`: the same from ADV1980.DATA, the 1980 edition of the
  texts (`--fresh=tape` is plain `--fresh`).
- `--date` and `--time`: hold the clock.

## Checked (first pass, 2026-09-21)

- **`tests\crosscheck.py`:** the port repeats the site's own steps in a
  scratch copy: `--fresh=1980`, NEIN, NEIN, `SICHR MEIN`, with the clock at
  13 June 1980 14:34. Its save equals NEUSPIEL.DAT in all 13,552
  variables (ADV1980.DATA, the five later edits undone). That covers every word
  of text, the vocabulary, the travel table, the objects, the loop
  variables after one command, and SAVED=1441 and SAVET=874 from the date
  arithmetic. On the tape's own edition (`--fresh`), 503 differ, all from
  those edits.
- **`tests\regress.py`:** 30 checks, followed by the cross-check and the
  win. It covers:
  - the opening, lower case, 7-bit input, the score and quitting;
  - the hours: a weekday, 18:00, a Saturday, `-u`;
  - SICHR, and BRING after 10, 40 and 91 minutes and with `-u`;
  - BRING of a missing game, and `--no-fixes`;
  - an impostor, and a wizard's whole maintenance, whose new NEUSPIEL
    then starts a game anew;
  - `--fresh` and `--fresh=1980`, and refused options.
- **`tests\win.py`:** 350 of 350 points and ALTMEISTER. Through the save
  format, the test sets up the state the middle game leaves: treasures in
  the building, the magazine at Witt's End, the lamp lit, one turn left
  on CLOCK1. The program itself then closes the cave and sets up the
  repository, and SPRENG from the SW end with the marked rod at the NE
  end ends the game.
- **`tests\fuzz.py 200`:** 200 games of 400 random commands from the
  game's own vocabulary. None ended with a run-time error, BUG or a hang.
  The bounds-checked build played 100 of them through and stopped in the
  others only at the two subscripts above.
- **`tests\consoleplay.py`:** a real console (ConPTY). It checks the
  prompt line, that the console echoes a command once, and that the game
  ends by itself.

## Still to do (refine pass)

- **The Harris's IRAN** stays a stand-in (user, 2026-09-21). The FORTRAN
  library manual or the routine itself (on another Harris tape) would
  settle it; there is no Harris VULCAN emulator for a reference run, so
  NEUSPIEL is the strongest check there is.
- **The 1980 session log** (`src_original\english_ADVENTUR_session_1980.txt`,
  the English program from the same site) could be compared with a German
  game for the rules: blank lines, dwarves, the lamp.
- Done 2026-09-21: the 1980 edition is offered as `--fresh=1980`.
