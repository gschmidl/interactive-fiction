# ASCII-ladden (Commodore 64, 1985)

*ASCII-ladden*, a Norwegian text adventure in Commodore 64 BASIC by Tor Engebakken and John Andersen,
published as a type-in listing across four issues of *Mikrodata* / *PC Mikrodata* (1/1985-4/1985) in
the series "Adventure på norsk", and recovered from the magazine pages. The disk image carries one
repair: the listing sends `FLY` anywhere but at the stone to a line that was never printed, which
stopped the game with an error; it now gets the game's usual "Det er jeg ikke istand til." It runs in
any C64 emulator; no emulator is included.

## Command line

The disk image has no parameters: `LOAD"ASCII-LADDEN",8` and `RUN`, or let the emulator autostart it.

## Recommended start

In VICE: `x64sc -autostart "ascii-ladden-fixed.d64:ascii-ladden"`
