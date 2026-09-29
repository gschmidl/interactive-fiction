# ADVENT (Colossal Cave Adventure, 350 points, Philips P7000)

The 350-point Crowther and Woods game with the wizard's machinery ("ADVENTURE 07 JUNE 1978"), as a
Danish site ran it under IDOS on a Philips P7000 - a Four-Phase Systems System IV. The site changed a
few things: the magic words are ZYXXY and CLUNK, and the cobble crawl west of the grate is dark.
`advent.exe` boots the site's own disc pack on an emulated Four-Phase IV/70, starts the game as the
operator did and shows the machine's 24 x 81 screen in the console window.

## Command line

`advent.exe [options]` (`run.bat` adds `-u`, keeps the working disc pack in a `saves` folder, and
passes its parameters on)

| Option | Effect |
| --- | --- |
| `-u`, `--unlimited` | clear the wizard's prime-time hours and the wait before resuming a suspended game. This build enforces neither; with `-u`, `HOURS` and `SUSPEND` say so |
| `--pack=FILE` | the working copy of the disc pack, which also holds the suspended game (default `advent.pack`, made from `p7000.pack` when missing) |
| `--entry-line-top` | show what you type on the screen's top line, where the P7000 showed it (by default it is at the bottom) |
| `--transcript` | follow the screen as a scrolling log and read lines from standard input (the default when either is not a console) |
| `--fixed-clock` | let the 60 Hz clock count instructions instead of real time, so a run repeats exactly |
| `--trace=FILE` | write an instruction trace to FILE (debugging) |
| `-h`, `--help` | list the options |

## Recommended start

`run.bat`
