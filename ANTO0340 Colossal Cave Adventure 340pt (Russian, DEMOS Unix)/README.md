# Colossal Cave Adventure, 340 points — Russian (DEMOS Unix)

«ПРИКЛЮЧЕНИЕ», a Russian 340-point *Adventure* written in C for DEMOS
(the Soviet Unix) and archived as `ANTO0340 Sources.zip`. The source
headers date the engine to 07.01.85 and the newest file to 1994, with the
makefiles rewritten in 2001. All text is KOI8-R.

The game is unusual among the Russian ports in being a genuine
reimplementation rather than a translation of the FORTRAN: the cave is
described by a small declarative language (`cave/adv_*`) that a separate
tool, `ini`, compiles into three binary files the interpreter reads.

| | |
|---|---|
| `port\` | **the Windows port — start at `port\README.md`, play with `port\play.bat`** |
| `src_original\` | the zip and its contents, untouched |
| `linux_reference\` | the Linux build this port was checked against |

## Where it came from

* Sources: `ANTO0340 Sources.zip`, from the Russian section of the eXo
  interactive-fiction collection (kept verbatim in `src_original/`)
* Linux build: the same collection's prebuilt Linux package for the
  0340-point Russian Adventure (kept in `linux_reference/`)

The Linux build runs the game under `luit -encoding KOI8-R`, which
translates the game's KOI8-R to the terminal's encoding. There is no
`luit` on Windows, so the port does that conversion itself, in the
program.

## Two things worth knowing

**The archived sources do not build and run as they stand.** They are K&R
C: an undeclared function is assumed to return `int`, and `get.c` calls
`malloc` undeclared, so the returned pointer is truncated to 32 bits. The
game segfaults before its first prompt on x86-64 Linux exactly as it would
on Windows. `conv()` is undeclared in two more places, which breaks the
score display and every error message. The port declares them; so does
`port\tests\posixfix.h`, which is the only thing added to the sources for
the Linux reference build.

**The Linux build's text is not the archived text.** 35 of its 670
messages had been proof-read — typos fixed and a few lines rewritten — and
those edited sources were not archived. `port\tests\recover-revised-sources.py`
recovers them by locating each edited message back in the cave files;
`port\src\cave_revised\` reproduces the Linux build's `adv.text` and
`adv.data` byte for byte. The port ships both wordings and lets you pick.
