# UNDERGROUND V1 — native Windows port

Runs Gary Kleppe's 1979 UNDERGROUND on Windows with **no emulator and no
RSTS/E**, by interpreting the recovered BASIC-PLUS source directly.

```
build.bat            (or: gcc -O2 -o underg.exe bplus.c -lm)
underg.exe
```

## Why an interpreter and not a rewrite

The source is the artifact. `UNDERG.BAS` and `LOOK.BAS` here are the recovered
1979 program, unmodified — `bplus.c` reads them as plain text at startup. So:

* **Edit and run.** Open `UNDERG.BAS` in any editor, change a line, save, run
  `underg.exe`. No tokenising step, no recompiling the BASIC, and errors report
  the program's own line numbers (`?Error 5 at line 50`).
* Nothing was translated, so nothing can be mistranslated. A port to QB64 would
  have meant rewriting 320 postfix `IF` modifiers by hand, and the result would
  be a different program that shares the text.

## Typing

Commands may be typed in either case. Most of that is the game's own doing:
line 1001 runs your input through `CVT$$(A$,188%)`, and bit 32 of that flag
upper-cases, so the verb/noun parser was always case-insensitive.

Two prompts skipped that conversion and so were upper-case-only: the Y/N
question (line 3080, `INSTR(1%,"YN",Z$)`) and the password (line 40, whose
`P$<"A" OR P$>"Z"` range test lower-case letters fail, since they sort above
`Z`). Rather than edit the recovered source, keyboard input is folded to upper
case in the interpreter — which is what the original hardware did anyway, the
console being an upper-case-only KSR. Set `BPKEEPCASE=1` to switch that off and
get the strict original behaviour.

`BPTRACE=1 underg.exe` prints each line number as it executes — handy when a
change doesn't do what you expect.

## Files

| File | What it is |
|---|---|
| `UNDERG.BAS` | Main program: parser, verbs, all text, and the world in `DATA` |
| `LOOK.BAS` | Room describer, chained to and from |
| `bplus.c` | The interpreter (single file, ~1200 lines) |
| `build.bat` | One-line build |
| `UNDER?.DAT` | Save files, created at run time — safe to delete |

## Verified against the original

Replaying the same commands on RSTS/E under SIMH and on `underg.exe` gives
line-for-line identical output. Two differences are expected and correct:

* **Case.** RSTS's KSR console shouts in capitals. The game's own text is mixed
  case, which is what you get here — e.g. line 800 really does begin with a
  lowercase "you are on a path…".
* `W` from the building answers "You can't move in that direction" rather than
  `?Statement not found`, because the transcription bug at line 2000 is fixed.

The save files are **byte-interchangeable with RSTS/E**: 2756 bytes of data
padded to 6 blocks, exactly what `CAT` reports on the real system.

## What BASIC-PLUS needed that a modern BASIC doesn't have

These are the things that make this a dialect rather than an old version:

* **Postfix modifiers**, and they chain, outermost last:
  `O%(X%)=X%(0%) IF O%(X%)=-1% FOR X%=0% TO 15%`
* **`THEN` runs to end of line**, or to its matching `ELSE`.
* **String comparison ignores trailing blanks**, so `"N     " = "N"`. The parser
  builds fixed-width 6- and 5-character word fields with `LSET` and then compares
  them to literals; without blank-padded comparison *nothing* matches.
* **`LSET`** assigns into a fixed-length field, keeping the target's length.
* **Numeric truthiness**, hence subtraction as "not equal": `IF X%(0%)-19% THEN`.
* **Integer division truncates.**
* **`GOSUB`/`RETURN` and `FOR`/`NEXT` resume mid-line**, e.g.
  `1220 … GOSUB3080 : IFZ$="N"THEN1000ELSE…`
* **Arrays auto-dimension to (10)** on first use. UNDERG depends on this: `CHAIN`
  clears variables and the chain back from LOOK re-enters at 995, past the `DIM`
  of `A$` on line 970.
* **Virtual arrays (`DIM #n`) are not variables** — they are a static view onto a
  file, belonging to the *program*. `CHAIN` clears variables but not these, which
  is why re-entering at 995 still has the map. Elements are 2 bytes,
  little-endian, packed contiguously with no per-array alignment.
* **Core common** carries the save-file name across `CHAIN`:
  `SYS(CHR$(8%)+name)` puts, `SYS(CHR$(7%))` gets.
* `RND`, `ERL` and `ERR` must be reserved words: a BASIC-PLUS variable is one
  letter plus an optional digit, so `erl-20%` would otherwise read as `E`,`R`,`L`.

## V2 and V3

`bplus.c` also implements what V2 needs — `DEF FN`/`FNEND`, `FIELD`,
`GET`/`PUT … RECORD`, `MAT` assignment, `XLATE`, and `\` statement continuations
— so it can load V2 once that version is runnable. It isn't yet: **V2's
`GROUND.DAT` is still lost**, and without it there is no map.

**V3 has no source at all** — only the message-file printouts survive — so there
is nothing there to run.

To try another program: `underg.exe MAIN.BAS OTHER.BAS` (the first is the
starting program; the rest are available as `CHAIN` targets).
