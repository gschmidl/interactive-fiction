# Getting LANDS OF ZARAST off the tape

Notes on decoding the five `.SHR` images and making them run. Everything
here was derived from the tape itself; nothing was borrowed from another
game.

## The files

They are in `tapes/mpl` of the Tymshare tape collection, all dated
1984-12-27, plus the characters other people left in `tapes/novafield`.

| file | bytes | words | what it is |
|---|---|---|---|
| `dungen.shr` | 256,790 | 51,358 | the dungeon crawl — the game |
| `ventur.shr` | 148,725 | 29,745 | an older stand-alone adventure |
| `filer.shr` | 99,910 | 19,982 | builds the world file |
| `charac.shr` | 87,995 | 17,599 | character creation, long form |
| `charc.shr` | 87,955 | 17,591 | character creation, quick form |
| `newadv.dat` | 67,645 | 13,529 | the world |
| `dungen.hlp` | 1,828 | — | the author's instructions |
| `dungen.not` | 547 | — | his command list |

`libr.adv` is in `mpl` too — a character called PAM, a dwarf thief.

## Word format

Every length is a multiple of five, which is the first clue: a 36-bit
PDP-10 word is five bytes, each holding one 7-bit septet right-justified,
with the word's spare 36th bit in the high bit of byte 4. This is the
"5 septets per word" ANSI-ASCII tape mode, and the same encoding the
EXPLOR files on this tape use. Only byte 4 of each group of five ever has
its high bit set, in any of these files, which is exactly what the layout
predicts.

## What language

`dungen.shr` decodes with `TBA system error` and `INTENB Error return`
near the front, and 211 files on the tape start at `400326`, these five
among them. TBA is Tymshare BASIC; these are compiled TYMBASIC programs
with its runtime linked in. `tapes/spunkdoc` has the *TYMBASIC Reference
Manual* (`tbarm.mem`) and *TYMBASIC GID — General Internal Design*
(`tbagid.mem`), and `tapes/calstate` has the TYMCOM-X monitor sources —
which between them answer nearly every question below.

## Load format

The file *is* the high segment, word for word, loading at `400000`. There
is no header to skip and no low segment on disk: a compiled TBA program
builds its low segment at run time, copying the initial contents down out
of the high segment with a BLT. The compiler puts its equivalent of the
job data area in the first words, and the monitor reads it back out:

```
400000   XWD first free low-seg address, start address    (.JBSA)
400001   the REENTER instruction                          (.JBREN)
400002   RH = highest legal low-segment address           (.JBREL)
400003   LH = length of the file in words
400004   XWD 002006, 000001
```

The check that settles the origin is the pair at `400333`/`400334`,
`MOVE 1,401124` / `BLT 1,2401`: at this origin `401124` holds
`XWD 401026,002364`, which copies fourteen words of initial low core from
`401026` down to `2364..2401`. One word out in either direction it is text
or an instruction, and the BLT reads from nowhere. The start address then
comes out as a `JRST` to the real entry at `401120`, with the REENTER path
one word behind it — which is what a start address should look like.

## The missing word

Load the file flat at `400000` and the code below a certain point resolves
perfectly while everything above it is displaced by one. There is a
mechanical test for this. A TBA routine ends

```
        SUB  17,X
        POPJ 17,
```

and it pushed some fixed number of words on entry, so `X` must hold an
`XWD n,n` literal. In `ventur.shr` as it comes off the tape, 46 of the 71
such epilogues land on one and the other 25 land one word past it. Insert
a single word at file index `7164` and **all 71 land** — and 79 of 79 in
`dungen`, 72 of 72 in `charac` and `charc`, 65 of 65 in `filer`.

It is not damage peculiar to this game. Every TBA-compiled `.SHR` on the
tape needs the same repair, at a position that depends only on which build
of the runtime was linked in: `7164` for these five and for `fscord/custm`
and `dulam/tktown`, `4410` for `fttba/cnest` and `games/cavern`, `5076`
for `games/bowlng`. The hole always falls immediately after a `POPJ` that
ends a routine, at a module boundary. Nothing in any of the five images
addresses the word it occupies, so a zero restores a working image.

`tools/mkimages.py` finds that index from each file rather than trusting a
constant, and refuses to emit an image it could not make self-consistent.

### The same wound, provable

