# Porting ZENO

## What the game needed

`ZENO EXEC` is 2,594 lines of REXX — REX, in 1983 — and it will not run on
its own. It needs three things from VM/CMS:

* **IOS3270**, an IBM full-screen panel handler (5785-HAX, written at IBM
  Uithoorn by Bert Wijnen and Theo Alkema). Every screen in the game is one
  of its panels. It draws the boxes, it owns the function keys, and it is
  what makes the room description, the console, the editor and the books
  appear at all.
* **EXECIO**, to move the CMS program stack to and from a file. That is
  `SAVE` and `RESTORE`, and it is also how the starting world is loaded.
* **STATE** and **ERASE**, to test for and delete a file.

IOS3270 itself survives, but only as an S/370 `MODULE` — machine code for a
mainframe, and a licensed IBM program product besides. So it had to be
reimplemented. That turned out to be tractable, because the author's own
package includes `IOS3270 IOS3270`: the product's online help, which
documents the whole panel language.

## How the port works

`Zeno.exe` is the mainframe. It hands `ZENO EXEC` to an embedded Regina
REXX and answers the commands the exec issues.

```
  ZENO.EXEC ──► Regina REXX ──► 'IOS3270 ZENO IOS3270 * ;ROOM (NOCLEAR NOQUIT'
                    ▲                          │
                    │                          ▼
              RexxVariablePool  ◄──────  cms.c ─► panel.c ─► screen.c
              (reads &rmsg.1,             the CMS   the IOS3270   a 3270 on
               writes IOSD, IOSK)         commands  panel engine  the console
```

The join is exact, not an approximation. On VM, IOS3270 reached into the
running exec's variables through **EXECCOMM** — it read `&rmsg.1` to draw a
line and set `&IOSD` to say which key you pressed. Regina exposes the same
interface as `RexxVariablePool`, so the C side can do literally what the
1983 module did. That is why the exec needs no modification to run: it is
not being emulated or translated, it is being *hosted*.

`EXECCOMM` is still there in the module's binary, incidentally — it is one
of the few legible strings in it.

* `src\main.c` — loads the exec, registers the subcommand environment,
  starts Regina.
* `src\cms.c` — the command handler: IOS3270, EXECIO, STATE, ERASE, SUBSET,
  plus the variable-pool wrappers.
* `src\panel.c` — the IOS3270 panel engine.
* `src\screen.c` — a 24×80 3270 on a Windows console screen buffer.

## Decoding IOS3270

Zeno uses ten of IOS3270's thirty-odd function characters: `.h` intensify,
`.s` space, `.c` field characters active, `.n` do not edit input, `.y`
return input on PF keys too, `.j` redefine field characters, `.b` bottom
title, `.f` PF key values, `.i` imbed another panel, `.l` skip to line.
Options: `NOCLEAR`, `NOQUIT`, `NOREAD`, and a cursor address.

### The field-definition characters

This was the one genuinely ambiguous part. The help file prints its default
table as ASCII art:

```
¢.j-%$^#¢@!.              " " = leave as-is, "&" = reset.
    |||||||||'-input, autoskip.
    ||||||||'-'fill' character for input fields.
    |||||||'-variable place holder.
    ...
    '-intensify.
```

Ten legend markers, but only eight characters visible. The resolution is
that the `¢` characters are attribute bytes, and **a 3270 attribute byte
occupies a screen position**. Counting columns with that in mind, the
markers land at columns 5–14 and the characters at 5, 6, 7, 8, 9 (the `¢`
itself), 10, 11, 12 — which aligns them exactly:

| char | | char | |
| --- | --- | --- | --- |
| `%` | intensify | `@` | pen select |
| `$` | input | `!` | pen select, intensify |
| `^` | input, intensify | `.` | variable place holder |
| `#` | input, intensify, autoskip | `_` | fill character — *initially unset* |
| `¢` | input, non-display | `\|` | input, autoskip — *initially unset* |

Slots nine and ten being unset is what lets the author draw his boxes out of
`_` and `|` without every border becoming an input field.

Zeno's one `.j` line is `.j` followed by eight blanks and a `$`. The first
blank is the separator, so it changes slot eight only: the **variable
place-holder** moves from `.` to `$`. That is why `&rmsg.1` reads as one
stem name instead of `rmsg` followed by a place-holder and a `1`.

**The check that settles it.** The editor is invoked with the option
`0025`, documented as "cursor at the *cc*'th input field". Under this table
`;EDITOR` has exactly twenty-four input fields — twelve program lines and
twelve line-command boxes — which makes the command line the twenty-fifth.
No other reading of the table produces that number.

### Columns: elastic blanks

A panel line mixes literal text, fields, and `&variable` references whose
values are almost never the width of the reference. Pure left-to-right
flow breaks the boxes; pure column alignment makes consecutive variables
overwrite one another. What works, on every panel in the file, is:

> A run of literal blanks always renders at least one blank, and otherwise
> stretches or shrinks so that whatever follows lands on its own source
> column — if the cursor has not already passed it.

