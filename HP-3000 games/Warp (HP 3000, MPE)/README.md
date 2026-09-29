# Warp (HP 3000)

*Warp* by Rob Lucke and Bill Frolik (HP Corvallis; version 38): "the paradoxical world of
Warp awaits you with intrigue, suspense, and adventure". A large Pascal/3000 adventure with a rich
parser - macros, a counter, conditional commands and built-in help on many topics (`HELP`) - that
circulated among HP 3000 sites for years. The original MPE program and its data files are built into
`Warp.exe`, which emulates an HP 3000 Series III and the parts of MPE the program uses.

Warp can turn players away during working hours, which the local Warpmaster sets with `SETHOURS`;
this copy lets you in at any hour.

## Command line

`Warp.exe [options]`

| Option | Effect |
| --- | --- |
| `--about` | say what the program is |
| `-u` | set the clock's hour to 14, which lifts the opening hours of the HP 3000 games that keep them; this one needs none |
| `--trace` | write an instruction trace to standard error |
| `-h`, `--help` | list the options |

## Recommended start

`Warp.exe`