`NEWADV.DAT` is where this stops being an argument and becomes a
measurement, because FILER regenerates it and FILER is deterministic.
Compare the tape's copy against one the port's own FILER builds:

* words 0..3673 are identical;
* from 3674 on, `tape[k] == fresh[k+1]` for 9,849 of the 9,854 words;
* the tape file's last word is not in the fresh one at all.

One word missing at 3674, one word of padding at the end to keep the byte
count. The five words that still differ read 15 on the tape where a new
world has 3 — the world as people left it, not damage. So the tape
extraction drops one word per file and pads the end, and that is the whole
story for the `.SHR` files too: their last words are a per-file oddity
sitting above anything the code refers to.

`tools/mkworld.py` puts that word back, from this game's own FILER output,
and writes `data/newadv-1984.dat`.

## Making it run

The CPU is the DECsystem-10 emulator written for the EXPLOR port,
unchanged. The monitor needed real work, because TBA asks for much more
than FORTRAN did.

### TYMCOM-X calls

The negative CALLIs are Tymshare's own. Their names and calling sequences
are in the monitor sources on the tape (`calstate/p034n.fdm`, the P034
edit), which carries DDT's whole symbol table — `CALLIN`, from `-116
SNOOPU` down to `-1 LIGHTS`. The convention throughout is that the AC is
both argument and result, and that success takes the skip return.

The set these five programs use is almost entirely the runtime arranging
for `^C` and `^O` to interrupt it, asking what kind of terminal it has,
and identifying itself to Tymshare's accounting:

| call | what the port does |
|---|---|
| `INTADR` `INTENB` `INTACT` `INTASS` `SETTR1` `SETTR2` `TINASS` | hand back "nothing was set before", skip |
| `DISMIS` | nothing to dismiss |
| `GETTMC` `SETTMC` `SETMOD` | terminal mode |
| `LSAUUO` | accounting. The monitor's own comment says it always returns to the call, AC intact |
| `SETPRV` `CHKLIC` | privilege, licence |

Of those, a run of the game actually reaches `SETPRV`, `INTENB`, `INTADR`,
`INTASS`, `TINASS`, `SETTR1`, `GETTMC` and `LSAUUO`; the rest are answered
because the images contain calls to them on paths this port has not been
down.

Two of DEC's five customer-defined opcodes matter. `042` is `AUXCAL`,
which drives an auxiliary circuit — a second terminal, or another job's
pseudo-terminal. TBA probes for one around every line of input and output;
there is never going to be one here. `043` is `CHANIO`, and that one is
load-bearing: it is every file UUO in one instruction, with the function
in the left half of the AC and the channel in the right, so a program can
pick its channel at run time. TBA does all of its file work this way. The
function numbers and their mapping onto the ordinary UUOs come from the
monitor's own `CHNOTB` dispatch table.

`TTCALL 16` (`OUTCHI`, output the character in E itself) and `TTCALL 17`
(`OUTPTR`, output an ASCIZ string through the byte pointer in C(E)) are
TYMCOM-X extensions; `OUTPTR` is how TBA writes most of its output.

### Files

A TOPS-10 disk file is an array of 36-bit words and the programs use it as
one: `NEWADV.DAT` is 13,529 single-precision floats laid end to end, read
in 512-word dump-mode transfers positioned by `USETI`. A file is held here
as a word array, read whole at LOOKUP and written back at CLOSE, which
makes random access and rewriting the same operation.

Three things had to be right or the game would not start:

**Extended LOOKUP/ENTER blocks.** TBA uses the long form, which begins
with a word count rather than the filename — that is how the two shapes
are told apart, since a real filename always has something in its left
half. `.RBSIZ` has to come back in words, because that is what the runtime
uses to decide how much of the file there is to read.

**Update mode.** A LOOKUP followed by an ENTER of the same file on the same
channel means "rewrite parts of this in place", not "start it again". TBA
opens `NEWADV.DAT` and `SCENAR.IO` exactly like that, and an ENTER that
truncated threw the world away — TYMBASIC run-phase error 142, *End of
file found*, on the next read.

**The directory.** DUNGEN will not start until it has satisfied itself that
`NEWADV.DAT` and `SCENAR.IO` are both there — *"SORRY YOU MUST HAVE TWO
FILES ON YOUR SYSTEM TO PLAY / RUN FILER.SHR"* — and the way it looks is
the way a TOPS-10 program looked: it opens the directory file itself and
reads it with `CHANIO` function 33. The port makes a UFD out of whatever
is in the save directory. The calling sequence, down to which word holds
the count and which the continuation, is from `UFDUUO` in the monitor
sources.

### The case of what you type

`LDLLCT`, "LOWER CASE TO UPPER CASE", is a TYMCOM-X terminal line flag,
and the scanner acts on it before a program sees a character:
`TLNE U,LDLLCT` / `SUBI T3,40` / *"HERE IN LOWER CASE, CONVERT TO UPPER"*.
It was on for the terminals these programs were written for, and they
assume it — DUNGEN compares your answer against `Y`, so a typed `y` falls
through to the no branch and the offer to roll up a character can never be
accepted. TBA reads the line with `GETLCH` once and never sets it, so the
setting is the terminal's; the port folds, and reports the bit back
through `GETLCH` in the position the monitor keeps it in.

### The dice

Characters came out with 9/9/9/9/9/9 for their abilities. TBA's `RND`
reads `MSTIME`, and the game rolls its dice with a `HIBER` between each
roll — so a clock that only advances in whole seconds hands back the same
"random" number six times. Giving `MSTIME` real millisecond resolution
fixes it, and so does having `HIBER` and `SLEEP` move a virtual clock on
even when `-q` says not to wait for real.

### A 1984 filesystem artifact, reproduced

`mpl/libr.adv` is the author's own character, PAM, and her file ends with
one string more than anyone else's:

```
PAM@ DWARF@ THIEF@ EVIL@ @ ROGUE@ * GITATOR@ *
```

Neither program writes anything after the `*` — CHARC and the game both
write name, race, class, alignment, password, rank, `*` and stop. And
`GITATOR` occurs nowhere in the source except inside `PRESTIDIGITATOR`,
the first-level magic-user rank.

It is stale data. TBA opens a character file `FOR SYMBOLIC RANDOM IO`,
which is a LOOKUP followed by an ENTER — update mode, which does not
truncate — so rewriting a file with something shorter leaves the tail of
the old contents behind. Roll PAM up as an elf magic-user and then roll
her again as a dwarf thief, and the port produces a 280-byte file whose
string section is byte-for-byte the one on the tape, `GITATOR@ *` and
all. That is what happened in 1984, and it doubles as a test of the
update-mode handling: get the truncation wrong and the artifact cannot
be reproduced.

### The later version

`novafield` holds the game a second time, later: `pub.shr` and `b.shr`,
the character generator `cr.shr`, and — unlike `mpl` — the BASIC source,
`pub.tba` (134KB, 1987-02-12) and `charc.tba`. `zarast87.exe` runs those.

It needed no new work. They are TBA images with the same runtime build as
the 1984 ones, they have the same one-word hole at the same index `7164`,
and `tools/mkimages.py` finds and repairs it the same way — 77 of 77
epilogues in `pub` and `b`, 72 of 72 in `cr`. They ran first try on the
emulator as built.

How much of a revision is it? Almost none, in content. Pull every string
of 25 characters or more out of `dungen.shr` and out of `pub.shr` and you
get 516 from each, and they are the same 516 — room descriptions,
monsters, objects, messages, the parser's vocabulary, the spell and rank
tables. The images are 51,358 and 51,140 words: the difference is code,
about 220 words of it. The 1987 set also has just the one character
generator where 1984 had a long form and a quick form, and no FILER of
its own, so `zarast87` uses the 1984 one — same author, same world file,
and the later game reads what it writes.

Characters and world files are interchangeable in both directions,
including the ones that came off the tape in 1984.

The source was not used to build either binary, and is not needed to.
It is what settled the questions above — what the character file format
is, what the game opens, why PAM's file has a tail — and anyone wanting
to read how the game works should start there.

## Confirming it

* Characters this port writes have the same layout as the ones on the
  tape — the same eight-column numeric fields, the same `@`-terminated
  strings, the same trailing `*` — and `GIJOE`, `SHERA` and the rest of
  `novafield` load and play.
* `FILER` in this port produces a `NEWADV.DAT` that lines up word for word
  with the one on the tape, once the tape's dropped word is put back.
* Six hundred commands of random input through `DUNGEN` with three
  characters in the party, and six hundred more through `VENTUR`, raise no
  unimplemented monitor call at all.
