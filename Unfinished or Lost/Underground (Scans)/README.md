# UNDERGROUND (Gary Kleppe, 1979)

Gary Kleppe's UNDERGROUND, a cave adventure in BASIC-PLUS for DEC's RSTS/E, recovered from scanned
line-printer listings. The playable one is the first version of March 1979 - 87 rooms and 45 objects,
its whole world in the program's DATA statements; of a later, larger version only part survives.
`underg.exe` is a BASIC-PLUS interpreter that runs the recovered source, `UNDERG.BAS` and its room
describer `LOOK.BAS`, unmodified. When you stop, the game saves to `UNDERx.DAT` and gives you a
one-letter password to resume with.

## Command line

`underg.exe [PROGRAM.BAS...]`

| Parameter | Effect |
| --- | --- |
| `PROGRAM.BAS...` | the programs to load (default `UNDERG.BAS` and `LOOK.BAS` from the current directory) |

Environment variables: `BPKEEPCASE=1` stops the folding of typed input to capitals (the Y/N question
and the password then accept capitals only, as on the original upper-case terminal); `BPTRACE=1`
prints each line number as it runs.

## Recommended start

`underg.exe`, started in its own folder (double-clicking it does that).
