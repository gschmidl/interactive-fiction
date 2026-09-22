# Mystery Mansion (HP 1000 RTE) - Windows console port

`mmm.exe` is Bill Wolpert's Mystery Mansion in its HP 1000 version, for
RTE-IVB/6VM: "MYSTERY 2. REVISION 16 - 23 JUL 81", the FTN4 source
`&MMM` from the INTEREX CSL/1000 startup tape (27 Aug 1986), compiled with
gfortran and a small RTE run-time. Run `play.bat`.

A murder has been done in the mansion - the scene, the murderer and the
weapon are drawn from the clock - and the day runs out: night falls at turn
300, the mansion goes up at 450, and at 550 the game gives up on you. Real
time counts too: every 30 seconds spent thinking is a turn (hold the clock
with `--time` and it does not). Answer questions with YES or NO in full.

## How it is built

`build.sh` (or `build.bat`):

1. `src/convert.py` (with `convert2.py`) reads `&MMM` straight off the tape
   image - `archive_original\CSL-1000_Startup-Tape.zip`, through
   `src/hptape.py`, which knows the TF tape's 10240-byte records, its tar-like
   file headers and RTE's FMP records - and writes `.build/src/mmmroot.f`
   (the main program MMM, the BLOCK DATA, MMRI, MMRL) and `mmmseg.f` (the
   twelve segments MMSA-MMSL). Every edit asserts how often it matches, and
   the pass counts are checked (`convert: ... write 906, exec8 499 ...`).
2. Both are compiled with the port's run-time, `src/port/pmmm.f` and
   `src/port/pmmmc.c`. The main program keeps its variables
   (`-fno-automatic`); the segments get fresh, zeroed ones on every call
   (`-finit-local-zero`), because RTE read a segment from the disc each time
   EXEC 8 asked for it.

The copies of the tape's files in `..\src_original` (with MANIFEST.txt) are
for reading; the build does not use them.

## What the source settled

**The dialect.** `convert.py`'s docstring lists it: 16-bit INTEGER, two
characters to a word (Hollerith constants are packed first character first in
memory, which is what gfortran's `A2` reads and writes, and the few
statements that take a word apart with `/256` call `CFIRST`/`CSECND`/`CJOIN`/
`CONE` instead), octal `nB`, `3"..."` in a FORMAT, the two-way arithmetic IF,
`END$`, a BLOCK DATA named like its COMMON block, word tables filled through
EQUIVALENCEd COMPLEX arrays. Columns 73-80 are the sequence field: 22 lines
have a comma or a period there that FTN4 never read - the relocatable `%MMM`
on the same tape confirms it, since it has the literals meeting as `""`,
which HP's formatter took as the end of one and the start of the next.

**RTE.** EXEC 8 loads a segment and never returns; here it is a jump back to
a dispatcher in `pmmmc.c` that calls the next segment with its five RMPAR
parameters. EXEC 11 is the clock (10 ms, seconds, minutes, hours, day of the
year). OPEN/CREAT/READF/WRITF/POSNT/CLOSE are FMP: a file is a sequence of
word records, kept as `saves\CR<cartridge>\<name>` (`RTEFMP type count`, then
the records). A new type 1 file reads as zeros, as its space was allocated.

**The terminal.** Output goes through a filter that does what RTE's terminal
driver and an HP 264x did: a record ending in `_` is not followed by CR LF,
NUL and DEL are ignored, `ESC A/B/C/D/H/h/J/K` become ANSI, `ESC @` waits a
second, `&d` enhancements become ANSI attributes, `&f` softkey definitions
(RECORD KEY) are swallowed. The game uses these: after the security code it
moves the cursor up and writes blanks over the line, so the code does not
stay on the screen. Input is folded to capitals, as the terminals had it.

**The logical units.** LU 1 is the player's terminal (in an RTE session the
terminal is LU 1) and LU 0 RTE's bit bucket. Every other LU - the line
printer 6, a second terminal, a cassette - is a file `saves\LU<n>.txt` that
the game adds to. So `RECORD` then `6` (or `RECORD ON PRINTER`) leaves a
transcript of the rest of the game in `saves\LU6.txt`; `RECORD ON CASSETTE`
writes the commands typed to a tape LU's file, and `RECORD FROM CASSETTE`
plays such a file back. At its end the port strikes the break key, which is
how a player stopped a tape: the game shows `>**` and reads the keyboard
again.

## Options

    mmm [--site] [-u] [--code N] [--time HH:MM[:SS[.mmm]]]
        [--date YYYY-MM-DD] [--no-fixes] [-h]

`--site` runs the game as `RU,MMM,1,,,MM` did on Wolpert's own machine, with
his cartridge MM mounted (`saves\CRMM`). The game then checks that the
cartridge is there (file MMMC), keeps playing hours - 7:00-7:30, 11:30-12:00
and 16:00-17:30 - and asks anyone else for a security code; asks the
player's name and shows any message kept for that name in MMTLAN; counts
its runs in MMCF (on the default cartridge, `saves\CR0`); and at the end
logs the game in MMTLDA and asks for comments. An empty command is answered "THINK FASTER. I'M WAITING FOR AN
INPUT." and, the fourth time, "GAME ABORTED!". MMMC and MMTLAN were never
distributed; the port starts them empty.

