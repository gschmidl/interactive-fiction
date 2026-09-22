# Dave's Dungeon (DAVESCAVE) - Windows console port

`davescave.exe` is Dave Parker's "Dungeons and Dragons", distributed by the
MITRE Corporation, version 1.0 of September 1980: a dungeon crawl behind a
magic elven gate in the Misty Mountains, played with two-letter commands
(`LI` lists them), with your character, your map and your stocked dungeon
kept between expeditions under your three initials. Run `play.bat`.

`src/convert.py` edits `davescave.for` into source gfortran will take - every
edit asserts how often it matches - and turns `TEXTFILE.DAD` into the file the
port reads. The port's run-time is `src/port/pdave.f`.

## Where it came from

The MITRE tape (`archive_original\tape6_traxSrc_3-29-86.tap`) is a SIMH image
of a plain Unix V7 **tar** archive - 10240-byte records, 3278 members - not a
DEC format. The game is in its `res/` directory, which was a VAX/VMS
directory (`.COM` files, `LOGIN.LNK`, `.EXE`s):

- `davescave.for` - the source, dated 28-Mar-1986 on the tape;
- `davescave.ftl` - a **VAX-11 FORTRAN IV-PLUS V1.3-22 compiler listing of
  DAVESCAVE.FOR.3, 9-Sep-1980**. Its source is the same as the .for, line for
  line, so its storage maps describe this program exactly;
- `textfile.dad` - the word table (directions, room kinds, monsters,
  treasures, magic items), 1018 bytes.

No compiled copy of the game is anywhere on the tape.

## What the listing and the run-time library settled

- **Default integers are 2 bytes.** The storage map gives every implicitly
  typed integer as I*2 (FORTRAN IV-PLUS, NOI4), so every unit gets
  `IMPLICIT INTEGER*2 (I-N)`.
- **RAN(I1,I2) is RANDU.** The listing shows it compiles to `FOR$IRAN`, and
  FORRTL.EXE from the VAX/VMS disk in this collection has it at transfer
  vector +400. Disassembled: the seed is I1 (high word) and I2 (low word) as
  one longword, multiplied by 65539 modulo 2**32, bit 31 cleared, converted
  with CVTLF (which rounds half away from zero) and scaled by 2**-31; the new
  seed goes back into the two words, and a zero I2 is a special case that
  bumps I1 and returns I1,3 unmultiplied. `pdave.f` does exactly that, and
  `tests\pran.py` checks it against a model of the instructions:
  24 of 24 draws.
- **TEXTFILE.DAD is PDP-11 FORTRAN segmented records**: 126-byte segments,
  each a control word (1 first, 0 middle, 2 last) and 124 bytes of data; nine
  of them make the 1000 bytes of `ITEXT(500)`. Both machines are little
  endian, so the two-character words need no swapping.
- **Tab-format lines were taken as typed.** Line 662 reaches column 73 once
  its tab is expanded, and the comma it ends with is needed; a FORMAT string
  (`...LYING AROU` / `ND IN THE HALLWAY?`) is continued across a line that
  ends at column 70 with no gap. So the port is compiled with long lines and
  `-fno-pad-source`.
- **Common blocks were never padded**: the map puts `TNAME` at offset 51 of
  `/DANDD3/`, so the port uses `-fno-align-commons`.
- **`/DANDD3/` is initialised in two routines** - `TNAME` in the main
  program, the three file names in `START` (which also appear, differently,
  in the main program). The VAX linker applied every module's data to the one
  overlaid psect, later modules over earlier ones, so the names that stuck
  are `START`'s: `DB3:ROOMFILE.DAD` and so on, with the player's initials
  written over `DAD`. gfortran keeps one routine's copy of a common block, so
  those values move into a single `BLOCK DATA`.

## Bugs in the original, kept

- **`SBATTL` declares `/DANDD2/` without `IUSED` and `MXITMS`**, so the
  `LDFLAG` it sets is the low byte of everyone else's `IUSED`. The VAX laid
  it out that way too, and so does the port.
- **`BREST` for `BRESP`**: "Do you want to modify your last game?" tests
  `BRESP.EQ.'Y'.OR.BREST.EQ.'y'`, and `BREST` is an undeclared REAL that is
  never set - so a lower-case y never worked there on the VAX. (The port
  folds input to upper case, so it is a Y by the time the program sees it.)
- `SFILLD` takes three arguments and `RESET` passes it five; the VAX ignored
  the extra two.
- The cure spell can leave you with fewer than no hits ("you have taken -1
  hits").

## What the port changes

Nothing about the game. The mechanical edits are listed at the top of
`convert.py`: tab format, the `IMPLICIT`, `NAME=`/`TYPE=` in the nine OPENs,
a byte compared with a character constant as its code, DEC's octal `"7`
(three bells) as 7, `IAND`/`IOR`/`IEOR` on mixed integer sizes made one
size, literals passed to subroutines given the 2-byte kind they had, and the
program's output moved from unit 5 (the VMS terminal, both ways) to unit 6.
Every record starts with a blank carriage control character, which the port
prints, as the collection's other source ports do.

**Input is folded to upper case** (added 2026-09-21 at the user's request).
The program knows its two-letter commands and its Y/N answers in upper case
only, as the upper-case terminals of the time sent them. So every READ from
the terminal now takes its line from the port's `PREAD`, which masks bit 8
off, drops NULs and folds lower case, and then reads its items from that
line with its own FORMAT. At end of piped input `PREAD` stops the game
quietly. `--no-fixes` is accepted and changes nothing: the folding stands in
for the terminal, it is not a fix.

Files: `TEXTFILE.DAD` lives beside the .exe; the player's `ROOMFILE`,
`CHARFILE` and `DATAFILE` go in `saves\` with the initials as the extension,
as the original put them in the player's directory on `DB3:`.

## Options

    davescave [--time HHMMSS] [--no-fixes] [-h]

The dice are seeded from `SECNDS` - at the start and again every expedition -
so `--time` holds the clock and makes a game reproducible.

## Checked so far (first pass, 2026-09-21)

- `tests\pran.py`: the generator matches the VAX's own code, 24 of 24.
- `tests\casefold.py`: a session typed in lower case prints exactly what it
  prints typed in upper case, and so does one with bit 8 set and NULs mixed
  in. 60 more random mixed-case sessions of 300 commands each ran clean.
- 60 fuzz runs of 300 random commands, with new and continued characters:
  no crash, no run-time error.
- Expeditions played: the gate, stairways, corridors, walls, an elbow, the
  cure spell, taking in the hallway, `HW`, `EX`, `QT` and death; new
  character, continue, and character files written and read back.

## Still to do (refine pass)

- **A reference run.** Neither the VAX/VMS disk in this collection (no
  FORTRAN compiler, only the run-time) nor anything else at hand can compile
  it; a PDP-11 RSX or TRAX system with F4P, or VMS with FORTRAN, would let a
  session be compared byte for byte.
- INTEGER*2 intermediates: FORTRAN IV-PLUS did integer arithmetic in 16 bits
  where gfortran widens a 2-byte value mixed with a literal to 4 bytes. The
  low 16 bits of `+ - *` agree either way; a division or comparison of an
  overflowed intermediate would not. No such case has been found; an audit
  would make sure.
- F_floating arithmetic rounds halves away from zero where IEEE rounds them
  to even; only `RAN`'s conversion is done the VAX way.
- Play a character up a few levels, down the stairs and through a battle
  with a dragon.
