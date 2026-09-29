# Martian Adventure (Waterloo, 1978-79, unfinished)

Brad Templeton's unfinished Martian adventure for the Honeywell 6000 at the University of Waterloo,
written in B in 1978, with a Math building in Mark Niemiec's adventure language (1979). What survives
plays in part: you can walk the Martian desert from your ship to the Viking I lander, past the WELCOME
sign and through the airlock into the domed city - 76 rooms, every word from the 1978 files - and the
Math building with its puzzles. The object system and much of the planned game were never written or
are lost. `mars.exe` is a transliteration of what exists, with a menu (Mars, Deimos, the Math
building, notes).

## Command line

`mars.exe [options]`

| Option | Effect |
| --- | --- |
| `-fix` | repair the authors' bugs instead of reproducing them: the game starting in the dark, the lamp verb that cannot turn the lamp off, `south` doing nothing, `read` falling into the teleport verb, and three slips in the Math building |
| `-light` | start with the lamp on, without the other repairs |

Any other argument lists the options.

## Recommended start

`mars.exe -fix`
