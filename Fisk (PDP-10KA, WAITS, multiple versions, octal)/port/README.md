# FisK — a PDP-10/WAITS text adventure, running on Windows

*FisK*, by John Sobotik and Richard Beigel, Stanford 1980 (this build carries
in-game news dated 1984). Written in SAIL, saved as a two-segment WAITS core
image, and archived as octal transfer files.

`bin\fisk.exe` plays it. There is no DOSBox, no simh, no separate data file —
the core image is compiled into the executable and a small PDP-10 interpreter
runs it.

```
bin\fisk.exe
```

## How it works

Same approach as the ARCHON and EXPLOR ports — run the original binary rather
than rewrite the game — but nothing is shared with them: different CPU,
different operating system, different language runtime.

Three pieces:

| file | what it is |
| --- | --- |
| `src/cpu.c` | a user-mode PDP-10 interpreter (KA10 instruction set plus the KL10 `ADJBP` that SAIL emits) |
| `src/waits.c` | the slice of the WAITS monitor the game calls: TTCALLs, buffered disk/terminal I/O, `LOOKUP`/`ENTER`, `CALLI` |
| `src/image.c` | the core images, generated from the octal files by `tools/mkimage.py` |

The game's own code is never patched.

### Working out the image layout

The `.DMP` files are 95 616 octal words each. A saved WAITS image starts at
the vestigial job data area, so file word 0 is address `0o74`; the low segment
runs for 56 704 words and the high segment starts at `0o400000`. That split is
confirmed twice over: `.JBSA` (`0o120`) then lands exactly on the canonical
SAIL entry sequence

```
500351/ JRST  500353      ; start
500352/ JRST  500354      ; reenter
500353/ TDZA  1,1
500354/ MOVEI 1,1
```

and the code at `500373` does `HRRZ 16,115` to read the core size out of the
job data area, which only makes sense at this offset.

### Working out the monitor conventions

Everything in `waits.c` was read off the SAIL runtime in the image rather than
guessed, because the two obvious guesses are both wrong:

* **`IN` and `OUT` skip on *error or end of file*, not on success.** At
  `504620` the non-skip return falls straight through into the "set up the
  new buffer" code, while the skip return calls the status checker at
  `502236`, whose failure path prints `Error in option string`-style runtime
  messages. `OPEN`, `LOOKUP`, `ENTER` and `RENAME` do skip on success —
  `503443` treats the non-skip return from `LOOKUP` as "file not found".
* **The SAIL runtime's channel table *is* the buffer ring header.** What
  looks like private state at `26(2)`/`27(2)`/`30(2)` is the monitor's
  three-word header — buffer address, byte pointer, byte count. The runtime
  never writes the byte pointer back before `OUT`, because it never needed to.
* **The byte pointer is left with P=0 and an address of BUF+1**, so the first
  `IDPB` lands on BUF+2 (`504636`: `MOVE 0,27(2)` / `TLZ 0,770077` /
  `HRRI 0,1(1)`). Building it the textbook way, as `POINT 7,BUF+1`, silently
  drops the first five characters of every buffer.

`GETSTS`/`SETSTS` carry a real per-channel status word; `502252` tests
`IO.EOF` (`020000`) separately from the error mask `740000`, which is how the
runtime learns a file has ended.

### Two CPU bugs worth naming

* The floating-point function field is bits `(op>>3)&3`, not `(op>>4)&3`.
  With the wrong shift, `FADR` multiplies and `FMPR` subtracts. The game's
  random-number seeding is `500 + FIX(201.0 * RAN(0))`, so a broken `FMPR`
  produced a subscript of 2 519 763 into a 900-element array and the SAIL
  runtime aborted with `?scalar out of range at user PC 440607` before the
  first prompt.
* `ASH` is an arithmetic shift. Shifting the magnitude and re-applying the
  sign rounds negative numbers the wrong way.

## Where the game's text lives — and the "missing line" error

The archived `FISK.TXT` is the *source* of the message database, and the
running game does not read it. `rjb.jfp/fisk.dmp` was `SAVE`d after the
database had already been loaded: the messages sit in the low segment
(`0o60074`–`0o130073` or so) obfuscated with `XOR 0o64`, which is why
`Rickety House` does not appear anywhere in the octal file in plain text.

`tools/dis.py -x` decodes it:

```
python tools\dis.py -x ..\dump_original\rjb.jfp\fisk.dmp\775 71513 3
```

The loader that reads `DSK:FISK.TXT` is still in the image, at `433645`, but
it is guarded:

```
477770/ MOVE  2,2171
477771/ JUMPE 2,500013     ; flag clear -> keep the database already in core
477772/ PUSHJ 17,433645    ; flag set   -> reload it from FISK.TXT
```

and word `0o2171` is zero in the saved image. That guard is why this port never
touches the text file, and why it never produces

```
BUG: OUR TEXT FILE IS MISSING A LINE.  PLEASE REPORT
```

That message comes from the message-fetch routine at `406365`, which prints it
whenever the index it is handed maps to an empty array slot — i.e. after a
reload from a `FISK.TXT` that does not contain every line the compiled code
asks for. The mainframe run that hit it took the reload path; this one uses
the database the authors baked into the image.

You can take the reload path deliberately:

```
bin\fisk.exe -p 2171=1
```

It reads all 99 393 bytes of `FISK.TXT`, echoes the section names
(`MESSAGES`, `SCORES`, `HEALTHS`, `COOK`, `NAMES`, `DESCRIPTIONS`, `OBJECTS`,
`QUICKOBJECTS`, `READMAZE`, `HELP`) and reaches the first prompt — but the
parser tables it rebuilds no longer match the compiled code, and the game
stops recognising words. It is a diagnostic, not a way to play.

