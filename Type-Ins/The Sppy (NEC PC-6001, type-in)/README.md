# The Sppy (NEC PC-6001)

*THE SPPY* (the title screen says "THE SPY 006", by YELLOW SOFT), version 2.2 by Momoki Todoroki: a
Japanese spy adventure in N60-BASIC for the 32K NEC PC-6001, with a machine-code routine and a
drawn title screen, recovered from a magazine type-in listing. Commands are short katakana words,
the commonest of them on the function keys. It runs in a PC-6001 emulator; no emulator is included.

The package holds three cassette recordings:

| Tape | What it is |
| --- | --- |
| `1 The Sppy (3P, CLOAD''BASIC'', tape 2, RUN).wav` | the game program |
| `2 Title Screen.wav` | the title-screen data the game loads when it starts ("DATA BLOAD") |
| `0 Saver.wav` | the author's small program that writes the title-screen data to tape; not needed to play |

## Command line

No parameters. Start the PC-6001 with 3 pages, play `1 The Sppy ...` and `CLOAD"BASIC"`, then `RUN`;
when the game prints `DATA BLOAD`, play `2 Title Screen.wav`.

## Recommended start

As above, in a PC-6001 emulator that plays WAV cassettes.
