# Adventures in Pascal — Windows port

**Barry C. Breen, 28-Oct-80, rev. 3-Dec-82.** Colossal Cave (the 350-point
Crowther & Woods game) rewritten in **OMSI Pascal V1.2 for RSX-11M V3.2 on a
PDP-11/23**, "as an exercise in learning PASCAL and RSX while developing
software for the Sundstrand Data Control Digital Ground Proximity Warning
Computer for the Boeing 767/757". It was adapted from Kent Blackett's 1977
FORTRAN IV-PLUS/IAS version "found in a cave", and came out on the DECUS RSX
SIG tape **RSX82B, UIC [351,130]**.

What Breen added to the cave: separately compiled, overlaid modules (the task
ran in under 12 K words); pointers and linked lists for the travel table and
the object lists; a vocabulary stored as a letter tree on disk; a working
wizard mode with its state in a file instead of a saved core image; and a
second text database for the **VT100** that uses double-width and double-height
lines for signs, voices and magic words.

## Playing

    adventure.bat            (or adventure.exe from a console)

The game first asks `Are you using a VT100?`. Windows Terminal is one: answer
`yes` and the note in the debris room, the dwarves' voices and the hollow
PLUGH come out in big letters, as they did in 1980. Answer `no` for plain text.

The data directory holds the author's own `ADVWIZ.DTA`, exactly as it left his
machine, and that file says the cave is **shut to non-wizards for most of the
working day** (his site's hours: open 7-9, 11-14 and 16-20 on weekdays). You can

* play inside those hours, or take the 35-turn demonstration game offered outside them;
* be a wizard (below); or
* run `adventure -u`, which ignores the hours and the 45-minute wait before a
  suspended game may be resumed. `-u` changes nothing on disk.

| option | |
|---|---|
| `-u`, `--unlimited` | ignore cave hours and the resume latency; answer the wizard test for you (below) |
| `-d`, `--date=DD-MMM-YYYY` | pretend it is this date |
| `-t`, `--time=HHMM[SS]` | pretend it is this time. The random numbers are seeded from the time of day, so a frozen clock makes the whole game repeat exactly |
| `--echo` | show each input line (for transcripts made with redirected input) |
| `--no-fixes` | leave the three original bugs in (see *Fixes*) |
| `--data=DIR`, `--save=DIR` | where the database and `ADVWIZ.DTA` live (defaults `data\` and `save\` beside the exe; also `ADVPAS_DATA`, `ADVPAS_SAVE`, `ADVPAS_DATE`, `ADVPAS_TIME`) |
| `-h`, `--help` | |

`SUSPEND` (or `PAUSE`, `SAVE`) stores the game under your name in `ADVWIZ.DTA`;
there are three slots, a saved game can be resumed once, and not within 45
minutes. `save\ADVWIZ.DTA` is created from `data\ADVWIZ.DTA` the first time it
is needed; delete it (or run `poof.exe`) to start over.

### Being a wizard

The message of the day in the author's file is a riddle that gives it away:

> The word that you need is a very small guy, / And a number of ones plus the hour that you try. /
> Add to the letters, as viewed on the screen / And type them back at this infernal machine.

Say you are a wizard, give the magic word **DWARF**, answer **NO** to "Do you
know what I thought it was?", and the game shows five random letters. Shift
each one forward by *magic number digit + hour of the day* (the number is
11111, so at 09:xx that is 10 places: `XJNBV` → `HTXLF`) and type the result.
`MAGIC MODE` **as the very first command** of a game does the same test and
then lets you change the hours, schedule a holiday, set the length of the demo
game, the magic word and number, the latency, and the message of the day.

### Under `-u`, the game answers its own test

Doing that sum in your head against the clock is the awkward part — the reply
goes stale when the hour turns. With `-u` the game tells you both secrets as it
asks for them, the way the TOPS-10 350-point port does:

```
->magic mode
Are you a wizard?
->yes
Prove it!  Say the magic word!
 -u: the magic word is DWARF
->dwarf
That is not what I thought it was.  Do you know what I thought it was?
->no
JNBVM
 -u: the reply is TXLFW
->txlfw
Oh dear, you really *are* a wizard!  Sorry to have bothered you.....
```

(The answer to the question in the middle is **no**; *yes* makes you an
impostor.) Both, because both are secrets: maintenance mode lets a wizard
change the magic word, and a changed one is otherwise unrecoverable. On a live
clock, in the last ten minutes of the hour, a third line says what the reply
turns into (` -u: from 10:00 it becomes UYMGX`), since that is exactly the trap;
with the clock frozen by `-t` it cannot go stale.

This is not a second implementation of the rule. `CHKMAGIC` in `WIZMAG.PAS`
works out the expected reply in order to check it, and `OMSIWIZREPL` in
`src/wizhint.inc` is that loop copied statement for statement, fed the same
magic number the game has just read from `ADVWIZ.DTA` — so if a wizard changes
the word or the number, the hint and the check move together
(`test/scripts/11-unlimited-wizard-hints.txt` does exactly that: DWARF/11111 at
09:00, then GNOME/12345 at 23:30). It costs two calls in the generated source,
both of which return at once without `-u`; nothing is printed without it.

Note that under `-u` maintenance mode also *shows* the hours as always open and
the latency as 0, because that is what `-u` makes the game read. Only what you
actually change there is written to `ADVWIZ.DTA`.

`peek.exe` lists what `ADVWIZ.DTA` holds (that was the Grand Wizard's cheat);
`poof.exe` writes a fresh one: DWARF, 11111, cave always open.

## How it was ported

**The game is the author's source, compiled** — not a translation. Free Pascal
3.2.2 (`{$mode tp}`: 16-bit `INTEGER`, as on the PDP-11) builds a native
64-bit Windows program from `src_original\*.PAS`, which is never edited.

`tools/weave.py` copies each module to `build/gen/<module>.inc`, line for line
(line N of the `.inc` is line N of the `.PAS`), making only these substitutions.
All but the last two items are dialect:

1. `PROCEDURE X(...);EXTERNAL;` and module-level `FORWARD` declarations are
   blanked. OMSI compiled every module separately against `ADVGBL.PAS`
   (`PAS <module>=ADVGBL,<module>/E`) and TKB joined them; here the modules are
   included into one program and `forwards.inc`, generated from the real
   headers, declares every global-level routine. The tool checks that every
   `EXTERNAL` has a definition and that the headers agree (they do, apart from
   parameter names).
2. OMSI random-access files. `F:FILE OF T` becomes a file handle plus an
   explicit buffer `F_buf`; `F^` → `F_buf`; `RESET/REWRITE/SEEK/GET/PUT/CLOSE`
   on those files call the run-time unit.
3. `EXIT` — in OMSI Pascal it leaves the innermost loop (DATIME goes on to use
   the loop variable afterwards, which settles it) — becomes `BREAK`.
4. `{$C ...}` inline MACRO-11 becomes a comment. Only one block does anything:
   DATIME's `GTIM$` directive, replaced by a call that returns month, day, year-1900.
5. Octal `33B` → `27`. The identifier `OBJECT` (reserved in Free Pascal) → `OBJECT_`.
6. `READLN` from the terminal and `READ/READLN` from the ASCII database go
   through the run-time unit (blank-padded character arrays; OMSI's integer
   `READ` stops at the first non-digit — `1You are standing...` — and takes commas).
7. Twenty-four exact-text patches, each applied exactly once or the build stops, each
   with its reason in `weave.py` and in `build/gen/weave.log`: name- versus
   structure-equivalence of array types (6), a `FOR` counter borrowed from the
   enclosing procedure or a `VAR` parameter (5), `ARR5` declared once instead of per
   module (2), the pre-ISO `PROCEDURE SPK` parameter (1), the `GTIM$` call of
   item 4 (1), the debug hook (1), the VT100 auto-wrap call described below (1), the two
   `-u` wizard-hint calls (2), and:
   * **two NIL reads.** `WHILE (LINK2^.VERBVAL<>K) AND (LINK2<>NIL)` and, after a
     `BACK` with no way back, `I<>LINK1^.NEWLOC[2]` with `LINK1=NIL`. OMSI
     evaluates both operands and an RSX task may read location 0, so the PDP-11
     never noticed and the value read cannot change the outcome; Windows faults.
     The tests are reordered / guarded.
   * **FIX 1, FIX 2 and FIX 3**, below.

`src/omsirt.pas` is the run-time unit: the OMSI file semantics (`SEEK(F,n)`
loads record *n* into `F^`, records count from 1, `PUT` rewrites in place and
moves on), `TIME` (hours since midnight, a REAL), the clock, the command line,
the console (VT processing on; the game leaves a VT100 in VT52 mode on exit, as
RSX terminals usually were, so the port sends `ESC <` afterwards to put a
modern terminal back in ANSI mode).

**Auto wrap.** Every line of text is written as all 72 columns, trailing blanks
included, and a double-width or double-height line holds only 40. A VT100 left
the factory with auto wrap *off*, so the extra blanks piled up harmlessly at the
right margin. Windows Terminal has it on: the blanks wrapped into a blank row
under every big line, and the top and bottom halves of double-height text
("Adventure!!", "Magic Word XYZZY") ended up a row apart. So when the player
answers yes to "Are you using a VT100?" the port gives them one: `ESC[?7l`, at
the point where the game itself sends `ESC <`, and `ESC[?7h` again on the way
out — on a normal exit, at end of input, and from a Ctrl-C / window-close
handler, so the shell is never left unable to wrap. `test/console_check.py`
reads the pseudo console's own output stream and insists that every `ESC#3`
line is followed directly by its `ESC#4` line.

### The database, and what it proved

OMSI's `FILE OF T` on disk: records never span a 512-byte block — each block
holds `512 DIV SIZEOF(T)` records (6 text lines of 74 bytes, 9 vocabulary nodes
of 56, 256 integers) and the rest of the block is zero.

`ADVFLS` and `100FLS`, the author's programs that build the binary files from
the ASCII database, are ported the same way as the game, and
`test/check_data.py` runs them and compares with the binary files on the tape:

    ADVTXT.DTA   914 records written, all identical      KATAB.DTA  768
    ADVDAT.DTA   771 pointers                            ADVENT.DTA 11754 integers
    ADVTXT.100   997 records                             ADVDAT.100 771 pointers

Every byte the programs write is the byte on the tape. The only differences are
after the last record, where the tape files still hold what the disk block
contained before (RSX does not clear it). That one comparison checks the file
layout, the `SEEK/PUT` semantics, the integer reader and the compiled logic of
two programs at once. `data\` holds the tape's own files, unchanged.

**`ADVENTURE.100` was lost and is recovered.** On the tape (and on ibiblio) it
is a byte-for-byte copy of `ADVENTURE.DAT`; the VT100 text survived only inside
the binary `ADVTXT.100`. `tools/mk100.py` undoes `100FLS` — sections 1-6 from
`ADVTXT.100`, split where `ADVDAT.100`'s pointer arrays say, sections 7-12 from
`ADVENTURE.DAT` (the readme says they are common) — and the round trip through
the ported `100FLS` reproduces `ADVTXT.100` and `ADVDAT.100` exactly. 190 lines
use `ESC#3`/`ESC#4` (double height) or `ESC#6` (double width).

### Fixes (off with `--no-fixes`)

All three are in the 1980 source. The first two were found by fuzzing a
range-checked build, the third by using maintenance mode the way anyone would today.

* **FIX 1 — the two fatal falls did not kill.** Breen changed "dead" from
  FORTRAN's location 0 to -1 everywhere in the code (the dwarves' knives, the
  troll bridge with the bear), but the database still sends location 20 (broken
  neck at the bottom of the pit) and 21 ("You didn't make it") on to location
  **0**. The original then carried on *at* location 0: it listed every object
  that does not exist yet (the rusty-mark rod, the oyster, the pirate's chest,
  the pearl…), and the next move indexed `KEY[0]`. The port turns a forced
  move to 0 into -1, and the usual "Oh dear, you seem to have gotten yourself
  killed" follows.
* **FIX 2 — bare `TAKE` beside the far end of a two-place object.** The second
  location of the grate, steps, fissure bridge etc. is linked in as `OBJ+100`.
  `WHATSHERE` folds that back; intransitive `TAKE` did not, and indexed
  `PLACE/FIXED/PROP[107]`, off the end of 64-element arrays (the FORTRAN has the
  same slip, harmlessly inside COMMON). Now: "You can't be serious!".

* **FIX 3 — a new magic word typed in lower case locked the wizards out.**
  Maintenance mode stored the word exactly as typed, but `GETIN` capitalises
  whatever the player types before `WIZARD` compares it, so `gnome` could never
  be matched again and only `POOF` got anyone back in. An RSX terminal of 1980
  usually upper-cased input by itself; a PC does not. The new word is now
  capitalised as it is stored.

With `--no-fixes` the first two paths read whatever Free Pascal put next to the arrays,
which is not what a PDP-11 had there; nothing faithful is possible.

Left alone, because they are how the game was: a saved game can be resumed only
once; `MAGIC MODE` only works as the first command; new cave hours are *added*
bit by bit, so overlapping ranges entered in magic mode corrupt the mask; every
text line is written as all 72 columns, trailing blanks included; "26 days ,".

## Checking it

    python test/check_data.py       the database comparison above
    python test/run_tests.py        eleven recorded sessions, clock frozen, byte-exact
    python test/console_check.py    at a real (invisible) console: prompts appear before
                                    input is awaited, ESC#3/#4 reach the terminal with the two
                                    halves on consecutive rows, clean exit, Ctrl-C
    python test/fuzz.py 1 200 500   random commands against the debug build
    python test/fuzz.py 1000 400 600 --deep    ...and from deep inside the cave

All of them use throwaway save directories; `save\` is never touched.

The sessions include the closed cave and the demo game, the wizard test and
maintenance mode, suspend / too soon / wrong name / resume / already resumed,
the first two fixes, `BACK` with no way back, the VT100 text, the `-u` wizard hints
following a changed magic word and number (which is also FIX 3; the script is
written by `test/make_wizard_test.py`, which checks each hint against the rule
worked out by hand), and **a complete endgame
scoring 350 out of 350** ("Adventurer Grandmaster").

`adventure --debug` is the test bench that makes that last one possible without
fighting the dwarves (`src/debug.inc`, not part of the 1980 game): at any
prompt, `#goto N`, `#take N`, `#put N LOC`, `#prop N V`, `#where N`,
`#dwarf I LOC`, `#set NAME V`, `#show`.

The final fuzz — 200 runs of 500 commands and 400 deep runs of 600, against the
build with range and overflow checks — ends with no run-time error.

## Building

Free Pascal 3.2.2 in WSL (Debian packages `fpc` and `fpc-source`). Debian's
`ppcx64` already targets Win64 with its internal assembler and linker; only the
Win64 run-time units are missing, and `tools/build-fpc-win64-rtl.sh` compiles
them from `/usr/share/fpcsrc` into `~/fpc-win64/units` — no root, no binutils.

    wsl -d Debian -- bash tools/build-fpc-win64-rtl.sh     once
    wsl -d Debian -- bash build.sh                         adventure, peek, poof, advfls, 100fls
    wsl -d Debian -- bash build.sh debug                   build/dbg/adventure-dbg.exe

`build/` is entirely derived.

## Files

    archive_original\   the fifteen files of [351,130] as downloaded from ibiblio
    src_original\       the 50 modules extracted from ADV.ULB by port\tools\ulbx.py
                        (48 .PAS, ADVHDR.TXT, ADVBLD.ODL), inserted 6-Oct-80 to 29-Oct-81
    port\src\           omsirt.pas, the five program shells, glue.inc, wizhint.inc (-u hints),
                        debug.inc (--debug)
    port\tools\         weave.py, mk100.py, ulbx.py, build-fpc-win64-rtl.sh
    port\data\          the tape's database and ADVWIZ.DTA; ADVENTURE.100 rebuilt
    port\save\          your ADVWIZ.DTA (created on first use)
    port\test\          scripts, transcripts and the checks above

ADV.ULB is an RSX-11M LBR *universal* library: a header block, a table of
8-byte module entries (two RAD50 words, block, byte), and block-aligned modules
whose 64-byte header carries the insert date, the file type in RAD50 and the
input file's FCS attributes, with the file's blocks following verbatim. All 50
modules extract with the record stream ending exactly at the recorded end of
file and the longest line equal to the recorded record size.

ADV.OLB (the OMSI-compiled, "improved", MACRO-assembled objects) is kept but
not used: without OMSI's PASLIB no task can be built from it.

## The author's ADVWIZ.DTA

Magic word DWARF, number 11111, demo game 35 turns, latency 45 minutes, no
saved games, the riddle as message of the day, and a holiday named
**"Wizard Week"** on days 1443-1447 of the game's own calendar — 23 to 27
November 1980, Thanksgiving week, which dates the file to the month after the
game was finished. (`adventure -d 28-OCT-1980` says "The next holiday will be
in 26 days".)
