# Aventure (Colossal Cave Adventure, 350 points, French)

DEC's *Adventure* Release 3 - the 350-point Crowther and Woods game in Kent Blackett and Bob Supnik's
FORTRAN IV version - translated into French, as found on an OpenVMS VAX disk (linked 26 August
1985). The source is lost, so the original VAX program and its data files run on a built-in VAX
user-mode emulator (`vaxvms.exe`) together with the real VMS FORTRAN and library run-times.

## Command line

`play.cmd` takes no parameters. It runs `vaxvms.exe -L lib -D . ADVENT.EXE`; the game itself takes
none either. The emulator's options:

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

`play.cmd`
