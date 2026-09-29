# Adventure! (Robert A. Howell, 1982)

Robert A. Howell's cut-down *Colossal Cave* in Atari BASIC for the Atari 400/800 (1982), 301 points:
the "all different" maze shares one description, above ground is four identical rooms, and the 15
treasures and the magazines give 281 points - nobody has shown how to get the rest. The disk image
runs as it is in an Atari emulator; no emulator is included.

## Command line

The disk has no parameters. The machine it needs: Atari 800 hardware, OS revision A (its start-up
file jumps straight into OS-A editor routines) and the Atari BASIC revision A cartridge. It then boots
to "Do you want INSTRUCTIONS?".

## Recommended start

In Altirra, with the OS-A and BASIC-A ROM images:

`Altirra64.exe /hardware:800 /kernelref:<OS-A ROM> /memsize:48K /cartmapper 1 /cart <BASIC-A ROM> /disk HOWE0301.atr`

or in atari800: `atari800 -atari -basic HOWE0301.atr` (with OS revision A installed).
