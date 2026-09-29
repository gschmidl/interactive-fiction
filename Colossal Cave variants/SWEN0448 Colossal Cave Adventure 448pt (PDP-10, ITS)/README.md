# Adventure, 448 points (PDP-10, ITS)

The 448-point *Adventure*: Crowther and Woods by way of Brown University (Dave Wallace, Dave Nebiker,
Eric Albert and Les Wu, from 1978), ported to MIT's ITS by EJS in July 1979. The FORTRAN-10 source
survived, so this is the original program compiled for the Windows console, keeping the PDP-10's
packing of five characters to a word.

The cave keeps Brown's prime time: on weekdays from 12:00 to 16:59 only wizards may play, and anyone
else is sent away or offered a 30-turn demonstration game. `run.bat` lifts that.

## Command line

`adv448.exe [options]` (`run.bat` adds `-u` and passes its parameters on). It reads its database
`FT01.DAT` from the current directory, so start it in its own folder (`run.bat` does). It first asks
whether you are a wizard (say no) and whether this is a restarted game.

| Option | Effect |
| --- | --- |
| `-u`, `--unlimited` | no prime time: the cave is open to everyone at all hours |
| `-h`, `--help` | list the options |

## Recommended start

`run.bat`