## The two images, and what the two `FISK.TXT` files are for

The three copies inside each dump directory (`652`/`653`/`775`,
`551`/`552`/`775`) are byte-identical, so those are archive version numbers,
not game versions. But the two dumps themselves are **two revisions of the
game**, and the difference is exactly the difference between the two archived
text files:

| | dump | text source |
| --- | --- | --- |
| later | `rjb.jfp/fisk.dmp` | `rjb.jfp/fisk.txt` (922 records) |
| earlier | `3.1/fisk.dmp` | `rjb.1/fisk.txt` (912 records) |

Decoding both embedded databases and diffing them gives the same 27 changes as
diffing the two text files. The later revision adds

* two rooms — **Atoll** and **Shed** — with their descriptions,
* two ocean locations (`619`, `620`),
* message `365` ("You may want to try throwing it somewhere."),
* a rewritten line for the sleeper in the bedroom (`apparently asleep` ->
  `snoring`),

plus about twenty copy-edits (`comming` -> `coming`, `It's` -> `It is`,
`oldfashioned` -> `old-fashioned`, `Waukesha Wisconsin` -> `Waukesha,
Wisconsin`, a missing article in the double-eagle description) and the credits,
which name Richard Beigel and `b.bagel` in the later dump and John Sobotik and
`s.sobotik` in the earlier one.

So the text files are not interchangeable, and each is already inside its own
dump: the loader that reads `FISK.TXT` runs once, at build time, and the
result is what got `SAVE`d. Nothing reads them at run time. `mkimage.py` still
pairs each dump with its own text file, because the monitor offers
`DSK:FISK.TXT` on the reload path described above and handing an image the
wrong revision there would be handing it the wrong game.

```
bin\fisk.exe            # rjb.jfp  (default, later revision)
bin\fisk.exe -v v31     # 3.1      (earlier revision)
```

A 1 500-command random test run shows only the credit lines differing between
the two, but that is a property of the test, not of the images — it never
reached the Atoll or the Shed, and never examined the objects whose
descriptions were reworded.

## Files the game reads and writes

`LOG`, `PHOTO` and `RUNFILE` work: disk files live in the current directory as
plain lowercase `name.ext`.

* `LOG` / `UNLOG` — writes your commands to `fisk.log`
* `PHOTO` / `UNPHOTO` — writes a transcript to `fisk.pho`
* `RUNFILE "name"` — reads commands from `name.fsk`

Host line endings are converted on the way in and out, so these are ordinary
Windows text files.

## Command line

```
fisk [-v NAME] [-t] [-m] [-D file] [-p ADDR=VAL]
  -v NAME   pick a game image: jfp (default) or v31
  -t        trace every instruction to stderr
  -m        trace monitor calls to stderr
  -D file   dump the 256K-word core to file on exit
  -p A=V    poke octal word V into octal address A before starting
```

## Building

```
build.bat
```

Needs gcc (MinGW, or the one in Strawberry Perl). `make` works too. Python 3
is needed only to regenerate `src/image.c` from the octal transfer files:

```
python tools\mkimage.py src\image.c ^
    "jfp=..\dump_original\rjb.jfp\fisk.dmp\775,..\dump_original\rjb.jfp\fisk.txt\637" ^
    "v31=..\dump_original\3.1\fisk.dmp\775,..\dump_original\rjb.1\fisk.txt\551"
```

## Deliberate differences from the mainframe

* Terminal output is flushed before the game blocks for input. SAIL leaves a
  prompt sitting in its output ring and only hands it to the monitor when the
  ring fills, so without this the first 640 characters of the game would
  appear all at once, several moves late. The monitor peeks at the ring
  instead of the game being changed.
* `INBUF` reuses the ring it already built for a given header. The game
  re-opens a scratch channel once per move, and carving a fresh ring out of
  `.JBFF` every time would exhaust core after a few hundred moves.
* `MSTIME` ticks in real milliseconds and yields the CPU while the game spins
  on it. FisK pauses by busy-waiting on the clock (`400376`), so second-
  granularity would turn every short pause into a full second of burnt CPU.
* Devices other than `TTY` and `DSK` report "not available". The game probes
  `SX:<GAMES.SPY` once a move — a Stanford-local facility — and copes.
* Terminal output is assembled a line at a time rather than streamed, so a
  bare carriage return returns the carriage and what follows overprints,
  the way it did on the real thing. There is exactly one bare CR in the
  message database: the envelope on the letter in the mailbox draws its
  left border by retyping over the line it has just printed. Treating CR
  as "drop it, WAITS text is CRLF" appends that overprint instead, and the
  envelope comes out with a row of stray `!` through the middle of it.
  Tabs are expanded to 8-column stops in the same pass, which two records
  confirm independently: in the classified-ads page the tab-indented lines
  land on exactly the same 73-column border as the untabbed ones, and in
  the construction-site sign they land on 56.

Those are the only three control codes the game ever sends to the terminal.
The message database contains nothing below 040 except tab, CR, LF and the
^D record separator, and over a 1 500-command run the running game emitted
no other control character either. All fifteen pieces of ASCII art render
intact; the widest is 76 columns, so none of them wraps on an 80-column
screen. Three small raggednesses in the art are in the authors' own text
and are left alone: the newspaper's `EXTRA!  EXTRA!` line carries four
spaces past its right border, the construction sign's `FisK Construction
Works Inc.` line is one character wider than the rest of its box, and the
payphone's coin box and handset hang outside the frame on purpose.
