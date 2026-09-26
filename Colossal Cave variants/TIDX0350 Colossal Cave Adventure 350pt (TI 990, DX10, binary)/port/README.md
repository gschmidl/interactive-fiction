# Adventure "thru a Cave" (TI 990, DX10 GAMES library)

This is the 350-point Colossal Cave Adventure from a TI 990 DX10 games tape: `ADVENTURE  Wandering adventure thru a Cave`
in the GAMES menu. It is Woods' version in TI 990 FORTRAN, in mixed case, with his wizard machinery:
- opening hours;
- a short game in the off hours;
- "magic mode";
- a suspended game that must wait 90 minutes before it can be resumed.

Its opening says "I look at only the first six letters". The tape holds no source for it, only the linked program
image.

**Port = the original program on an emulated TI 990/10.**
- The program is ADVEN's procedure and task segments, cut out of `.GAMES.PROG`; see `..\src_original\README.md`.
- `src/cpu990.c` runs it in user mode.
- `src/dx10.c` answers the DX10 3.7 supervisor calls it makes, and supplies the SCI synonyms its FORTRAN run-time looks
  up.
- `src/adven.c` does what the `ADVENTUR` command procedure did around the task.

The texts, the dwarves, the random numbers, the save file and the run-time's messages are therefore the original's own.

## Build and play

    build.bat            (MinGW-w64 gcc + sh; or ./build.sh anywhere with a C compiler)
    run.bat              (runs adventure.exe -u in the saves folder)

The game starts with the procedure's question, as it did at the terminal:

    Adventure thru a Cave
     Will/did you save your game?: NO

- **Enter or NO** plays without a save file.
- **YES** asks for `SAVE/RESTORE PATHNAME`. A new name is created. An existing file needs `Y` at "The file exists, do you
  want to use it? (Y/N)".
- In the game, **SAVE** suspends it into that file and ends the run. **RESTORE**, typed after the instructions
  question, brings it back. Without `-u` that works only after 90 minutes; earlier it gets "This adventure was
  suspended a mere N minutes ago. Even wizards have to wait longer than that!"

Options (GNU style; anything else is refused with exit status 2):

| Option | Effect |
|---|---|
| `-u`, `--unlimited` | The cave never closes and a suspended game resumes at once. It clears the program's own settings (from CAVE record 27) each time the program reads the clock: the weekday, weekend and holiday prime-time windows and the 90-minute wait. Without it the program keeps its hours: weekdays 8-12 and 13-17 are prime time, and only wizards or a short visit are allowed. |
| `--no-fixes` | The original exactly (see Fixes). |
| `--clock=TIME` | A clock that starts at `YYYY-MM-DD HH:MM:SS` (or `HH:MM:SS` today) and advances one second per reading. The random numbers are seeded from the clock, so this is what makes a session repeatable; the tests use it. |
| `--trace` | Each DX10 supervisor call on stderr. |
| `-h`, `--help` | Usage. |

## How the original ran, and what the port copies

- SCI's `ADVENTUR` procedure sets `UNIT1 = UNIT2 = ME`, `UNIT3 = @GAMES.FILES.CAVE` and `UNIT5` (the save file, or
  DUMY). It bids task ADVEN in the foreground and runs `SDT` when the task ends.
- `CAVE$` creates the save file as a sequential file of 80-byte records.
- The FORTRAN run-time, linked into the task:
  - gets its station and foreground status from Get Parameters (>17);
  - reads the station's record of `.S$FGTCA` on LUNO >01 to resolve the UNITn synonyms;
  - assigns LUNOs (>91), opens and rewinds units 1 and 2;
  - reads CAVE's scrambled records by number (texts on demand, settings in record 27);
  - writes its own messages (`STOP 1`, `NORMAL PROGRAM COMPLETION`) on LUNO >00 with blank adjustment.
- The port gives the task all of that. The calls it makes are the I/O SVC with subopcodes >91 >93 >00 >01 >02 >09 >0A
  >0B >0C >0E, plus >17, >03 and >0A (binary to decimal), and >04 at the end. Anything else stops the run with a
  message.
- The terminal is the reference system's console, a teleprinter.
  - Every record goes out as CR LF plus the text; FORTRAN's `0` carriage control adds a second LF.
  - The prompt `[=]  ` is the program's own.
  - Input is upper-cased, as the reference terminals (sim990 `/u`) send it. The program folds it to lower case itself.
  - Closing unit 2 with EOF feeds the paper, which puts the four blank lines before NORMAL PROGRAM COMPLETION.
  - When stdin is not a terminal, the port echoes each input line, so a transcript reads like the console.
- RANDOM (in the run-time) seeds itself from the minute and second of the first clock reading. It reads again while
  the minute is 0.

## Verification

