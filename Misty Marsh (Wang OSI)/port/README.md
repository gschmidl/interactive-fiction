# Misty Marsh (Wang OIS)

A cave adventure that is not a program. Misty Marsh was written on a Wang word
processor as a **glossary** — a keystroke macro that types a document at you,
reads your answer back out of the document, and jumps to another glossary entry
depending on what it finds. There is no interpreter to port and no source to
compile: the game *is* the macro.

`misty.exe` runs that macro. Every word it prints and every branch it takes is
read out of `src_original/mistymarsh.img` at build time; nothing in the game
text or its logic is written here.

    make regen        disk image -> data/*.bin -> src/data.h -> misty.exe
    make              rebuild the interpreter alone
    ./misty.exe       play

## What is on the disk

The image is 616 sectors of 512 bytes, addressed as 1232 blocks of 256: a
7-byte header, then 249 bytes of payload, with free blocks filled with `0xAA`.
Text is stored with **bit 7 set** on every character. Three documents matter:

| doc | what it is |
|-----|------------|
| `0x10` | MISTY MARSH GAME / OIS VERSION — the playing document |
| `0x59` | MISTY MARSH GAME / WP VERSION — document and glossary |
| `0x01` | the glossary printed as a readable listing, keystrokes spelled `(-LIKE-THIS-)` |

`0x01` is what the port compiles. It is a *printout* of the glossary, so it also
carries line breaks and format lines that were never part of the program, and
the comparison against the second copy in `0x59` is what settled which bytes are
which.

## The language

Ninety entries, each named by one character and reached with `(-GO-TO-GL-)x`.

| | |
|---|---|
| `(-N-KEYS-)` | collect what the operator types, up to EXECUTE |
| `(-IF-)"c"` … `(-END-)` | is the character under the cursor `c`? `(-IF-)-` negates |
| `(-BACKSPACE-)`, `(-EAST-)` | walk the cursor back over the answer and along it |
| `(-PROMPT-)text(-EXECUTE-)` | write the workstation's prompt line |
| `(-GO-TO-PAGE-)f`, `(-SEARCH-)`, `(-INSERT-)` | the game's memory (see below) |
| `(-IF-)(-PAGE-)` | did the last search run off the end? |

Answers are tested a letter at a time, because the answer is not a string
anywhere — it is text the operator typed into the document. The pit puzzle is
the clearest case: three backspaces, then `d`, `i`, `g`, each its own `(-IF-)`.

## The two scratch pages

**Page f is the map.** Opening a door inserts a mark there; coming back to the
same door searches for it, and `(-IF-)(-PAGE-)` — "did the search run off the
end" — is how the game asks whether it has been there before. The opening
keystrokes seed the page with a mark `M` so it is never empty.

**Page w is the scoreboard, and the only arithmetic in the game.** Each prize
writes a decimal-tabbed `10` there. At the end, entries `(1)(2)(3)` walk the
column and total it with the word processor's own math command — `+a=` opens
accumulator *a* on the number under the cursor, `a+` adds the next one, `a#`
totals — then copy the result into the document behind `Your score = `. The
game's own closing note apologises for scores that come out as `00`; here the
column adds up, so the number it prints is the one the glossary computed.

## Two things the disk got wrong, and how they were found

**The blocks are not in numbered order.** Every document carries a block
numbered `0.0`, and it is not the first — it sits physically *after* a run and
continues it. Sorting by number files it at the front, which moves a chunk of
text 60 kB backwards, where it looks like a damaged opening. In disk order all
89 entry markers still land at offset 64 of a block, and the sentence the `0.0`
block starts with joins up with the one the last entry breaks off in the middle
of. `extract.py` reads blocks in disk order.

**Entries run on past their ends.** Each entry is a page, a page is a whole
number of blocks, and the tail of the last block still holds whatever was there
before — often perfectly readable text from another entry, so it cannot be
spotted by eye. The rule that works comes from the language:

* an entry ends at the first `(-GO-TO-GL-)` outside any open `(-IF-)`, since
  nothing after an unconditional jump can run;
* unless what follows the last `(-END-)` is plain text rather than a keystroke,
  in which case the entry ended at that `(-END-)` — this is the door hub `(a)`,
  seven guarded jumps and then the middle of somebody else's sentence;
* the four entries that never jump are the deaths: they finish
  `(-ERROR-)Game's over!(-EXECUTE-)` and stop, so they end at that keystroke.

That places all ninety. One block was left part-written and needs the single
documented deletion in `ERRATA` at the top of `parse.py`; the disk's own playing
copy of the game confirms nothing is missing there.

## Files

    src/misty.c       the interpreter -- the workstation, not the game
    src/data.h        generated; the glossary, compiled
    tools/extract.py  disk image -> data/doc*.bin
    tools/gloss.py    read the glossary by hand: -l for all, -e c for one entry
    tools/parse.py    the compiler, and where every judgement call is written down
    tools/gendata.py  -> src/data.h
    alternative/      a hand-written JavaScript reimplementation, kept because
                      it plays in a browser. It is not this.

## Playing

Answers are typed and entered; a blank line is EXECUTE. The line in brackets is
the workstation's prompt line, which is where the game does its taunting.

Opening moves, if you want to see it work: `n`, `r`, `k`, `f`, `dig`, `b`,
EXECUTE, `ankle`, EXECUTE, EXECUTE — that reaches the atrium with seven doors.
Door 2 is a room with a word painted on the wall, and the answer is what the
word means.
