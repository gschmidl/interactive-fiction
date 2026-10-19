# Crystal Cave, 1979 and 1980

The same ten commands, typed into the Tymshare PDP-10 original and into
the Univac port made from it three months later. Full transcripts in
[transcript-pdp10-1979.txt](transcript-pdp10-1979.txt) and
[transcript-univac-1980.txt](transcript-univac-1980.txt).

## Side by side

| | **PDP-10, Oct 1979** | **Univac, Jan 1980 →** |
|---|---|---|
| greeting | `WELCOME TO THE CRYSTAL CAVE ADVENTURE!!` | `Welcome to the Crystal Cave expedition.` |
| room 1 | `YOU ARE STANDING AT THE END OF A ROAD BEFORE A BARN.` | `You are standing before a barn at the Northern end of a road.` |
| | — | `There are well-worn paths in several directions.` *(new sentence)* |
| | — | `A Boy Scout compass is lying nearby.` *(new object)* |
| `LOOK` | reprints the description | `Sorry, but I am not allowed to give more details.` first |
| `SCORE` | `…YOU WOULD SCORE 51 OUT OF A POSSIBLE 500.` | `…(after 9 moves), you would score 50 out of a possible 515 points.` |
| quitting | `DO YOU INDEED WISH TO QUIT NOW?` | `But, but, ... ok, goodbye.` |
| case | upper case only | mixed case |

The compass is the single most telling line. The Univac source's own
revision history reads *"January, 1980. Converted from DecSystem 20
FORTRAN to Univac ASCII FORTRAN, **adding stuff for compass**"* — and
here is the version from before that was added, with no compass in the
starting room and no compass in the object table.

## What else the data shows

Working from the decoded image against the Univac `DATA.TXT`:

* **Maximum score 500 → 515.**
* **The multi-command parser does not exist yet.** The Univac intro
  gains two whole sentences the 1979 image has no trace of: *"Several
  commands may be input on one line, as in `IN. GET LAMP,KEYS. OUT`"*
  and *"If you simply type `AGAIN`, I will repeat your last command
  string."*
* **The credit line was trimmed:** `ORIGINALLY DEVELOPED BY WILLIE
  CROWTHER AT STANFORD` → `developed by Willie Crowther at Stanford`.
* **"Adventure" became "expedition"** throughout — `HOW TO END YOUR
  ADVENTURE` → `how to end your expedition`, and the greeting above.
* **96 short-form room descriptions → 100.** Both versions run out at
  location slot 150, with the same four arena rooms at 146–149 before
  it, so the map did not grow; the descriptions were filled in.
* **Room 150 was rewritten and renamed:** `THE ARMS CHAMBER, WHERE ALL
  MANNER OF WEAPONS AND SHIELDS ARE STORED!` (with a further line
  listing the weapons) → `the repository, where all of the implements
  and paraphernalia of the cave expedition are stored`.
* Smaller polish everywhere: `SINK HOLE` → `sinkhole`, `THERE ARE
  INSTRUMENTS STREWN ABOUT` → `There are rusted, useless instruments
  strewn about`, `A DIMLY LIT, MUDDY CRAWL` gains an `East-West`.
* **The wizard's magic word, `PHROG`, is in the clear** in the PDP-10
  image. The Univac build moved it behind two undocumented Fieldata
  cipher routines that were never archived with that source.

Measured overlap: **72%** of the Univac database's sentences appear
verbatim in the 1979 image, and **79%** the other way. The remainder is
the rewriting above, not new material.

## Table sizes

The Univac build prints its own on startup. The PDP-10 build cannot —
its database is already parsed into the core image, so the counting code
never runs — hence only one column here.

```
 15213 of  15250 Words of messages        226 of 250 RTEXT messages
  1018 of   1100 Travel options            11 of  12 Class messages
   371 of    400 Vocabulary words           9 of  20 Hints
   150 of    150 Locations                 34 of  35 Magic messages
    90 of    100 Objects                    6 of  10 Special-object travels
    41 of     50 Action verbs
```

## What did not change

The world. Same barn, same pasture, same privy, same wide shaft, same
smelly sinkhole, same pigs, same Hall of the Mountain King, same jade
idol in the arena. The 39 object mnemonics are identical and in the same
order in all three PDP-10 builds and in the Univac source: `KEYS LAMP
SEARS RICK WALLE BRIDG BOAT DAM DOOR GATE KEG KNIFE FOOD BOTTL WATER
WINE AXE SPICE COLUM COLA SHOWE VENDI CRAP BATTE ROPE TOMB TOAD SAND
MIRRO ORCS DWARF BEAR SKELE SPIDE DRAGO DJIN COBOL GIANT ARARI`.

Kurland's conversion was a careful port, not a redesign.