The reference is Dave Pitts' sim990 3.3.0 with DX10 3.7, the GAMES tape restored with `RD`, and `ADVENTUR` run on the
system console (`..\..\..\_work\_TI990_work`; tools there: `ref990.py` records a session, `spi990.py` dumps a module).

- **The program images:** equal to DX10's own SPI listings, word for word.
- **Three recorded sessions** (`tests/reference/`) replay identically with `tests/crossref.py`, up to NORMAL PROGRAM
  COMPLETION:
  - ref1: 50 moves into the cave with two dwarves throwing knives, a death and the reincarnation question;
  - ref2: the save file, SAVE;
  - ref3: an existing save file.
- Each session is replayed at the clock second that seeds RANDOM as it was seeded there
  (`tests/reference/clocks.txt`, found by `crossref.py --find`). The one line left out is SCI's initial value in the
  pathname prompt, which remembers the last pathname of the same logon.

`tests/regress.py` runs these plus checks of:
- the options;
- SAVE and RESTORE, too early and in time;
- `-u`;
- the hours;
- the end of the input;
- the minute-0 seed.

`tests/fuzz.py` plays random commands. 1,000 games of 500 commands gave no fault, no unsupported call and no run-time
error.

`tests/consoleplay.py` runs the port in a real (pseudo) console, where the console does the echo and the line editing:
each answer stays on its prompt's line and the reply starts on the next, as on the reference console; Backspace (the key
sends DEL) edits; small letters are taken; QUIT ends with NORMAL PROGRAM COMPLETION; Ctrl+C ends the game quietly (exit
0); `run.bat` makes `saves\` and SAVE writes there.

**Random sessions on the reference** (`tests/reffuzz.py`, with the reference system running): each walks into the cave,
types random commands and quits, is recorded on the reference console, and joins `crossref.py`'s set once the clock
search finds the start second that replays it. Any session the port did not replay would be an error in the port, since
both run the same program. The five recorded on 2026-09-22 (fuzz1-5, 23 to 59 lines) all replay identically, so
`crossref.py` now checks eight sessions; see `tests/reference/README.md` for the list.

**The original's calendar.** On Saturday 2026-09-19 (DX10's own date, as its SDT line shows) the program says the cave
is closed, though its hours read "Sat - Sun: Open all day": it counts that Saturday as a weekday. The port does the same
(session fuzz1 replays), so this is the program's own date arithmetic, not the port's.

**MAINT (magic mode).** The magic word from CAVE record 27 is accepted: `DWARF` at "Prove it!  Say the magic word!".
"That is not what I thought it was.  Do you know what I thought it was?" is then Woods' trick question, not a refusal
(answer NO). The program prints a six-letter challenge and wants a reply computed from it, the magic number and the
clock. Woods' formula, adapted to six letters, is refused in 42 variants (time as HHMM or minutes, magic number 11111,
five- or six-letter wrap), and the TI program does not use his 1027 generator. So the TI version's formula differs; it
was not decoded.

## Fixes (on by default; `--no-fixes` turns them off)

1. **The first random number at minute 0.** RANDOM loops on the clock until the minute is not 0. On the original, the
   first random event in the first minute of any hour froze the game until the minute changed, up to a minute. With
   fixes on, that reading gives minute 60 instead, so there is no wait and the seed is still one no other minute gives.

## Differences from the original setup

- SCI offered the last pathname of the same logon as the initial value of `SAVE/RESTORE PATHNAME`. The port has no
  logon, so the prompt starts empty.
- The save file is a host file of length-prefixed records, not a DX10 sequential file.
- Changes the wizard's MAINT makes to CAVE (hours, message of the day) last for the run only. The data file itself is
  built into the program.
- The original shows its run-time error messages (e.g. END OF FILE on the terminal) and stops. The port ends quietly
  when its input runs out.

## Files

    build.sh / build.bat   build adventure.exe
    run.bat                play (with -u) in saves\
    src/cpu990.c/.h        the TI 990/10, user mode (instruction semantics as sim990's)
    src/dx10.c/.h          the DX10 supervisor calls, SCI's synonyms, the terminal
    src/adven.c            the ADVENTUR / CAVE$ procedures, options, main
    src/images.c/.h        the program and data (generated by src/mkimages.py from ..\src_original)
    tests/                 crossref.py, regress.py, fuzz.py, consoleplay.py, reffuzz.py, common.py;
                           reference/ holds the recorded sessions

## Still to do (refine pass)

- Nothing. Decided not to do (user, 2026-09-22): decoding MAINT's six-letter reply formula, so magic mode stays
  closed (see above); emulating the 911 VDT, the menu's usual terminal - the console teleprinter was chosen because
  it can be recorded exactly, and the game plays the same on it.
