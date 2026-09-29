# Ankh (PROGRAM ANKH1)

A treasure hunt written in PRIMOS Pascal by **Randall Tice** at the College of
William & Mary, January 1984, with minor modifications in December 1984.  The
source banner thanks Dave Montouri, Mike Dullaghan and Chip Roberson, and
credits Dave Montuori with the `GETCOMMAND` procedure that made it, in the
author's words, "a much more user friendly game".

Six treasures, twenty-four rooms, a Myna bird that gives hints, a gremlin that
picks your pocket, and a door marked south that kills you if you open it.

## What is here

    src_original/   the Pascal source and the two data files it loads
    port/           the C port and a native Windows executable

To play, run `port/ankh.exe`.  To rebuild it, run `port/build.bat` (or `make`)
with MinGW gcc on the PATH.

## Recovering the game

`aknh.pas` alone will not run.  `PROGRAM ANKH1(INPUT,OUTPUT,ROADS,LEXICON)`
opens two external files at start-up, and neither was among the recovered
sources:

* **LEXICON** — nine vocabulary lists (treasures, movers, props, places,
  directions, animate, inanimate, nouns, verbs), each terminated by a line
  beginning `XXX`.  The program reads only the first three characters of each
  line, which is why the game accepts three-letter abbreviations.
* **ROADS** — a `FILE OF INTEGER` holding `ROUTES[1..24][1..10]` followed by
  `OBJECT[1..17]`: the whole map and every object's starting room.

Both were still on the emulator's PRIMOS disk pack
(`p50em/disk26u0.600m`).  `src_original/extract_from_primos.py` documents and
performs the extraction: it walks the UFD entries, follows each file's record
chain, and decodes PRIMOS text storage (high-bit characters, DC1 blank
compression, NUL padding).  Run from a scratch directory it reproduces all
four files.

The recovered data cross-checks against the source everywhere the two touch.
The `DIRECTIONS` list fixes the slot order as
`N, UP, N-E, E, S-E, DOWN, S, S-W, W, N-W`, and on that reading every
hard-coded subscript in the Pascal lands where the prose says it should:
`ROUTES[1,9]` is the bookcase on the west wall, `ROUTES[13,7]` is the throne to
the south, `ROUTES[23,9]` is the Sphinx west of the pyramid, and the three
rooms the game calls dark (13, 14 and 24) are exactly the three carrying `-100`
in the slot the main loop tests.

## Verification

The port was checked against the original running under the Prime 50-series
emulator (PRIMOS 23.4.Y2K.R1), by driving both with the same input and
diffing the output.  Five sessions covering 175, 190, 205, 40 and 28 lines --
the opening room, scoring, help, inventory, the bookcase, the wall passwords,
the flashlight, the quicksand, the throne room, the sceptre, death,
reincarnation and quitting -- are **identical line for line**, including the
eight-column integer fields PRIMOS Pascal writes in the score line.

## Original behaviour that was kept

The port is a transliteration, not a repair.  Preserved as found:

* the typos `I'n not sure how to do that.` and `dark, gloomy puramid`
* the one-way doors -- saying `JANUS` in the orchard opens room 4 southward
  and simultaneously closes the way back west
* `MOVE_IT`'s second `CASE`, which re-kills you in the seven fatal cases
* the unreachable `IS AN UNKNOWN OBJECT THAT IS HERE` arm of `GAD_ABOUT`
* `SLOW_DEATH`, which prints no prompt at all while you sink in the quicksand

## Deliberate deviations

Two, both forced:

1. **Blank input no longer crashes.**  In the original, pressing RETURN at the
   `What"ll you do?` prompt sets `LINELENGTH` to 0, `GETCOMMAND` then indexes
   `INLINE[0]`, and PRIMOS raises `ILLEGAL_SEGNO$` and kills the program.  The
   port blank-fills the input buffer, so the same code yields two empty words
   and simply re-prompts.
2. **End of input exits** instead of raising a Pascal run-time error, so the
   executable can be driven from a pipe.

## Notes for a player (spoilers)

* Three-letter abbreviations, but the diagonals are typed `N-E`, `N-W`, `S-E`,
  `S-W` -- typing `southeast` gets truncated to `SOU` and sends you south.
* Verbs are `GO`, `TAKE`, `DROP`, `MOVE`, `SAY`, `HELP`, `INVENTORY`, `SCORE`,
  `LIGHT`/`ON`, `OFF`, `QUIT`.  You *go* directions but you only *move* things.
* `MOVE BOOKCASE` opens the west wall of the starting room -- and closes the
  stairs below the landing until you move it back.
* `SAY FOOL` at the bottom of the staircase opens the brick wall south.
  `SAY JANUS` in the orchard closes it again and opens the way west.
* `SAY MAN` answers the Sphinx.
* Carrying the sceptre stops you climbing out through the hole in the ceiling.
* Treasures score only once carried to the attic.  Asking the Myna bird for
  help anywhere but the first room costs you a point.