`-u` passes security code 15815, as a player allowed to play at any hour
would have (`--code N` passes another; 27740, 24500 and 31313 are also
valid). Without `--site` there are no hours and it changes nothing.

`--time` and `--date` hold the clock: the mystery and every roll of the dice
come from it, so a game repeats.

## Debug mode

Wolpert's own: `DISPLAY` asks "nn ARE YOU THE CREATOR?", and the answer is
`YES` and 99 - nn (`52` -> `YES 47`). The menu shows and changes the items,
the rooms, the exits, the inhabitants, the test data (`13`: ROOM is the
player's room, MROOM/MWHO/MWEP the mystery), and shows cartridge MM's player
log and comments (`14`, `--site` only). Setting test datum 30 to 0 skips the
question afterwards.

## Bugs in the original

Fixed (`--no-fixes` restores it):

1. `RECORD` then the player's own LU (`1`): MMSE, MMSG, MMSI and MMSK write
   every message again on the recording LU and loop while it is the
   terminal's, so the game printed the same message for ever. MMSD tests for
   this case; the other four now do too.

Kept:

- `X AND Y` after GET or DROP: MMSB loads afresh for the second object and
  takes the word count from `INUM`, a local that holds whatever the disc
  image held (0 here, and it works). A command that begins with AND makes
  it shift words into `IWRD(1-6,0)`, below the word table, and after that a
  lone GET or DROP can make the count -1 and the next shift writes 18 words
  below it. On the HP that was memory below the COMMON block; the port puts
  an 18-word pad there.
- Other out-of-range subscripts land inside COMMON (IRES(93), IROM(0),
  IVRB(1-4,90) in the creator's verb list); the port has the same layout, so
  they read what the HP read, except that a word of text has its two
  characters the other way round.
- Answers to a number question that are not numbers leave the number as it
  was, and the game asks again (the program counts on it: SUSPEND sets -1
  and loops while it is negative).

## What the port changes

- Dropping articles and adjectives (MMSB, DO 176): `I=I-1` stands inside
  the DO 175 loops, so the original stepped `I` back six times for every
  word it moved up, and the DO 176 loop wandered up through the memory
  below the word table before it came back to the words. gfortran does
  not let a DO variable change; here `I` steps back once, to look at the
  word that moved into its place, and the words come out the same.
- Cartridge MM's MMMC and MMTLAN (never distributed) start empty; the other
  LUs are files (above); at the end of a cassette file the break key is
  struck; at the end of piped input the program stops, where the game would
  wait for ever.
- Two lines the FTN4 compiler let through: a continuation card with `/` in
  column 1 (read as blank) and a period after the last literal of FORMAT
  5889 (the relocatable keeps it, so HP's formatter passed over it).

## Checked so far (first pass, 2026-09-21)

- `tests\regress.py`: the mystery solved through the debug mode (weapon in
  the booty, murderer in the murder room, one step in: CONGRATULATIONS,
  +200 points); SUSPEND on cartridge 5 and RESTORE in another run; `--site`
  refusing a player after hours without a code, and with `-u` taking the
  name, the comments and the log to cartridge MM, which the creator's
  display then shows; RECORD on the terminal (fix 1), on the printer, on and
  from a cassette.
- `tests\consoleplay.py`: a `--site` game at a real (pseudo) console: the
  security code is blanked out, the name and the commands are typed on the
  line of their question, no record ends in `_`, and the program ends with
  exit code 0.
- `tests\fuzz.py`: 5700 games of 300-500 random commands from the game's
  own word tables (30% of them `--site -u`) with no crash, no hang and nothing
  on stderr. It found the loop of fix 1 and the writes below the word table.
- The session of the HP 3000 version's transcript (revision 16 too,
  `..\..\Mystery Mansion (HP 3000, MPE)\doc`) played here: N, LOOK and QUIT
  agree (47 points, 38 to the next level); the RTE text differs in places
  (INVENTORY is "A WORD IS TOO LONG" here, "I DON'T KNOW ANY WORDS LONGER
  THAN EIGHT CHARACTERS." there; the line breaks of the gate's description)
  and only takes YES or NO.

## Still to do (refine pass)

- A reference run of `%MMM` (the relocatable on the same tape) under SIMH's
  HP 1000 with the RTE-6/VM system of the Adventure ports: the mystery for a
  given time (the port uses IEEE REAL, not HP's floating point, so RND runs
  differently from the same clock), the value of MMSB's `INUM` in the
  segment's disc image, what lies below `/MMBC/`, and what FTN4's free-field
  READ did with letters.
- `--site`: a terminal read time-out like the one that fed the "THINK
  FASTER" messages on Wolpert's machine (its length is not in the program).
- Prove the 999 points reachable, not only the mystery solvable.
- The compiled copy of the game on `f1_dskup_f2_rte2250sys.tap.gz`, another
  revision (see the README one folder up): extract it and compare.
