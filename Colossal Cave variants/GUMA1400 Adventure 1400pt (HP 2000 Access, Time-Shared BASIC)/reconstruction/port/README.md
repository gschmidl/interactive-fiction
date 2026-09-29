# Adventure ]I[ — HP 2000 Access Time-Shared BASIC, ported to Windows

A 1400-point Colossal Cave descendant written around 1978/79 for an HP 2000
Access timesharing system, running natively on Windows.

This is a **source port**: there is no rewrite of the game. `advent.exe` is an
interpreter for HP 2000 Access Time-Shared BASIC, and it runs the archived
BASIC listings in `basic/` directly. The data files are not shipped either —
they are built on first run by executing the game's own data-builder programs,
the way Alex Guma intended them to be used.

The listings are the archived ones, with exactly one repair each, both shown
in full in `basic/<variant>/fixes.diff` and both undoable by copying
`basic/<variant>/unfixed/advent.txt` back over `advent.txt`. See
[Repairs](#repairs).

## Quick start

```bash
advent.exe
```

That plays the Guma reconstruction. To play the 1981 printout instead:

```bash
advent.exe -v orig
```

The first run of each variant builds its data files into `data/<variant>/`,
which takes a fraction of a second. `advent.exe --rebuild` forces a rebuild.

Commands are one or two words, abbreviated to five letters. Several commands
can go on one line separated by **periods**: `w. u. take lamp. light`. Type
`help` for the game's own instructions.

## The two variants

Two independent sources for this game survive, and they are not the same
program. Both are included.

### `recon` (default) — Alex Guma's 2023 reconstruction

Alex Guma wrote the game as an 11th-grader at Falls Church HS in Fairfax
County, VA, on the school district's HP 2000 Access network. In 2023 he
recovered and lightly edited it, and wrote the nine `mk*.txt` programs that
rebuild the data files. His edits retargeted the source at HP 3000 BASIC,
which is the dialect Quuxplusone's `advent.tap` runs under.

### `orig` — the Rick Hammerstone printout, 1981/82

A line printer listing of `ADVENT` and `ADVEN2` as they stood when Rick
Hammerstone played the game at a summer computer camp. This is genuine,
unedited HP 2000 Access source.

### Exactly how they differ

Ignoring whitespace, the complete list:

| | `orig` (HP 2000 Access) | `recon` (Guma) |
|---|---|---|
| not-equal | `#` | `<>` |
| deferred end-of-file | `IF END #n THEN` | `ON END #n THEN` |
| exponentiation | `^` | `**` |
| PRINT separators | `PRINT "x"B$` | `PRINT "x";B$` |
| chain | `CHAIN R9,"Advent.B500",1830` | `CHAIN "ADVENT",1830` |
| file names | `Matrix.L957`, `Comand.L957`, … | `MATRIX`, `COMMANDS`, … |
| account check | `SYSTEM T$[K1,K4],"Time"` then `Z7=POS("B500S452S310L804L615S405",T$[1,4])` | replaced by `Z7=1` |
| error trap | `2090 IF ERROR THEN 1190` | commented out |
| save file size | `2800 CREATE R9,A$,K2` | `2800 CREATE R9,A$,K5` (both work) |
| STAB / CHOP | has `5850 CHAIN R9,"Adven2",4850` and `5851 IF O1$#"TREE" THEN 5860` | **both lines are missing** |
| ALLEZ | `6850 ENTER K9,R9,K` | `6850 ENTER K9,R9,X` |
| | | adds `1011 R1=0` |

Two of these are behavioural. Each is a defect in one source that the other
source shows the fix for, so both are repaired — see [Repairs](#repairs).

* **STAB and CHOP were broken in `recon`.** Line 5850 is what sends a `STAB
  <creature>` to the kill-with-a-weapon code in ADVEN2. Without it, `stab
  witch` fell through into the CHOP-the-tree branch and answered `I see no
  witch here.`

* **ALLEZ was broken in `orig`.** `ENTER` read the room number into `K`, but
  line 6870 range-checks `X` and 6900 teleports to `INT(X)`. `X` still held
  whatever the command parser had left there, so `allez` always jumped to the
  same wrong room whatever number you typed.

SAVE and LOAD work in both, with each source's own `CREATE` size — see
"Record sizes" below for why Guma's `K2` to `K5` change was not needed here.

With those two repaired, **the two sources are behaviourally
indistinguishable**: all ten regression scripts — 200-turn sessions covering
every verb, all 67 rooms, the reactor countdown, save and load — produce
byte-identical transcripts from both. Which is the real result here. Two
copies of this program survived, one a 1981 line printer listing and one a
2023 recovery, and once each one's own transcription damage is undone they
agree exactly.

## Repairs

Two lines added to `recon`, one character changed in `orig`. Nothing else in
either listing was touched.

```
basic/recon/advent.txt
+ 5850 CHAIN "ADVEN2",4850
+ 5851 IF O1$<>"TREE" THEN 5860

basic/orig/advent.txt
- 6850  ENTER K9,R9,K
+ 6850  ENTER K9,R9,X
```

Neither is an invention: each variant's defect is repaired using the reading
the *other* variant supplies, so no new text enters the game. The two added
lines are the original's own 5850 and 5851 written in the reconstruction's
spelling (`<>` for `#`, and the bare `CHAIN` form it uses everywhere else);
the changed character is the `X` the reconstruction uses.

`basic/<variant>/fixes.diff` holds the unified diff against the archived
listing, and `basic/<variant>/unfixed/advent.txt` holds the archived listing
itself, so copying it back restores the shipped bug. Neither is ever loaded
by the interpreter. `../src_original/` remains untouched.

One consequence worth knowing: restoring 5850 makes lines 5851-5855 — the
"You don't have the axe" EPA joke — unreachable, because 5840 already sends
every non-creature to 5860. That is how the 1981 listing reads too, so the
dead code is the authors', not an artefact of the repair.

Two smaller quirks are *not* repaired, because both sources agree on them and
they are the game's own behaviour: `stab witch` without the knife answers
`You don't have the STAB WITCH.` (line 5830 only sets `A$` to `"KNIFE"` when
no object was typed), and `INSERT` is shadowed by the direction word `IN`.

`ADREAD`, the third chained program, was never printed out by either
correspondent. Only Guma's copy of it survives, so **both** variants use it.

The data files are Guma's too — the original ones are lost, and his listings
(`src_original/Guma/*.pdf`) are of a different version. He made two
deliberate fixes he documented: removing the exit from room 65 so the train
works, and hiding the duplicated train and card reader at the stations.

## What the interpreter implements

Statements: `COM` `DIM` `FILES` `DEF FN` `DATA` `READ` `RESTORE` `LET` `PRINT`
`MAT` `IF…THEN` `IF END #` `IF ERROR` `ON END #` `GOTO` `GOTO…OF` `GOSUB`
`GOSUB…OF` `RETURN` `FOR`/`NEXT` `INPUT` `LINPUT` `ENTER` `CONVERT` `ASSIGN`
`CREATE` `PURGE` `ADVANCE` `UPDATE` `CHAIN` `SYSTEM` `STOP` `END` `REM`.

Functions: `LEN` `POS` `UPS$` `CHR$` `NUM` `RND` `INT` `ABS` `SGN` `SQR` `LOG`
`EXP` `SIN` `COS` `TAN` `ATN` `TIM` `TAB` `LIN` `SPA` `FNx`. Operators include
`MIN` `MAX` `AND` `OR` `NOT` and both `**` and `^`.

Three details are worth calling out because the game depends on them:

* **Strings are fixed buffers.** `A$[i,j]` reads positions *i* to *j* of the
  physical buffer, and a whole-variable assignment blank-fills the rest of it.
  That is what makes the game's central idiom work: it matches a typed word
  against tables like `"WITCH:GIRL :SLIME:ZARKA"` by taking `O1$[K1,K5]`, and
  a four-letter word only matches `GIRL ` if position 5 is a blank.

* **CR and LF are independent.** On an HP terminal a line feed moves down and
  keeps the column; a carriage return goes back to column 1. The game relies
  on this — `PRINT "x"'10` leaves a blank line, while `PRINT "x"'13'10"y"`
  puts `y` hard against the left margin. The port models the carriage rather
  than translating character by character.

* **Declarations take effect at load time.** `COM`, `DIM`, `FILES`, `DEF` and
  `DATA` are processed when a program is loaded, not when control reaches
  them. `CHAIN "ADVENT",1830` re-enters the program past its `FILES` statement
  and the files still have to be open.

## Modelling assumptions

These are the places where behaviour is a reasoned model of the HP 2000 rather
than something the surviving sources pin down.

**Record sizes.** Data files are 512-word (1024-byte) records; an item never
straddles a record boundary; a number costs three words and a string one plus
`ceil(len/2)`.

That size is not a guess — it is derived. Assume the authors sized all eleven
of their files correctly, and ask what record size makes every `CREATE` they
wrote sufficient. `COMMANDS` in 3 records needs at least 104 words; `DESCRP`
in 60 needs 97; the original's save in 2 records needs 333. The smallest
record size consistent with all of them is 333 words, and the next power of
two is 512.

The one piece of evidence that appears to point the other way is Guma
changing the save file from `K2` to `K5`. But he was running the game under
**HP 3000 BASIC**, whose file layout is not HP 2000 Access's, so that change
is evidence about the machine he tested on rather than about the one the game
was written for. Taking the authors at their word instead, both sources save
and load correctly with the sizes they each chose.

On-disk files are a private format; nothing here reads or writes a real HP
volume. A data directory left over from a different layout is detected and
rebuilt automatically.

**ASSIGN status codes.** 0 = assigned, 1 = read only, 2 = no such file,
3 = wrong password. This is the only reading consistent with the game using
`IF NOT R9` before writing a save but `IF R9<K2` before reading one.

**Passwords are not enforced.** The game `CREATE`s a save file with no
password and then `ASSIGN`s it with `"Argyle"`, so a supplied password is
accepted against a file that has none.

**`SYSTEM x$,"Time"` returns the account id.** Both this program and the
earlier `ZORK` listing in `src_original/Guma/` use the result only as a
four-character account code. Defaults to `B500`, the account the game itself
lived in, which is on the wizard list — so `orig` gets the same `ALLEZ` and
`FIND` privileges that `recon` hardcodes. Use `-u S999` to play as an ordinary
user; `-u S3xx` will even reproduce the "Advent is down from 8:01 to 1:20"
lockout.

**`RND` repeats.** As on HP, the sequence is identical from run to run unless
the program reseeds it. `-r` seeds from the clock instead. This matters: the
dark-room xeener bugs kill you 90% of the time, so without `-r` they always
kill you on the first attempt.

**`Score` and `Bug` files.** These belong to the HP account rather than to
the game, and the game assumes both are already catalogued. The launcher
creates them empty on every start, as the account would have had them.

This matters more than it sounds. `QUIT`, after printing `Okay.`, does:

```
2370 ASSIGN "Score",K3,R9
2380 IF R9 THEN 2370
```

On a real HP that retry waits for whichever other user has the file open. On
a single-user port nobody is holding it, so if `Score` is simply not there the
loop can never end: the game prints `Okay.` and spins in silence. Hence
checking for the file on every start and not only when the data files are
being rebuilt.

As a backstop the interpreter also recognises the pattern. Two `ASSIGN`
failures for the same missing name, from the same line, with no terminal I/O
in between, cannot be anything but that loop, so it stops and says which file
is missing and where. A person retrying a name by hand types something in
between, so it never fires on them.

## Options

```
advent                 play the Guma reconstruction
advent -v orig         play the 1981 Hammerstone printout
advent --rebuild       rebuild the data files first

-p DIR   directory holding the BASIC source (also honoured in game mode)
-d DIR   directory holding the data files   (also honoured in game mode)
-u ID    account id reported by SYSTEM        (default B500)
-r       reseed RND from the clock
-e       echo input lines when stdin is not a terminal
-q       do not print DONE when a program stops
PROG...  run these programs instead of the game
```

The last one makes `advent.exe` a general HP 2000 Access BASIC interpreter:

```bash
advent.exe -p basic/recon -d data/recon mkmatrix
```

## Layout

```
advent.exe            the interpreter and game launcher
src/hptsb.c           all of it, one C99 file
basic/recon/          Guma's reconstruction
basic/orig/           the Hammerstone printout
basic/*/fixes.diff    the one repair made to each, as a unified diff
basic/*/unfixed/      the archived listing, before that repair
basic/*/catalog.txt   HP account catalog: maps Matrix.L957 -> MATRIX
data/<variant>/       built on first run; delete to rebuild
tests/regress.sh      20 recorded transcripts, both variants
```

The `.txt` files under `basic/` are copies of what is in `../src_original/`,
carrying only the repairs above. The loader ignores anything that is not a
numbered line,
which is why the Hammerstone transcriptions can be used as they are, with
their `ADVENT` header line and their line printer indentation glitches.

## Build

```bash
build.cmd          # Windows
make               # anything else
make test          # run the regression suite
```

One C99 file, no dependencies beyond libm. Builds clean under
`-Wall -Wextra`. The Windows-only code is the `ENTER` time limit; it falls
back to `select()` elsewhere.

## Provenance

Everything under `../src_original/` came from
<https://github.com/Quuxplusone/Advent/tree/anon1400/ANON1400>:

* `Guma/` — files Alex Guma sent in July 2023: printouts of an earlier
  version's program and data listings.
* `GumaReconstruction/` — the playable reconstruction, August 2023.
* `Hammerstone/` — Rick Hammerstone's 1981/82 printout, transcribed.
