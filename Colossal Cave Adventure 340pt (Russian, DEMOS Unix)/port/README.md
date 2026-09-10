# ПРИКЛЮЧЕНИЕ — Windows port

A native Windows build of the Russian 340-point *Adventure* (`ANTO0340`),
written for DEMOS Unix around 1984–85 and archived as C source.

**To play: run `play.bat`** (or `adventure.exe`). Type in Russian:
`да`, `вниз`, `возьми лампу`, `помоги`, `счет`, `сохрани`, `сдаюсь`.

```
Приглашаем в путешествие! Вам нужны инструкции?
Вы стоите у конца дороги перед небольшим кирпичным домом. Вокруг вас
лес. Небольшой ручей вытекает из дома и течет по оврагу.
```

## What is here

| | |
|---|---|
| `adventure.exe` | the game (upstream calls it `ad`) |
| `adv.common`, `adv.data`, `adv.text` | its database — the 1985 wording |
| `text-revised\` | the same database with proof-read wording (see below) |
| `play.bat` / `play-revised-text.bat` | pick a wording and run |
| `ini.exe` | rebuilds the database from `src\cave\` |
| `build.sh` / `build.bat` | rebuild everything |
| `verify.bat` / `tests\` | the verification suite |
| `src\` | the ported sources |

The game reads its database from the current directory first and then from
its own, which is all `play-revised-text.bat` does. Saved games
(`сохрани`) are written next to `adventure.exe` as `adv.frozen`.

## The Cyrillic

The game is KOI8-R throughout — message file, vocabulary, and the
"lower-case this letter" routine, which flips bit 0x20 on any byte with
the top three bits set. That is not a detail that can be translated away,
so nothing in the game's own text handling was touched. Instead the port
intercepts file descriptors 0 and 1:

* **Output.** KOI8-R is converted to UTF-16 and written with
  `WriteConsoleW`, so the console never has to agree about a code page.
  Bare LF becomes CR LF, which conhost and Windows Terminal both want.
  Redirected output is UTF-8 instead, in binary mode, so a piped
  transcript is exactly what the game produced.
* **Input.** The console is read with `ReadConsoleW` and folded back to
  KOI8-R, so you can type Russian directly. Redirected input is taken as
  UTF-8, falling back to raw bytes if it is not valid UTF-8 — a KOI8-R
  script file works too.
* **Fonts.** If the console is still using a raster font (which has no
  Cyrillic at all) it is switched to Consolas.

This replaces `luit -encoding KOI8-R`, which is what the Linux build in
`..\linux_reference\` uses and what has no equivalent on Windows.

## Two wordings

The Linux build's `adv.text` is *not* what this source zip produces: 35 of
its 670 messages had been proof-read by somebody, and those edited sources
were never archived. They have been recovered here —
`tests\recover-revised-sources.py` locates each edited message back in the
cave files, and `src\cave_revised\` is the result. It reproduces the Linux
build's `adv.text` and `adv.data` **byte for byte**.

So you can have either:

* `play.bat` — the 1985 text as the zip has it, typos and all
  («Дополнит**у**льную информацию», «ктоме вас», «забит **м**ессой валунов»);
* `play-revised-text.bat` — the corrected text, matching the Linux build.
  Beyond typos it rewrites a few lines, e.g. the well house is «Это
  колодезный домик для большого родника» rather than «Хороший дом для
  большого родника».

Neither changes the map, the puzzles or the scoring: `adv.data` is
identical for both.

## What was changed in the sources

`src\port\portcompat.h` is force-included into every 1984 file, so the
game code itself is almost untouched. It supplies:

* **Return-type declarations.** K&R C assumes an undeclared function
  returns `int`, which truncates a pointer on any 64-bit target. This is
  fatal, not cosmetic: `get.c` calls `malloc` undeclared, so the sources
  **segfault before the first prompt** on x86-64 Linux just as much as on
  Windows. `conv()` is undeclared in `fatal.c` and `mscore.c` too, which
  breaks the score display and every error message.
* **Binary file modes** — `adv.text` is a blob of NUL-terminated strings
  addressed by offset; text mode would corrupt it at the first 0x1A.
* **A legal save-file name** — the original writes `adv:frozen`, and a
  colon on Windows names an NTFS alternate data stream.
* **The I/O redirection** described above, plus renaming the game's own
  `tolower(buf,len)` out of the C library's way.

Four small edits were made in the game files themselves, each marked
`PORT:`:

| file | change |
|---|---|
| `adv\events.c` | `time(tim); srand(tim[1])` seeded from the *high* half of a 64-bit `time_t`, i.e. always 0 — every game would have been identical. Now seeded from the clock. |
| `adv\yes.c` | end of input re-asked "do you really want to give up?" for ever. Now exits. |
| `adv\pct.c`, `adv\events.c` | dropped the local `unsigned rand()` declarations that conflict with `<stdlib.h>`. |
| `adv\getans.c` | forward declarations for two `static` functions; gcc ≥ 10 rejects a static definition after an implicit declaration. |

One warning is left deliberately: `tolower.c` returns a `char *` from a
function the original declares as `int`. Nothing uses the value; fixing it
would change the source for no reason.

There is one further difference from the Linux build, and it is an
improvement rather than a change: `read(0,buf,79)` on a terminal returns
one line, but from a pipe it returns 79 bytes of whatever is buffered, so
the Linux build **discards every command after the first** when driven
from a file. The port reproduces terminal semantics on both, which is what
makes the transcripts below possible.

## Verification

`verify.bat` (or `sh tests\verify.sh`) checks the port against a native
Linux build of the *untouched* 1985 sources. Both sides use the same fixed
RNG (`-DTESTRAND`) so their transcripts can be compared directly; the
Linux side is driven through a real pty by `tests\ptyrun.py` and is
regenerated by `wsl bash tests/linux-reference.sh`.

```
adv.text (revised) vs Linux build              IDENTICAL
adv.data vs Linux build                        IDENTICAL
console encoder over the whole message file    IDENTICAL
40-command transcript vs Linux                 IDENTICAL
406-command transcript vs Linux                IDENTICAL
save / restart / restore vs Linux              IDENTICAL
```

The database check is the strong one: `adv.text` and `adv.data` have no
host-dependent layout, and `ini.exe` reproduces the Linux `ini` byte for
byte (`md5 9bcc69c2… / 6160958d…`). The console encoder is checked against
an independent KOI8-R decoder over all 42 619 bytes of the message file,
covering 131 distinct byte values. Save files are byte-identical across
the two builds, so a game saved on Linux resumes on Windows.

`adv.common` is the one file that differs — it is a raw image of the C
globals, and `long` is 8 bytes on Linux against 4 on Windows (14 479 vs
12 279 bytes). It is regenerated by `ini.exe` and only ever read by the
matching `adventure.exe`.

## Building

Needs MinGW-w64 gcc (Strawberry Perl's is fine) and an `sh`.

```
build.bat
```

The sources are K&R C, so `-std=gnu89`; they rely on tentative
definitions being merged across translation units, so `-fcommon`; their
string literals are KOI8-R bytes that must survive verbatim, so
`-finput-charset=ISO-8859-1 -fexec-charset=ISO-8859-1`; and no routine of
theirs may be replaced by a compiler intrinsic, so `-fno-builtin`.
