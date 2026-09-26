# BURGLAR HOUSE — NEC PC-8001

Recovered from a magazine type-in listing (scan supplied by the user).

| | |
|---|---|
| Title | BURGLAR HOUSE — Adventure #0 |
| Dated | 1983/ 4/19 |
| Credited | Copyright 1983 by T.SHIMAYA |
| Machine | NEC PC-8001, N-BASIC + Z80 machine code |
| Listing | PDF page 2 = BASIC loader + two MZ variant blocks; pages 3–5 = the PC-8001 dump |

You are a burglar working a mansion between **PM 10:00** and **AM 6:00**. The whole game
is Z80 machine code; BASIC only sets the screen up, binds the ten function keys to the
commonest commands, and calls `USR(0)`.

## Files

* `Burglar House.bin` — the machine code, **8704 bytes, loads at &H9000**, entry &H9000
* `Burglar House.bas` — the BASIC loader exactly as printed (lines 10–230)
* `Burglar House.hex` — the transcription, one line per 16 bytes, in the magazine's own layout
* `work/pages/*.hex` — the per-strip transcriptions it was assembled from
* `src_original/` — the source PDF

## Running it

Load `Burglar House.bin` at `&H9000`, then RUN the BASIC loader. The loader's
`CLEAR 100,&H8FFF` is what keeps BASIC from stepping on the code, so it must run *after*
the block is in memory.

In Takeda, use the CPU debugger. Rename "Burglar House.bin" to "debug.bin", put it in the
emulator folder, and L 9000 in the debugger, then Q. Load the loader tape and RUN.

The program checks itself on startup (see below). If it prints a `9x00 - 9xFF` range and
"Push any key", the block in memory is wrong — not the transcription.

## Verification

**The game verifies its own dump, and this transcription passes.**

The routine at `9100` sums each 256-byte page from `9100` to `AFFF` and compares the low
byte against a 31-entry table at `A140`. On a mismatch it prints the offending range and
refuses to start — this is the checksum the article describes, and it is why the article
warns that `A140`–`A160` must be typed especially carefully: that range *is* the table.

* **31 of 31 pages match** → bytes `9100`–`AFFF` (7936 of 8704) are proven exactly right.
* The check found a real error: at `92ED` a `ﾗ` (`D7`) had been read where the listing has
  `ﾄ` (`C4`) — "ｵﾜｯﾀﾄｷﾊ". The page was off by exactly `0x13`, which is `D7 - C4`.

The remaining 768 bytes (`9000`–`90FF`, `B000`–`B1FF`) lie outside the game's own check
and are verified structurally instead:

| structure | check | result |
|---|---|---|
| noun table `A161` | count byte says 40 groups of 3-char words | ends exactly at `A214`, the verb count byte |
| verb table `A214` | count byte says 39 groups | ends exactly at `A302`, where the room names start |
| verb dispatch table `9650` | one entry per verb | 39 entries, all inside `9000`–`B1FF`, ending exactly at `969E` — and `969E` and `96A0` are themselves entries in it |
| object names `A4ED` | one per noun | 40 names; **each noun is the 3-character prefix of its object name, 40/40**, with the two tables in opposite order |
| object records | 5-byte records, sorted | 130 records, field 0 non-decreasing `00`..`28`; 99 of them lie in the unchecked pages `B000`–`B1FF` |
| init block | `9158` does `LDIR` from `AE4D`, `BC=03A6` | ends at `B1F3`, followed by exactly 13 bytes of zero padding to `B1FF` |

## The trailing byte is NOT a checksum

Every printed line ends `:xx`, and `xx` is simply the **last data byte of that line repeated**
— 544 of 544 lines. It catches a misread final byte and (because a dropped byte shifts
everything left) most dropped bytes, but it is blind to an error in the middle of a line.
That is what the runtime page checksum is for. `bhcheck.py`, a working script
kept out of the repository, applies the line rule plus a strict 16-bytes-per-line and address-contiguity check; it was the
16-byte rule that caught two dropped bytes during transcription (`A680`, `9E20`).

## What is in the dump

```
9000-90FF  entry vectors, work area, "Tape read error"
9100-915F  self-check, then LDIR the initial game state to B200
9160-9300  title screen and instructions
9300-95FF  main loop, parser, printing (9589 = print the inline string that follows)
9600-9FFF  the verb routines
A140-A160  the page-checksum table
A161-A301  vocabulary: 40 noun groups, then 39 verb groups, 3 characters per word
A302-A4EC  62 room names
A4ED-A5AC  40 object names
A5AD-ABFF  74 adjectives and the message text (diary entries, deaths, endings)
AC00-B1F2  data tables + the initial object state copied to B200
```

Text is halfwidth katakana (JIS X 0201). Three characters are control codes in the
message strings: `5E` = newline, `5C` = set the print delay from the next byte,
`40` = splice in message *n*−0x1F from the table at `A8CF`.

## Things worth knowing

* **The parser compares exactly 3 characters.** Longer words are simply truncated, so
  `ｶｲﾀﾞﾝ` is stored as `ｶｲﾀ` and `ﾆﾝｷﾞｮｳ` as `ﾆﾝｷ`. Synonyms are grouped: `ﾈｺ`/`ﾆｬﾝ`,
  `ﾀﾍﾞﾙ`/`ｸｳ`/`ｼｮｸ`, `ｻｹﾌﾞ`/`ﾄﾞﾅﾙ`, `ﾀｽｹﾙ`/`HELP`.
* **`ｼﾒﾙ` and `ｱｹﾙ` each appear twice** in the verb table (entries 30–33). This is really in
  the data — the page it sits on checksums correctly — so it is the author's, not a
  transcription slip.
* **Commands are noun-then-verb**, or a bare verb; the instructions screen says so.
* `LOAD` and `SAVE` are verbs, so the game has its own save.
* The two short listings on PDF page 2 (`p02_mz80kc.hex`, `p02_mz80b.hex`) are tape-I/O
  replacements for the MZ-80K/C and MZ-80B and are irrelevant to the PC-8001 build. They
  *are* checksummed with a real sum-of-16-bytes rule, and both verify 10/10 — which is how
  the checksum question was settled in the first place.

## Also in this folder

* `loader.cmt` — the BASIC loader as a PC-8001 tape (10x `D3`, the 6-byte name `BASIC`,
  then the tokenised N-BASIC program). Load it after the binary is in memory, then RUN.
* `pc8001.sta0` — a Takeda emulator save state with the game already loaded and running.

Status: **finished**. The dump reproduces exactly, the game accepts its own checksums, and
it runs.
