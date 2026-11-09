# NERO0366, Adventure for the HP 9825: Windows console port

`advent.exe` plays "The Adventure Game -- Colossal CAVE 1/14/80" by Eric Nelson and Craig
Robinson (HP Desktop Computer Division, Fort Collins), the 366-point version with the boat,
reservoir and island. It runs the game's own HPL programs from `adventure.dsk`, translated
to C, with the disk's own database built in. It is one 32-bit exe that imports only
`msvcrt.dll` and `kernel32.dll`, so it runs on **Windows 7** (32- or 64-bit, no Universal CRT
or VC++ redistributable needed) as well as 10 and 11. It needs no other files.

    advent.exe [--no-fixes] [-w WIDTH] [--reflow] [--seed N] [--saves FILE] [--echo|--no-echo] [--no-pause]

| Option | |
|---|---|
| `--no-fixes` | play exactly as the 9825 program does, its restore bug and refilled text included (below) |
| `-w WIDTH` | the line width, the game's "printer's width" (default 79) |
| `--reflow` | join the lines of the game's text and refill them to the width, as the 9825 does |
| `--seed N` | seed the random numbers (default: the clock) |
| `--saves FILE` | where SUSPEND keeps the game (default `saves\ADVSAV` next to the exe) |
| `--echo` | print each command after reading it, as the 9825 does on its printer; the default when input is not a console |
| `--no-pause` | do not wait for Enter when the game ends |

## How it is built

`build.sh` (or `build.bat`) needs MinGW-w64 gcc with 32-bit libraries (Strawberry Perl's)
and python:

1. `src/hpl2c.py` translates `ADVENT`, `A1`, `A2` and `A3`, as the 9825 listed them
   (`../listings`), to C (`.build/game.c`).
2. `src/mkdata.py` turns every data file on `../adventure.dsk` into C tables
   (`.build/data.c`), using `../tools/hpldisk.py`.
3. They are compiled with the runtime `src/rt.c` and linked against `msvcrt.dll` (the
   toolchain's own `libmsvcrt.a` is the Universal CRT, so the build names `libmsvcrt-os`
   and tells the headers `__MSVCRT_VERSION__=0x0700`).

HPL has no procedures. `cll 'Name'(...)` and `gsb "Name"` jump to a labelled line and
come back at `ret`, and routines fall into one another ("Spk" into "Spkr" into "Echo"
into "Print"). So the translator makes one C function for each routine, holding the
lines reachable from its label, and one for each program's main line, entered at the line
a `chain` names. Every line becomes a label and `gto`/`jmp` become `goto`, so the
control flow is the listing's own. `cll` gives a routine new p-numbers, while `gsb`
lends it the caller's. Expressions follow HPL: implied multiplication (`2p1`,
`100rnd(0)`, `(1-p1)21`), `and`/`or` without short-circuit, left-to-right evaluation,
and `→` inside expressions. Strings keep their dimensioned lengths, `fti`/`itf` pack
16-bit integers into them, and the buffer "M" that `wrt "M.1"` fills and `red` empties
behaves as on the 9825. The data files keep their disk names and are read with
`rread`/`sread` record by record, as HPL reads them. The game state is held in
strings packed this way.

## Verified against the original running in MAME

`tests/mamerun.py` boots MAME's hp9825t (the eXo MAMEif set) with the 9895 floppy, runs
`ADVENT`, answers the set-up questions (select code 6, width 80), types the commands and
captures what the program prints on an HP 9871 on a 98032 card. `tests/cmpmame.py`
replays the same commands on the port:

    python tests\cmpmame.py      session 1: IDENTICAL (120 lines)
                                   data lines kept: same words
                                 session 2: IDENTICAL (159 lines)
                                   data lines kept: same words
                                 restore --no-fixes: IDENTICAL (first 19 lines)

The sessions are compared with `--reflow`; without it the port prints the same words,
broken into the data files' own lines (below).

The sessions cover the instructions, the well house, the grate and the rooms down to the
small pit, the lamp, liquids, two-word commands, `"GET" what?`, the WEST hint, `INFO`,
`SCORE` and `QUIT` with the rank messages. They stay where the game draws no random
numbers: the 9825 seeds with `rnd(-π)`, and the port uses its own generator. Below
that, `tests/fuzz.py` plays 500+ random games of 1500 commands each, with and without
`--no-fixes`. They meet the dwarves, knives, the snake, the troll, death and
reincarnation, and none hits an HPL error or hangs.

## The port's one fix: restoring a suspended game

`Readrest` (ADVENT line 40) reads `K$` and `T[*]` from SEC3H, so `T` holds the first 32
travel entries, travel block 1. It then reads `Q`, the block `T` is supposed to hold, from
the save. A new game clears `Q` (line 36) and so loads the right block, but a restored
game does not. Unless the game was saved while block 1 was loaded, the exits are wrong
until the player reaches a location in another block. Restored below the grate, west and
up go nowhere and south leads into the forest. The original in MAME does exactly this
(`tests/restore1.txt`, `restore2.txt`, `mame_r2.txt`). The port clears `Q` after a
restore too; `--no-fixes` restores the original behaviour.

## The game's text keeps its own lines

The messages and descriptions in the data files are stored a line per record, laid out by
hand for about 70 columns: words hyphenated across lines ("infor-" / "mation"), the
credits centred on lines of their own, a blank line after the introduction. The 9825's
"Print" routine (`A1` line 44, and in `A2`/`A3`) joins each line to the next and refills
the text to the printer's width, so on a wide printer "infor- mation" lands mid-line and
the credits run together. That suited the built-in 16-column strip printer. At a width of
71 or more (the longest stored line is 71 characters) the port prints each line as stored,
wrapping only text the program builds itself. Below 71, with `--reflow` and with
`--no-fixes` it refills the text as the 9825 does.

## Where the port differs from the 9825 (always)

- `ADVENT`'s questions "Enter output selectcode" and "What is your printer's width?" are
  answered by the port: the console (select code 11) at `-w` columns. A restored game also
  uses the console at the current width.
- At a console the player's command is not printed again after it is typed (the 9825
  prints it on its output device). The prompts ("?", "Do you want to start a new
  game?") are shown on the console, as on the 9825's LED display. Messages the 9825 shows
  only on its display ("Your Adventure is saved.") are printed.
- The random numbers are the port's own.
- `stp`/`end`: the console waits for Enter before closing.

## Notes on the program

- On restore `ADVENT` line 29 assigns `SEC12`, the source section, instead of `SEC12H` as
  file 4. Only `Mspk` reads file 4, and nothing reaches its caller (`"13000"`), so this
  bug never shows. The port keeps it.
- The dwarf movement code (`A1` "M6") collects candidate exits in `K[12]` while
  allowing up to 19. No location has more than 10, so it never overflows.
- `A1T` is a near-copy of `A1` that nothing chains to; it is not translated.
