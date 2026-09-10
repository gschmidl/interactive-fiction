# Colossal Cave Adventure, 751 points (PDP-10, TOPS-20)

**ADVENTURE < 6.1/ 3>, 14-Jan-82** — the Carnegie-Mellon Adventure, a
FORTRAN-10 program for which no source survives. What survives is the
binary, on a TOPS-20 pack.

`port\bin\adv751.exe` plays it on Windows. It is the original binary on
an emulated DECsystem-10, not a rewrite; see `port\README.md`.

```
port\           the port: emulator, build, tests, and how it was done
dump_original\  the files taken off the pack, as 36-bit words
src_original\   the eXo collection's TOPS-20 setup, untouched
work\           the reference machine and the tools that drove it
```

## What came off the pack

`dump_original\` holds rather more than the port needs:

| | |
|---|---|
| `ADVENTURE.EXE` | the game, 17-Jun-84 |
| `ADVTXT.BIN` | its text, 14-Jan-82, encrypted |
| `ADVVAR.BIN` | its world, 18-Apr-84 |
| `ADVWIZ.DAT` | the configuration and the wizard list |
| `ADVGRP.LOG`, `ADVWIN.LOG` | the gripe log and the scoreboard, still holding CMU players from 1984 |
| `FOROTS.EXE` | the FORTRAN-10 runtime the game runs on |
| `PA1050.EXE` | the TOPS-10 compatibility package, which the port replaces |
| `MONSYM.UNV`, `MACSYM.UNV` | the monitor's own symbol tables, where the JSYS numbers came from |
| `ADV501.EXE` | the 501 point version, 20-Nov-78 |
| `OLD-ADVENTURE.EXE` | 21-Jun-79 |
| `NEW-ADVENTURE.EXE` | 13-Feb-81 |
| `WIZDEF.DAT` | the configuration the game shipped with, before the eXo packagers pointed it at these files |

The three older games are the same lineage and two of them are built the
same way, but their databases were not kept — `<GAMES>` has only the
1982 text and the 1984 world, both belonging to 6.1/3. They are here
because they were there.
