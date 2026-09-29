# ZK: Spitbrook Interactive Fiction (VAX/VMS, 1985)

*ZK*, "the first interactive fiction game written exclusively for the VAX", by William Lees and
Edmund Sullivan at Digital's Spitbrook Road software engineering facility in Nashua, New Hampshire,
where the game is set ("Meet famous Spitbrook personalities!"); version V1.0-823 of 29 August 1985.
A screen game with a status line, a room banner and a scrolling text region, so it needs a console
that understands VT100 sequences (Windows Terminal does). The original VAX program runs on a built-in
VAX user-mode emulator (`vaxvms.exe`) with the real VMS Pascal, library, screen-management and math
run-times.

## Command line

`play.cmd` takes no parameters. It runs `vaxvms.exe -L lib -D . ZK.EXE`; the game itself takes none
either. The emulator's options:

`vaxvms.exe [options] IMAGE.EXE`

| Option | Effect |
| --- | --- |
| `-L DIR` | where to look for the VMS shareable images (repeatable) |
| `-D DIR` | where the game's data files are (default: the current directory) |
| `-V` | report how the images are loaded |
| `-s` | log system services and file access (repeat for more) |
| `-T N` | instruction trace level |
| `-B ADDR` | report the arguments each time the program reaches that hex address |
| `-b ADDR` | the same, then stop |
| `-M ADDR` | show memory at that hex address in the trace |
| `-W LO HI` | report every write to that hex address range |
| `-d LO HI` | disassemble that hex address range and exit |
| `-k N` | stop N instructions after the first terminal read, tracing them |

All but `-L` and `-D` are for debugging the emulator.

## Recommended start

`play.cmd`, in Windows Terminal.
