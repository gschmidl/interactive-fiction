# Adventure, 350 points (PDP-8, OS/8)

Dick Murphy's OS/8 version of the 350-point *Adventure* for the PDP-8 (1978), recoded from Bob
Supnik's RT-11 port - most of it in hand-written FPP-8 assembler to fit into 32K. Magic mode, SUSPEND
and the cave hours were left out on the way, so the cave is always open. `adventure.exe` is a PDP-8/E
emulator that boots the original OS/8 pack and runs the original program under the FORTRAN run-time
system.

## Command line

`adventure.exe [options]`

| Option | Effect |
| --- | --- |
| `-s FILE` | the file that keeps the saved game (default `adventure.sav` beside the program) |
| `-t FILE` | also write a transcript of the session to FILE |
| `-n` | do not read or write a save file |
| `-r` | show the OS/8 boot dialogue instead of hiding it |
| `-C` | trace console traffic to standard error |
| `-D N` | trace the first N instructions to standard error |
| `-h` | list the options |

`SAVE` and `RESTORE` work inside the game.

## Recommended start

`adventure.exe`
