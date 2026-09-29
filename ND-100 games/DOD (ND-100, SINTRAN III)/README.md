# DOD (ND-100, 1983)

DOD, a short Swedish dungeon game in ND BASIC by someone who signed their files ENB, from a Swedish
computer club's floppy of 1983: the mad hermit's spells have left you and your company of fifteen in
the caves of evil, and you must find the way out. Each turn leads somewhere at random - the harpies,
Medusa, the trolls' hall, the orcs, a river to cross, a giant demon - and every member of the company
still alive at the way out is a point. Answer with `F`, `H`, `V` (forward, right, left), `A` (attack),
`M` (magic), `FLY` or `TA` (take). `dod.exe` is an ND-100 with enough of SINTRAN III to run the
compiled program; the port's fixes (the lost way out, miscounted dead) are built in.

## Command line

`dod.exe [options]`

| Option | Effect |
| --- | --- |
| `-d DIR`, `--data DIR` | where the program is (default: `data` beside the program) |
| `--swedish` | show the 7-bit national characters as Ä Ö Å ä ö å, and @ \` as É é (default) |
| `--norwegian` | show them as Æ Ø Å æ ø å |
| `--ascii` | show them as the 7-bit codes `[ \ ] { \| }` |
| `--uptime UNITS` | start the machine's uptime at UNITS (1/50 s) and run it with the instructions alone: one game for each value |
| `-Z SECONDS`, `--clock SECONDS` | fix the clock at SECONDS since 1970; the uptime stays 0, so the same answers meet the same dice |
| `--no-hold` | do not wait where the program pauses |
| `--vdu` | tell the program the terminal is a screen (VT100) |
| `--terminal N` | the terminal's logical device number (default 1) |
| `--raw` | pass the terminal bytes through untranslated |
| `--debug` | debug commands on a line starting with `#`: `#peek ADDR [COUNT]`, `#poke ADDR VALUE...`, `#find VALUE... [in FROM TO]`, `#dump FILE` (octal; decimal with a trailing `.`) |
| `--prog FILE` | run another one-bank program file instead |
| `-v`, `--verbose` | log monitor calls on standard error |
| `-T`, `--trace` | trace every instruction on standard error |
| `-h`, `--help` | list the options |
| `--version` | show the version |

## Recommended start

`dod.exe`
