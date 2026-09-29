# FUNADV

An extended *Adventure*, "the first public release of FUNADV since it was ported from the CDC Cyber
Fortran", for VAX/VMS; its cave was last modified on 24 February 1992. It is not the standard cave:
there are a mongoose, a potion, runes, a chair, listings and mail, things can be worn, words are
matched to ten letters, and the screen flashes as you fall down a shaft. Found on an OpenVMS VAX
disk without its source, so the original VAX program runs on a built-in VAX user-mode emulator
(`vaxvms.exe`) with the real VMS run-time libraries.

## Command line

`play.cmd [options]` runs `vaxvms.exe -L lib -D . [options] FUNADV.EXE` and passes its parameters to
the emulator. The game itself takes none.

| Option | Effect |
| --- | --- |
| `-baud N` | pace the output at N bits per second, as on a serial line, so the game's falls and pauses show (default 9600; `0` sends everything at once) |
| `-flash N` | hold a reverse-screen flash for N milliseconds (default 120; `0` for none) |
| `-effects FILE` | rules for flashes added to the game's own (default `effects.txt`: three after the landing at the bottom of the shaft); `none` shows only the game's own |
| `-demo` | play the game's screen effects and exit |
| `-L DIR` | where to look for the VMS shareable images (repeatable) |
| `-D DIR` | where the game's data files are |
| `-V` | report how the images are loaded |
| `-s` | log system services and file access (repeat for more) |
| `-T N` | instruction trace level |
| `-B ADDR` | report the arguments each time the program reaches that hex address |
| `-b ADDR` | the same, then stop |
| `-M ADDR` | show memory at that hex address in the trace |
| `-W LO HI` | report every write to that hex address range |
| `-d LO HI` | disassemble that hex address range and exit |
| `-k N` | stop N instructions after the first terminal read, tracing them |

Pacing and flashes are off when the output is not a terminal. `-L` onwards are for debugging the
emulator.

## Recommended start

`play.cmd`
