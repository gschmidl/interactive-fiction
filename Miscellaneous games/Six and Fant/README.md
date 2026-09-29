# SIX/FANT worlds (University of Alberta, 1980)

SIX was the language of the SIX/FANT system at the University of Alberta in the late 1970s for
writing games like *Adventure*; FANT was the machine that ran the compiled worlds. `six.exe` runs SIX
source directly, with four worlds:

| World | What it is |
| --- | --- |
| `adventure.6` | ADVENTURE Fantasy World by Chris Gray, last changed 6 June 1980 |
| `mansion.6` | MANSION Fantasy World by Chris Gray, last changed 27 March 1980 |
| `this.6` | a larger world, 7,418 lines, with a score limit of 619; it needs `--fix-typos` |
| `ex.6` | a small example world: a candle that burns down |

## Command line

`six.exe [options] WORLD.6`

| Option | Effect |
| --- | --- |
| `--fix-typos` | take an undeclared word for the declared name it is a near miss of (needed by `this.6`) |
| `--width N` | wrap the text at N columns (default 79; 0 = never) |
| `--seed N` | seed for the random numbers |
| `--echo` | echo input lines (for piped input) |
| `--prompt TEXT` | print TEXT before every input |
| `--input FILE` | read the input lines from FILE |
| `--check` | only parse the world and report problems |
| `--strict` | treat undeclared words and questionable operations as errors |
| `--warn` | report tolerated problems on standard error |
| `--quiet` | no start-up note about tolerated problems |
| `--max-depth N` | the deepest procedure nesting allowed (default 20000) |
| `--version` | show the version |
| `-h`, `--help` | list the options |

## Recommended start

`six.exe adventure.6`, `six.exe mansion.6`, or `six.exe this.6 --fix-typos`
