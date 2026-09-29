# Wang 928 Adventure (370 points)

"W A N G 9 2 8 A D V E N T U R E (Version 2.1)", *Colossal Cave* for the Wang OIS office system, from
a Wang demonstration disk; author and date unknown. It is a 370-point game, although the only
addition to Woods' 350 points seems to be some ASCII art when the wizard brings you back to life.
`wangadv.exe` is a Z80 with enough of the OIS workstation around it to run the original program and
data unmodified, on the workstation's 24 x 80 screen.

Type at the entry field on the bottom line; `ESC` or Ctrl+C stops the machine, and `QUIT` ends the
game.

## Command line

`wangadv.exe [options]`

| Option | Effect |
| --- | --- |
| `-d`, `--debug` | report file access, refused operations and the program's fatal errors on standard error |
| `--trace N` | keep the addresses of the last N instructions (diagnostics) |

The data files are looked for in `data` beside the program, then beside the program, then in `data`
and in the current directory.

## Recommended start

`wangadv.exe`
