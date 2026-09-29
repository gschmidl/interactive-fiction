# The Monastery (Tymshare, 1987, unfinished)

An untitled, unfinished text adventure in Tymshare BASIC for a PDP-10 under TYMCOM-X, dated 8-9
January 1987 ("The Monastery" is a working title). You arrive at a ruined monastery built against a
mountain: a burned library, a cathedral with a headless statue and a blood-stained altar, secret
doors, a wandering ghoul, a zombie, a light-fingered hobbit, a dwarf. The game rolls up a character
and keeps it in `NAME.GME` between sessions; the prompt turns from `]` to `*` in combat, where
`PUMMEL` and `GRAPPLE` do far more than armed attacks. Native Windows port of the BASIC program.

## Command line

`monastery.exe [options]`

| Option | Effect |
| --- | --- |
| `--char NAME` | load the character `NAME.GME` instead of asking for a name |
| `--text FILE` | read the room text from FILE instead of the built-in copy (a `TEXT.GME` in the current directory is used anyway) |
| `--seed N` | fixed random numbers, for repeatable games |
| `--strict` | leave `QUIT` doing nothing, as in the 1987 program, where dying was the only way out |
| `-h`, `--help` | list the options |

## Recommended start

`monastery.exe`
