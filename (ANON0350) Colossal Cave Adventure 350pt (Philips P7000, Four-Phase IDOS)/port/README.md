# ADVENT UNDER IDOS - port

The 350-point Colossal Cave Adventure ("ADVENTURE   07 JUNE 1978") as a Danish site ran it under IDOS on a Philips
P7000, which is a Four-Phase Systems System IV. The game is not rewritten. The port boots the site's own disc pack
(`../src_original/P7000.PACK`) on an emulated Four-Phase IV/70 and types `// ADVENT`, as the operator did. It then
shows the machine's 24 x 81 video screen in the console window.

## Build and play

    build.bat          (or sh build.sh; needs gcc - MinGW-w64 on Windows - and sh on PATH)
    play.bat

`build.sh` compiles `advent.exe` and copies the pack to `p7000.pack` next to it. `play.bat` runs the game with `-u`.
It keeps the working copy of the pack in `saves\advent.pack`, which is made from `p7000.pack` on first use. A
suspended game lives in that pack.

Type commands and press Enter; Backspace deletes a character. Ctrl+C leaves at once. The game ends with QUIT, with
a death you decline to be reincarnated from, or with SUSPEND. The status line then says the game is over, and a key
closes the window.

    advent [OPTION]...
      -u, --unlimited    clear the wizard's prime-time hours and the wait before a resume
                         (this build enforces neither; HOURS and SUSPEND then say so)
      --transcript       follow the screen as a scrolling log and read lines from stdin
                         (the default when stdin or stdout is not a console)
      --fixed-clock      the 60 Hz clock counts instructions instead of real time (repeatable runs)
      --pack=FILE        the working copy of the pack (default advent.pack)
      --trace=FILE       an instruction trace (debugging)
      -h, --help

Unknown options are refused with exit status 2. There are no fixes, so there is no `--no-fixes`.

## About this version

It is the Crowther/Woods 350-point game with the wizard's machinery, but the site changed some things:
- The magic words are **ZYXXY** (the note in the debris room says so) and **CLUNK** (the hollow voice at Y2 says
  it). XYZZY and PLUGH are not words here.
- The crawl over cobbles west of the grate is dark: its room has no light bit in the site's table. Light the lamp
  before you go in, or you risk the pit. The port keeps the site's table (user, 2026-09-21).
- HOURS says the cave is closed to regular adventurers on weekdays from 8:00 to 15:00, and SUSPEND asks you to
  wait a minute before resuming. This build enforces neither: its START does not look at the clock, and its DATIME
  always answers day 273, 07:51. `-u` clears
  the wizard's hour masks (WKDAY, WKEND, HOLID) and the wait (LATNCY) while the game is in memory, so the messages
  say what the game does.
- SUSPEND keeps one game, in the file ADSAVE, and the next start goes on with it. After a game has ended, the next
  start is a new game (see below).

## How it works

- `src/cpu.c` is the IV/70 processor: 24-bit words, the type-1 and type-2 instruction sets, floating point,
  interrupts on 8 levels. The IV/90 extensions (MAP, BYTE, BIT, BDEC/DBIN, MVEL, IOXW) are not needed by IDOS or
  ADVENT, and are not emulated.
- `src/io.c` holds the devices:
  - the 8231 disc (channel 2, unit 024), with the whole pack in memory and every write going straight to the file;
  - the 7200 keyboard 0 (channel 3, level 3);
  - the 60 Hz clock (an INR at location 0 on level 0), in real time or counting instructions;
  - the memory-mapped screen at 0140: 24 lines of 32 words, 27 of them used, 3 characters a word, 032 the cursor.
- `src/advent.c` is the front end:
  - it boots with the console keys 37705121 (BOOT from channel 2, unit 024);
  - at the `// $BATCH` screen it types `// ADVENT` NEW LINE `//` NEW LINE;
  - it mirrors the screen in the console, or follows it as a log (`--transcript`);
  - keys go to the machine only when the program is waiting for one (see the keyboard notes in `io.c`).
- **New or old game.** ADVENT loads its whole state from ADSAVE at every start. Word 0 of ADSAVE is 1 for a game
  to play (fresh or suspended); the end of a game clears it. A start from a cleared ADSAVE tries to build the
  database from scratch ("INITIALIZING...", then "ERROR CODE = 5"), which this installation cannot do. The site had
  two job files. OLD is `// ADVENT`. NEW first does `// COPY /I=ADNEW,O=ADSAVE.` to start from the fresh state
  ADNEW. The port does NEW's copy itself, in place, before it boots, whenever word 0 is clear. IDOS's COPY writes a
  new file after the last one every time, and the pack is full after four new games; the site will have compacted
  it now and then.
- **The end of a game.** QUIT, and a death without reincarnation, end with the FORTRAN STOP: the score, then a halt
  (HLT at 043244). SUSPEND goes to the run-time library's EXIT instead. Its routine at 062612 reads IDOS's bootstrap
  from sector 0 into location 1 and enters it with a pointer to a program name in X1. This bootstrap wants the BOOT
  instruction there, as the console keys give it on a cold start, so on the real machine it would end up sounding
  the keyboard alarm in a loop at 00062, and the operator booted again. The port ends the run when the game halts
  or enters 062612.

What had to be inferred about the machine (the manuals do not say):
1. **The CPU probe.** IDOS tells an IV/70 from an IV/90 by running UFA with an exponent difference that overflows.
   An IV/70 must trap there.
2. **MVE keeps RB.** IDOS's memory clear saves location 0 in RB across four MVEs.
3. **Absent drives.** The other units on the disc controller (025-027) answer "not ready".
4. **ODD honours its count.** The rule is M = S, R = 77777777; then C times, M is rotated left one bit and R ^= M.
   IDOS uses it for its error-return offsets: `ODD R1,RA,7,47` = -2. An ODD that computes only the parity sends
   IDOS's directory code to the wrong return, and COPY then fills every free directory slot for ever.
5. **The keyboard word.** IDOS keeps one key in one memory word. A key is typed only when the program polls that
   word and finds it empty (or after 3M instructions of silence, for a program that has not polled yet).

## Tests

    python tests/regress.py            options, the reference walk, new/old games, SUSPEND, -u, endings
    python tests/regress.py --record   rewrite tests/reference/walk.out from this build
    python tests/fuzz.py [GAMES] [TURNS] [SEED]   random commands; three runs per game on one pack
    python tests/consoleplay.py        at a real console (ConPTY): SUSPEND and resume, QUIT, Backspace,
                                       Ctrl+C, play.bat

`tests/reference/walk.out` is this port's own output: there is no other P7000 to compare with. The walk goes in by
ZYXXY, catches the bird, drives off the snake, and comes back by CLUNK with the gold and silver. Dwarves then kill
the player, who is reincarnated. It runs with `--fixed-clock`, so it repeats exactly. With the real-time clock the
game's random numbers start elsewhere: the dwarves, the hollow voice and the replies to unknown words then differ.

Debugging (environment variables): `ADVENT_DUMP=FILE` writes memory at the end, `ADVENT_TRACE_FROM=N` and
`ADVENT_TRACE_TO=N` limit `--trace`, `ADVENT_STOP_AT=N` stops the machine, `ADVENT_WATCH=OCTAL` reports the
registers at each visit to an address.

## Still to do (refine pass)
- Fuzz with the real-time clock.
- The console mirror assumes an 81 x 25 window. Check it in Windows Terminal, and at other sizes.
- The screen shows 7-bit ASCII only. Check whether the 7200's other codes (attributes, graphics) occur in the game.
- Instructions IDOS and ADVENT never execute are emulated from the manual alone and are untested.
