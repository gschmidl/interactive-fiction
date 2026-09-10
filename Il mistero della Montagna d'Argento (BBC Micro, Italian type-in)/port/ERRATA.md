# Errata — "Il mistero della Montagna d'Argento" (Italian Usborne edition)

`montagna_libro.bas` is the listing exactly as printed on book pages 18–27.
`montagna.bas` is that file with the changes below applied, and is what goes on
the disc. `patch_errata.py` applies them; `errata.diff` is the resulting diff.

**As printed, the listing cannot run on a BBC Micro.** It stops at line 4480
before the first room is ever displayed. Every change below is either a
demonstrable typo or a typesetting artefact; no game text, number, room, object
or puzzle has been altered.

## A. Typographical errors in the book

| Line | Printed | Should be | Evidence |
|---|---|---|---|
| 910 | `LET R$="NON SAI NUOTARE` | `…NUOTARE":RETURN` | No closing quote and no `:RETURN`. The line ends at x=755px while the page's text block runs to x=973px, so it is genuinely short, not clipped. Every neighbouring line in the block ends `:RETURN`. |
| 2060 | `LET R$:"NON E' ATTACCATO…` | `LET R$="…` | Colon printed for `=`. |
| 2090 | `…:LET F(40` | `…:LET F(40)=0` | Line ends mid-statement at x=887px, well short of the 973px margin, so it is not a print clip. |
| 2100 | `AND F(53)=1 T LET C(14)…` | `…=1 THEN LET…` | A bare `T` where `THEN` belongs. Measured cell count (78 chars) matches `T`, not `THEN` (81). |
| 2250 | `LET R$:"NON SEI ABBASTANZA…` | `LET R$="…` | Colon printed for `=`. |
| 2260 | `LET E$(37)="EW"` | `="EO"` | English E/W left untranslated. Line 1070 tests `E$(37)="EO"`, and the movement code at 1120–1150 only understands N/E/S/**O**/A/B, so `"EW"` makes the passage it opens unusable. Line 3030 correctly writes `E$(27)="EO"`. |
| 2990 | `AND R(49) THEN` | `AND R=49 THEN` | `R` is the scalar room number; `DIM` at 3380 declares `C`, `E$`, `F`, `G$` only, never `R`. |
| 3300 | `LET R$=S4$+RIGHT$…` | `LET R$=X4$+…` | `S4$` is never assigned anywhere in the program. `X4$` is set at 3490 and is exactly the "the magic words are found…" sentence this line completes. |
| 4520 | `…INT (RND (1)*4)*3,1)` | `…*4)*4,1)` | `B$` holds four-character verb slots, so the four directions N/E/S/O sit at characters 1, 5, 9, 13. With `*3` the index lands on 1, 4, 7, 10 = `N,?,?,?`; `L$` then never gets assigned and 4580 dies with *No such variable*. With `*4` the generated maze code and its mirror come out as a matched pair (verified: `G$(1)="EOOSNONE"`, `G$(2)="OSESNEEO"`). |
| 4730 | `INPUT# 1,G$(1)` | `INPUT#X,G$(1)` | The channel is `X`, opened at 4690. Channel 1 is not open. |
| 4810 | `PRINT# 1,G$(1)` | `PRINT#X,G$(1)` | The channel is `X`, opened at 4770. |

## B. Typesetting artefacts that stop BBC BASIC tokenising

The compositor set a space between a function name and its opening bracket.
For most functions that is harmless, but four cannot take it:

