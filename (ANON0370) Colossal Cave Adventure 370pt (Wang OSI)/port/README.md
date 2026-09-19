# WANG 928 ADVENTURE — Windows port

`WngAdv.zip` holds a port of Colossal Cave Adventure for the **Wang OIS** office
system, as a set of files from a Wang demonstration disk:

| File | Size | What it is |
| --- | --- | --- |
| `DEMO.VENTURE.START` | 27 648 | Z80 program image, loaded at `0100h` |
| `DEMO.VENTURE.OLAY` | 18 432 | two code overlays, loaded at `4700h` |
| `DEMO.VENTURE.DATA` | 45 312 | the game text |
| `DEMO.VENTURE.DATA.txt` | 45 312 | the same file with `1Eh` line separators converted to `0Ah` |

There is no emulator for the Wang OIS, so this port supplies the machine instead
of rewriting the game: a Z80 core plus a shim for the parts of the OIS
workstation the program touches. **The original Z80 code runs unmodified** —
the three data files are shipped byte-for-byte as they came out of the zip, and
every room description, message, travel table and vocabulary entry is the
Wang program's own.

The banner reads `W A N G   9 2 8   A D V E N T U R E   (Version 2.1)`. It is a
370-point Adventure.

## Playing

```
wangadv.exe
```

It opens a 24×80 screen, exactly the workstation's. Type at the entry field on
the bottom line; `RETURN` submits, `BACKSPACE` corrects. `ESC` or `Ctrl+C` stops
the machine. The game ends by itself on `QUIT` → `YES`.

`wangadv.exe` looks for the three data files in `data\` beside the executable,
then beside the executable itself, then in `data\` and `.` under the current
directory.

## Building

Needs a MinGW-w64 `gcc` on `PATH` (the one bundled with Strawberry Perl
works). Either:

```
build.cmd
```

or, with `mingw32-make`:

```
mingw32-make
```

## Layout

```
wangadv.exe      the game
wangtest.exe     scripted front end: reads commands from stdin, prints the screen
data\            the three original Wang files, unmodified
src\z80.c/.h     Z80 CPU core
src\wang.c/.h    the workstation: memory map, screen planes, keyboard, OS mailbox
src\con_win.c    Windows console front end (ships in wangadv.exe)
src\con_test.c   scripted front end (ships in wangtest.exe)
tools\demo.txt   a short scripted session
tools\z80dis.py  the disassembler used to work the machine interface out
```

`wangtest.exe` replays a session without needing a console, which is how the
port was tested:

```
wangtest.exe < tools\demo.txt
```

Two development switches: `-d` traces OS calls and reports why the machine
stopped, `--trace N` dumps the last N program counters on exit.

## The machine, as recovered from the images

None of this was documented anywhere reachable; it was read out of the two
program images with `tools\z80dis.py`. The load address falls straight out of
the first three bytes of `START` — `C3 00 20`, a jump to `2000h` — which only
lands on the program's entry code if the image sits at `0100h`.

**Memory**

| Range | Use |
| --- | --- |
| `0000h–00FFh` | supervisor communication area |
| `0100h–6CFFh` | the `START` image; code from `2000h` |
| `4700h–6AFFh` | overlay window, filled from `OLAY` |
| `6BDBh–B7FFh` | heap, zero-filled at startup |
| `B800h–BBFFh` | file buffer, four pages below the top of user RAM |
| `BC00h–BFFFh` | supervisor reserve, where the stack lives |
| `C000h + row*100h` | screen attribute plane, 24 rows of 80 |
| `E000h + row*100h` | screen character plane, 24 rows of 80 |

The character plane stores `00h` for a space and ASCII otherwise. Only two
attribute bits are used: `80h` marks the 32-column entry field on the bottom
line, and `20h` marks the cell holding the cursor.

**Request mailbox.** The program puts a parameter-block pointer in `0005h` and a
command in `0004h`, then spins until `0004h` reads back zero.

| Command | Meaning |
| --- | --- |
| `01h` | file I/O; the block is an IOCB |
| `02h` | six time-of-day counters, which the program folds into its RNG seed |
| `10h` | spool/print request |

The IOCB is: function, status, handle, block count, buffer address, block
number, then three status bytes. Status `80h` is success. Function `0` opens
the file named by a `"/library:pir=NAME////"` spec, `1` reads 256-byte blocks,
`4` closes, `8` asks for the extent. Blocks are 256 bytes throughout — which is
also why every section of `DEMO.VENTURE.DATA` starts on a 256-byte boundary.

**Ports and interrupts.** `IN 00h` returns a keyboard scan code, translated
through a 256-entry table at `434Dh` that the shim inverts to map host keys
back to scan codes. Keys arrive as a mode-0 interrupt vectoring through a `JP`
the program plants at `0000h`. `OUT 01h` halts the workstation, `OUT 03h`
refreshes the display, and `IN 07h` reports the workstation sub-model — which
the program only uses to look up how much RAM it has.

**The two values the images cannot tell us** are the ones the supervisor would
have supplied: the workstation sub-model (`0007h` and `IN 07h`) and the number
of pages the supervisor reserves at the top of RAM (`0011h`). The shim reports
model 5 sub-model `0Ch`, which the program's own table reads as 48K, and a
four-page reserve. Those are the settings that make the program's arithmetic
close: the heap ends exactly below its file buffer, and the buffer ends exactly
below the stack. The stack never gets deeper than about 46 bytes in play.

## The data file

`DEMO.VENTURE.DATA` holds four sections, each starting on a 256-byte boundary,
bracketed by `02h`/`03h`, with records separated by `1Eh` and the record number
split off by a tab:

| Section | Records | Contents |
| --- | --- | --- |
| 1 | 139 | long room descriptions, rooms 1–140 |
| 2 | 65 | short room descriptions |
| 5 | 107 | object names and state descriptions, numbered `(state+1)*100 + object` |
| 6 | 230 | messages 1–201 and rank messages 301–308 |

The travel table and the vocabulary are not in the data file; they are compiled
into the Z80 code, which is another reason this port runs the original binary
rather than reimplementing the game.

## Notes

Adventure on the OIS has no SAVE or RESUME, so the shim never needs to write to
disk; the write-side I/O functions are stubbed and are never reached in play.
The final `(Press any key to return to Demonstrations Menu.)` is the original
program's own ending — on the demo disk it went back to a menu, and here the
keypress ends the process.
