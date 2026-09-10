# Il mistero della Montagna d'Argento (BBC Micro)

Italian Usborne type-in adventure — *The Mystery of the Silver Mountain* by
Chris Oxlade and Judy Tatchell, translated by Paolo Agostini. The listing runs
on book pages 18–27, with per-machine conversions on pages 28–30.

Recovered from a scan of the book (`src_original/`) and turned into a
**self-booting BBC Micro disc image**.

## Play it

`port/MONTAGNA.ssd` — mount in drive 0 and press **SHIFT+BREAK**.

* **BeebEm**: File ▸ Load Disc 0…, then SHIFT+F12 (or File ▸ Reset with SHIFT held)
* **jsbeeb** (bbc.godbolt.org): drag the .ssd in, or `?disc=…&autoboot`
* **b2 / BeebEm / MAME `bbcb`**: any BBC Model B with a DFS

Model B, 32K, Acorn DFS. Nothing else is required.

### Playing

Two-word commands in Italian, e.g. `PRENDI PANE`, `ESAMINA PENTOLA`,
`DIRE SVEGLIARE`. Directions are **single letters**: `N` `E` `S` `O`
(ovest = west), `A` (alto = up), `B` (basso = down) — the parser pads a verb to
four characters, so `EST` will not work but `E` will.

`INVENTARIO` lists what you carry. Typing `REGISTRARE` at the prompt saves the
game to the disc under a name you choose; menu option 2 reloads it.

You start at a crossroads and are looking for the Pietra del Destino. There are
three magic words, spoken one at a time with `DIRE` while holding the Stone in
the silver chamber; the library tells you where the third one is.

Several puzzles are **book-and-screen**: the screen gives you the clue and the
printed pages tell you what it means. The magic words themselves are planted in
the prose on pages 4–7 ("il potere di *guidare* il destino", "un'invocazione
d'*aiuto*"), and page 32's hints are mirror-printed so you can't read them by
accident. The neatest one: examining the inscriptions in the Goblin Cemetery
(room 2) reports `UNA PAROLA SBIADITA: 'P L DI'` — a word with letters worn
away. It is **PALUDI**, the marshes: room 11, where the reeds (object 10) lie.
Carry them to the fallen oak (room 56) and `SUONARE CANNE` frees the Goblin
guardian's spirit and opens the secret path (line 2900, `H=5610`). Page 13 sets
it up — the Goblin King left his subject a riddle nobody has solved — and the
mirrored hint for *Quercia divelta* confirms it: *"aspetta di sentire il suono
del vento che soffia tra le canne della palude dov'è nato."*

## Why the BBC

The book's base listing *is* the BBC version — it uses `CLS`, `TAB(`,
`OPENIN`/`OPENOUT` and `INPUT#`, and the conversions on pages 28–30 replace
exactly those for the other machines. The BBC section says only
"*Puoi saltare le parole LET, se vuoi*", i.e. no changes at all.

That said, as printed the listing does **not** run: it dies at line 4480 before
the first room appears, and there are ten further typos. See
[`port/ERRATA.md`](port/ERRATA.md) for the full list and the evidence for each.

## Layout

```
src_original/
  Mistero della montagna d'argento (Italian).pdf   the scan the listing came from
port/
  montagna_libro.bas   the listing exactly as printed (does not run)
  montagna.bas         the runnable version = _libro + ERRATA
  errata.diff          diff between the two
  ERRATA.md            every change, with the evidence for it
  patch_errata.py      derives montagna.bas from montagna_libro.bas
  tokenise.mjs         text listing -> BBC BASIC tokens, using the real BASIC ROM
  build_ssd.py         tokens + !BOOT -> self-booting Acorn DFS image
  play.mjs             headless test-drive of the finished disc
  montagna.tok         tokenised program (19398 bytes)
  MONTAGNA.ssd         >>> the deliverable <<<
```

## Rebuilding

Needs [jsbeeb](https://github.com/mattgodbolt/jsbeeb) for both the tokeniser and
the test harness — it drives the genuine BBC BASIC ROM, so tokenisation is
exactly what a real machine would produce:

```bash
git clone https://github.com/mattgodbolt/jsbeeb && (cd jsbeeb && npm install)
export JSBEEB=$PWD/jsbeeb

cd port
python patch_errata.py                        # montagna_libro.bas -> montagna.bas
node tokenise.mjs montagna.bas montagna.tok
python build_ssd.py                           # -> MONTAGNA.ssd
node play.mjs "1" "E" "E" "E" "PRENDI PANE" "INVENTARIO"
```

## How the transcription was checked

The scan is only ~150 dpi and its OCR text layer is unusable (`ANO` for `AND`,
`MIO$` for `MID$`, `!$` for `I$`), so the listing was read from the page images
and then verified structurally rather than by eye:

* **498 lines**, numbers strictly increasing; 2330 is the only gap in the
  10-step sequence and nothing branches to it.
* **Every** `GOTO`/`GOSUB`/`THEN` target — including all 58 entries in the five
  `ON …` dispatch lines — resolves to a line that exists.
* DATA counts land exactly on the constants in line 10: **80** room
  descriptions, **88** nouns (`NO`), **80** exit strings, **28** object
  locations (`G`), **13** initially-hidden objects.
* The verb table B$ across lines 3420/3430/3440/3445 splits into **57**
  four-character slots = `NV`, and the slot numbers agree with all five `ON`
  dispatch lines (e.g. `RING` is verb 42, matching line 350; `UNLO` is verb 48,
  matching line 355; `RIFL` is verb 57 → 3170, the wizard's room).
* All 77 distinct `H=` constants decompose into a valid (room 1–80, noun 1–88)
  pair — `H` is `VAL(STR$(R)+STR$(B))`.
* All 18 Caesar-shifted strings decode to grammatical Italian under the
  program's own decoder at 4260. This caught a real misreading: line 3470 is
  `TFDPOEJ` (→ SECONDI), not `TFDPEJ`.
* Ambiguous character runs were measured off the page rather than counted by
  eye — the `=` rules in lines 250 and 4430 are **40** signs each (the book
  prescribes 22 for the 22-column VIC and 32 for the 32-column TRS-80, so 40 for
  a 40-column BBC is consistent), line 740 is 5×U 5×R 5×A, line 4310 is 8 S's
  (not 9), and line 4330 has 5 spaces before `TUNNEL`.
* Room and object numbering cross-checks against the code in dozens of places:
  room 29 is the banquet hall with the Grarg (lines 140/150), room 27 the silver
  bell (3010), room 48 the wizard's den (530/3170), room 58 the cobwebs
  (1040/2480), rooms 75/76 the two ends of the bridge (830/1180); object 27 is
  the Pietra del Destino, and its start location C(27)=47 is the silver chamber
  that line 1910 requires you to be in.
