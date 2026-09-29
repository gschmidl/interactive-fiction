# Mystery Mansion (HP 3000, revision 16)

Bill Wolpert's *Mystery Mansion*, revision 16, in FORTRAN/3000: you arrive by taxi at dawn at the
iron gate of an old mansion, where a murder has been done; the mystery - scene, murderer and weapon -
is drawn from the clock. The original MPE program is built into `Mansion.exe`, which emulates an
HP 3000 Series III and the parts of MPE the program uses. A saved game is written to the directory
you start it from.

## Command line

`Mansion.exe [options]`

| Option | Effect |
| --- | --- |
| `--about` | say what the program is |
| `-u` | set the clock's hour to 14, which lifts the opening hours of the HP 3000 games that keep them; this one keeps none, and a fixed hour narrows the mysteries drawn from the clock |
| `--trace` | write an instruction trace to standard error |
| `-h`, `--help` | list the options |

## Recommended start

`Mansion.exe`