| Line(s) | Printed | Should be | Why |
|---|---|---|---|
| 560 | `ON VBGOSUB 800,…` | `ON VB GOSUB 800,…` | BBC BASIC tries to match a keyword only at the start of an alphanumeric run; finding none at `VBGOSUB` it skips the whole run, so `GOSUB` is never tokenised. (580/600/620/640 print the same way but *do* work, because a digit precedes `GOSUB` there and that restarts a word.) |
| 230, 310, 320, 360, 370, 1110, 3300, 4270, 4360, 4850, 4860, 4880 | `MID$ (` `LEFT$ (` `RIGHT$ (` | `MID$(` `LEFT$(` `RIGHT$(` | In BBC BASIC each of these is a **single token that includes the opening bracket**. With a space they stay as plain text and the program fails with *No such variable*. Verified on a real BASIC 2 ROM: `PRINT MID$(A$,2,3)` → `ELL`; `PRINT MID$ (A$,2,3)` → *No such variable*. |
| 4480, 4490, 4520 | `RND (1)` | `RND(1)` | `RND` also has a valid no-argument form, so `RND (1)` parses as bare `RND` followed by a separate `(1)`: `PRINT RND (1)` prints two values, and inside an expression it gives *Missing )*. This is what stops the printed listing at line 4480. |

The book itself writes `TAB(` with no space (lines 4410/4420), which is the same
rule. `LEN (`, `INT (`, `STR$ (`, `VAL (`, `ASC (`, `CHR$ (` all take a mandatory
argument, so the space is harmless and they are left exactly as printed —
each was checked on the real ROM.

## C. Playability fix (deliberate, one line)

**Object 74 duplicates object 72.** Line 4110 reads
`DATA GRARG,PORTA,SVEGLIARE,GUIDARE,PROTEGGERE,GUIDARE,AIUTO,COFANO,ACQUA`,
so nouns 71–75 — the five magic words — are:

```
71 SVEGLIARE   72 GUIDARE   73 PROTEGGERE   74 GUIDARE   75 AIUTO
```

The third magic word is object `F(52)+73`, and 4490 sets `F(52)=INT(RND(1)*3)`,
i.e. 0, 1 or 2 with equal chance. But the noun scanner at 400–420 stops at the
**first** match (`LET B=I:LET I=NO`), so typing `GUIDARE` always yields B=72 and
B=74 is unreachable. **When F(52)=1 the game cannot be won**: line 1940 never
fires, so F(62) never becomes 1.

This is a translation defect, not a typo — presumably two distinct English words
(GUIDE / GUARD or similar) both became *GUIDARE*.

**Applied fix — line 1940 only**, taking the printed vocabulary at face value:
when F(52)=1 the third magic word simply *is* GUIDARE that game, so 1940 accepts
B=72 in that case. No vocabulary is invented, no DATA is touched, and all three
variants stay in play.

```
-1940 IF B=(F(52)+73) AND F(60)=1 AND F(61)=1 THEN LET F(62)=1:RETURN
+1940 IF (B=F(52)+73 OR (F(52)=1 AND B=72)) AND F(60)=1 AND F(61)=1 THEN LET F(62)=1:RETURN
```

Line 1930 still claims B=72 first while F(61)=0, so the *second* magic word is
unaffected. Verified in the emulator, standing in room 47 holding the Stone with
F(60)=F(61)=1:

| F(52) | PROTEGGERE | GUIDARE | AIUTO |
|---|---|---|---|
| 0 | **wins** | sbagliata | sbagliata |
| 1 | sbagliata | **wins** (was: unwinnable) | sbagliata |
| 2 | sbagliata | sbagliata | **wins** |

If you would rather have the disc reproduce the defect, delete the `PLAYABILITY`
block from `patch_errata.py` and rebuild.

## Verified on a real emulated BBC Model B (BASIC 2 + Acorn DFS 1.2)

Boot from `MONTAGNA.ssd` with SHIFT+BREAK; all of the following were exercised:

* title screen, menu, new game — starts at room 77 *AD UN INCROCIO*, exits E,O
* movement 77→78→79→80 matching the exit DATA exactly
* room contents, hidden objects (`ESAMINA PENTOLA` → `AHA!` reveals the coins)
* `PRENDI` / `INVENTARIO`
* the Caesar-shift decoder at 4260 (`UN TROLL TI BLOCCA IL PASSAGGIO`)
* word-wrap at 4830 against the 40-column screen
* save and restore to disc (`REGISTRARE`, then menu option 2)
* ~68 consecutive turns with no *No room* error (TOP=&64C6, HIMEM=&7C00,
  leaving 5946 bytes for variables)
