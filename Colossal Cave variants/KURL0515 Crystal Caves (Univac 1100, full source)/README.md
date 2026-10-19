# Cave (Crystal Caves)

Duff Kurland's *Cave* (Information Systems Design, 1980) for Univac 1100 mainframes: John Kopf's
Tymshare game *Crystal Cave*, carried over by way of Bob Elman's Four Phase version, with a compass
added. The engine is Crowther and Woods' *Adventure*, the setting new: a Boy Scout's spelunking trip
through a cave below a barn, a pasture and a sinkhole. Native Windows port of the FORTRAN source,
recovered from scanned listings. The timesharing original's account checks, business-hours gate
and wait before resuming are removed; the magic word for wizard mode is `dwarf`.

## Command line

`cave.exe` takes no parameters. It reads `CAVE.DAT` from the current directory and keeps its saved
game (`CAVE.SAV`, written by `SUSPEND` and offered at the next start) and the wizard's settings
(`CAVE.CFG`) there.

## Recommended start

`cave.exe`, started in its own folder (double-clicking it does that).