So ` &tt &head&timenow` flows naturally when the time is eight characters
wide in a three-character reference, while `|  &vduline.1$` followed by
fifty-one blanks and a `|` still closes its box after a sixty-character VDU
line has been dropped into an eleven-character reference.

There is corroboration for this in the game's own source. `VDUSHOW` has:

```rexx
if substr(vduline.i,1,2) = hi then
  vduline.i = vduline.i||lo||'  |'/* this is a nasty fiddle ! */
```

`hi` and `lo` are `'1DE8'x` and `'1D60'X` — raw 3270 Start Field orders,
protected-intensified and protected-normal. The author is closing the
highlight and hand-padding the line so that his own `|` lands exactly on
the box edge, because the two attribute bytes have each eaten a column.
The fiddle only makes sense if columns are aligned, and it comes out right
here. (The engine suppresses the duplicate border it would otherwise draw.)

### Deliberate divergences

Two, both small, both to preserve the author's text rather than change it:

* **`!` in prose.** `!` is the pen-select-intensify character, and the book
  pages contain eight literal exclamation marks — "Be warned!", "stops you
  suffocating!)". A field character here opens a field only when a letter,
  digit or `&` follows it, which keeps the punctuation and still honours
  `%same`, where the attribute byte itself supplies the space between two
  words.
* **The start-up panel is held for 1.5 seconds** (`-fast` turns this off).
  It says "Things are just being set up - won't be long", which on a 1983
  mainframe was true; here loading takes nine milliseconds.

### Things that are faithful and look wrong

* The notebook cover's title field is `$40&bktitle2` inside a box thirty-nine
  columns wide, so it overhangs the right border by a couple of characters.
  That is in the panel file; no reading of the field width makes it fit, and
  pulling the border back inside the field would corrupt what you type.
* `Page  &curpage   of &lastpage` on the book screen has no field character
  in front of `&curpage`, so the page number is not typeable, even though
  `BOOK` checks it with `badint` as though it were. Contrast `;TRANS`, where
  the author *did* write `^10&mtfrom` for the fields he wanted typed. It
  looks like an oversight in the original. Paging works on PF7/PF8/PF2.
* Small gaps after short values — "the game is over after 118      moves" —
  are what an eight-column `&timenow` reference does with a three-digit
  number.

## Changes to the exec

Four, in `ZENOFIX.diff`, applied by `src\makefix.py` and used by default;
`-original` runs the untouched file. Briefly:

1. **A REXX error no longer ends the session.** `SIGNAL ON SYNTAX` was armed
   once and never disarmed, so any interpreter error killed the game. The
   handler now cancels the offending program through the author's own abend
   path and re-enters the main loop with the world intact. It also drops the
   clock entry that was firing — `CLOCK` removes it only after the call
   returns normally, and without that the same faulty line is re-dispatched
   every tick.
2. **`EVAL` guards division by zero,** the way it already guards every other
   expression it cannot work out. This is the crash you hit first: put
   `zz = 1 / 0` in a program and the original gives you "Error 42 occurred at
   line 1286" and throws you out.
3. **`EVAL`: `a` should be `opa`.** A one-letter typo, on the line that
   handles a non-numeric comparison, which makes `IF (ANS = KILL)` compare
   the letter `A` against `KILL`.
4. **`SAVE` stops indenting the file.** Records were written as `queue bl cr`
   with `bl = ' '`, and read straight back, so every save/restore cycle moved
   every program line and book title two more columns right. Three round
   trips and the LIFT program is six columns in. Fixed in the writer, not
   the reader, because `ZENO INITDATA` — which the same routine loads — has
   no such prefix and has lines that begin with real blanks.

The reported crash on "function keys not designated for editing" does not
occur here: an unmapped PF key returns an empty `&IOSD`, which every panel
in the game already handles.

## Build

Needs MinGW-w64 gcc and `regina.dll`. From `src\`:

```
make            # -> ..\Zeno.exe
```

`libregina.a` is an import library generated from the shipped DLL with
`gendef` and `dlltool`; `rexxsaa.h` is Regina's own header. Two things in
it are worth knowing, because both fail silently: `RexxAddQueue` and
`RexxPullQueue` reject a NULL queue name and want `"SESSION"`, and the
`RXQUEUE_NOWAIT` / `RXQUEUE_WAIT` constants are documented the wrong way
round — `1` is the one that does not block.

## Testing

* `make harness.exe` renders any panel to text without REXX:
  `harness ..\game ";EDITOR"` also lists the fields it found, which is how
  the twenty-five-input-field check above was made.
* `Zeno.exe -script FILE` replaces the console with a transcript. Script
  lines are `t TEXT` (type into the cursor field), `f N TEXT` (type into
  the *n*th input field), `k enter|pfN|tab|esc`.
* `Zeno.exe -script FILE -verify` uses the real console and reads the screen
  buffer back out, characters and colours, so the Win32 painting path is
  checked rather than assumed.
* `src\fuzz.py` plays random sessions in the room, at the console and in the
  editor, and greps for the syntax trap.
