# Colossal Cave Adventure, 751 points (PDP-10, TOPS-20)

**ADVENTURE < 6.1/ 3>, 14-Jan-82** — the 751 point Adventure, a
FORTRAN-10 program for which no source survives. What survives is the
binary, on a TOPS-20 pack.

The program's own messages name the machine it was set up for: Stanford's
LOTS (*NOTE: Open/Closed hours are disabled at LOTS*, *If you want to
explore, come to campus*). It calls its home directory `SRC:<GAMES>`, and
that is the directory a private dump of the SRI-NIC archive holds, with the
program, its text and its world bit for bit the same as on the pack.

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
| `ADVGRP.LOG`, `ADVWIN.LOG` | the gripe log and the scoreboard, still holding the players of 1984 |
| `FOROTS.EXE` | the FORTRAN-10 runtime the game runs on |
| `PA1050.EXE` | the TOPS-10 compatibility package, which the port replaces |
| `MONSYM.UNV`, `MACSYM.UNV` | the monitor's own symbol tables, where the JSYS numbers came from |
| `ADV501.EXE` | David Long's 501 point version, 20-Nov-78 — complete |
| `OLD-ADVENTURE.EXE` | *Version II*, 430 points, 21-Jun-79 — complete |
| `NEW-ADVENTURE.EXE` | edition 6.1/9, 13-Feb-81 — its databases lost, see below |
| `WIZDEF.DAT` | the configuration the game shipped with, before the eXo packagers pointed it at these files |

The three older games are Adventures too, and two of them are complete.
`ADV501.EXE` (*Adventure--Experimental Version:5.0/6, NOV-78*)
and `OLD-ADVENTURE.EXE` (*This is "Version II" of Adventure. Top score is
now 430 points.*) are core images with their databases already loaded.
Both start, play and score with no files beside them. `ADV501.EXE` also
runs on the pack as it is, with `R GAME:ADV501`, though quitting prints
*LOG FILE BLOCKED*: it writes its log to a directory the pack lacks.
ADV501 is David Long's 501 point game, which eXo also carries as a
JavaScript port.

Only `NEW-ADVENTURE.EXE` is incomplete. `<GAMES>` has only the 1982 text
and the 1984 world, both belonging to 6.1/3, and edition 6.1/9 refuses
them.

`NEW-ADVENTURE.EXE` turned up again in another private archive: a 1982 games directory
that also kept its configuration, billboard and play logs, though still not
its text or world. What could be recovered of that edition, 6.1/9, and how
it differs from 6.1/3, is in
[`..\Colossal Cave Adventure 751pt 6.1-9 (PDP-10, TOPS-20, databases lost)`](../Colossal%20Cave%20Adventure%20751pt%206.1-9%20%28PDP-10,%20TOPS-20,%20databases%20lost%29/README.md).
