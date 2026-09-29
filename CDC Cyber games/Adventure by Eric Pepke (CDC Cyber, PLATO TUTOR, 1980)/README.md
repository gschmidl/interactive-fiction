# Adventure (Eric Pepke, PLATO, 1980)

*Ninnies and Cretins: a variation on Adventure*, version 2.2, by Eric Pepke: a 48-room adventure with
a sentence parser ("open the doors with the keys"), written in TUTOR for the PLATO system at Florida
State University in 1979-80. `Adventure.exe` runs the original lesson on a built-in TUTOR interpreter
and PLATO terminal; the lost room dataset is rebuilt from data an older version kept in the lesson.
Type at the arrow and press Enter (PLATO's NEXT); HELP (F1) lists the commands. Saves are keyed by
your PLATO name.

## Command line

`Adventure.exe [options]`

| Option | Effect |
| --- | --- |
| `--name NAME` | the PLATO name to sign on as (default: your Windows user name) |
| `--course COURSE` | the PLATO course to sign on in (default `home`) |
| `--saves DIR` | where the saved state is kept (default: a `saves` folder beside the program) |
| `--seed N` | fixed random numbers (default: from the clock) |
| `--textlog` | print a transcript of the text on the screen on standard output |
| `--script FILE` | run without a window, taking the keys from FILE (for testing) |

## Recommended start

`Adventure.exe`
