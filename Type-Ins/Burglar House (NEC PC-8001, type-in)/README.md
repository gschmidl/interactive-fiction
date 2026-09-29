# Burglar House (NEC PC-8001, 1983)

*BURGLAR HOUSE - Adventure #0*, copyright 1983 by T. Shimaya, a Japanese type-in for the NEC PC-8001:
you are a burglar working a mansion between 10 p.m. and 6 a.m. The game is Z80 machine code with a
short N-BASIC loader that sets up the screen and the function keys; text is in halfwidth katakana,
commands are noun then verb, and `LOAD` and `SAVE` are the game's own. Recovered from the magazine
listing; the program checks its own bytes at start-up, and this transcription passes. It runs in a
PC-8001 emulator; no emulator is included.

## Command line

No parameters. `Burglar House.bin` (8704 bytes) must be in memory at &H9000 before the BASIC loader
(`loader.cmt`, a PC-8001 tape) is run: load the block, then load the loader tape and `RUN`.

## Recommended start

In Takeda's PC-8001 emulator: rename `Burglar House.bin` to `debug.bin`, put it in the emulator's
folder, and in the CPU debugger type `L 9000`, then `Q`; then load `loader.cmt` from tape and `RUN`.
