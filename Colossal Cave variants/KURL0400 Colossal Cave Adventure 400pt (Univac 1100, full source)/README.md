# Adventure, 400 points (Univac 1100)

Crowther and Woods' *Adventure* as Duff Kurland (Information Systems Design) ported and extended it
for Univac 1100 mainframes in 1978-79: Tymshare's 382-point version, by way of Bob Elman's port for
Four Phase Systems, in Univac ASCII FORTRAN. Native Windows port of the FORTRAN source, recovered from
scanned listings. The timesharing original's account checks, business-hours gate and wait before
resuming are removed; wizard mode still asks for the magic word.

## Command line

`adventure.exe` takes no parameters. It reads `ADV.DAT` from the current directory and keeps its
saved game (`ADV.SAV`, written by `SUSPEND` and offered at the next start) and the wizard's settings
(`ADV.CFG`) there.

## Recommended start

`adventure.exe`, started in its own folder (double-clicking it does that).
