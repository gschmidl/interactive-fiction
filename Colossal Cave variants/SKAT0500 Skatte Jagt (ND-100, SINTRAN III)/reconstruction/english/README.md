# Skattejakt in English

`../port/data/SKATTEJAKT-EN.PROG` is `SKATTEJAKT:PROG` with its Norwegian replaced. No instruction
changes, and `skattejakt.exe` runs it as it is (`run-english.bat`). A game suspended in it is an
English dump.

| file | what |
| --- | --- |
| `messages.no.txt` | every message as extracted from the image, keyed by its place in the text array. 7-bit: `[ \ ] { \| }` are Æ Ø Å æ ø å |
| `messages.en.txt` | the translation, with the same keys. Edit this and rebuild |
| `vocab.en.txt` | the English word table, by the game's own type and meaning numbers |
| `build.py` | makes the image; the banner, formats and word literals are in here |
| `check.py` | checks it against the sessions recorded on real SINTRAN |
| `deepcheck.py` | checks it against the original in every room |

## How the text is stored

Octal addresses, 3 words to an element, as in the port's `NOTES.md`.

- **Messages.** `LINES(I)` is at 071534 + 3\*I, and I starts at 1. Each line of text starts with an
  element holding the index of the next line as a 48-bit REAL (exponent word 040000+bits, value in
  the top of the mantissa), negative on a message's first line; the text follows, 6 characters to
  an element, XORed with 020040 045440 020040, the key the word table uses. An element of -1 ends
  it all. The array has room for 11,314 elements. The Norwegian used 11,302.
- **Pointers into it.** Single words: STEXT 061257, CTEXT 062457, LTEXT 062655, the magic messages
  and PTEXT 067216, RTEXT 071145. Object states have no pointers: the game walks from the object's
  first message, so their order is kept.
- **Words.** KTAB 064114 (type\*1000+meaning) and ATAB 064732 (XORed like the text), indexed from 1,
  400 at most, ended by -1. The parser takes the first match, so `vocab.en.txt` keeps the order of
  types.
- **The banner**, at 036763, is built like the messages with its own indices. Its records keep their
  sizes.
- **FORMATs** sit in the code and keep their length, blank padded.
- **Word literals** the program compares with are A6 variables near 022230, in the yes/no routine at
  031075, and `BOW` at 041317. `ODIN` and `SLEIPN` are left as they are.

English came out longer than the array allows, so rooms with the same words (the twisty mazes) share
one copy of the message. That brought it to 11,216 elements. Only rooms may share; object messages are
found by walking, so they may not.

## Choices in the translation

It translates this game, not back to Crowther and Woods, though it borrows their phrasing wherever
the meaning is the same. So it stays metric, the Badger Room stays (Grevlingrommet, the translator's
Bedquilt), the plover stays a roc, and the bear sign stays *The bear improves on closer acquaintance*.
The pirate spoke nynorsk; here he speaks pirate. The riddle is unchanged. The internal notes in
`>$<` and `*` lines are kept or translated, and are never shown.

The Norwegian parser turned `LÅS OPP` ("unlock") into opening. English has `UNLOCK`, and `LOCK UP` must
not open things, so that rule now turns `OPEN` into `OPEN` and does nothing. `NÆRM` on the flying carpet
became `CLOSER`.

## How it was checked

`python check.py` first follows every text pointer in the tables to its English message. Then it takes
the seven recorded gameplay sessions, puts their commands into English by the meaning of each word, and
plays them on the English image. The recorded Norwegian transcript, translated message by message, must
come out exactly. The game's random numbers are dumped with it, so the same meanings give the same game.
All 7 sessions agree, 4,439 messages and lines. The `keys_` sessions are left out, as they test the terminal.

`python deepcheck.py 80 5` suspends both images with the lamp lit, pokes the saved game into each of the
184 rooms and types the same random commands into both, with 5 seeds. All 920 runs agree.

Up to 294 of the 759 messages were not seen in those runs (the count goes by first line, so it includes
repeats like the maze rooms), and are only proofread and pointer-checked. Most are object states and rank
messages that need a real game to bring about. These were played by hand and
agree with the Norwegian: the princess up to the riddle, the wizard's questions, `HOURS`, saving and
resuming. The flying carpet's lift-off (#10282) and its `HIGHER`, `LOWER` and `CLOSER`, and winning the
riddle (#10251), have never been played. How to answer the riddle is not known: the original answers
`ODIN SLEIPN` with *To ord!!!* just as the English says *Two words!!!*.
