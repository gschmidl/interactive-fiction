# Caves (HP 3000)

A small BASIC/3000 game from an HP 3000's games account: you are lost in the famous Duzzledorf Caves
with your food run out, and there is only one path out. Each cavern lists the caverns it leads to;
answer with a cavern number, and questions with 1 or 0. The original MPE program is built into
`Caves1.exe`, which emulates an HP 3000 Series III and the parts of MPE the program uses.

## Command line

`Caves1.exe [options]`

| Option | Effect |
| --- | --- |
| `--about` | say what the program is |
| `-u` | set the clock's hour to 14, which lifts the opening hours of the HP 3000 games that keep them; this one keeps none |
| `--trace` | write an instruction trace to standard error |
| `-h`, `--help` | list the options |

## Recommended start

`Caves1.exe`
