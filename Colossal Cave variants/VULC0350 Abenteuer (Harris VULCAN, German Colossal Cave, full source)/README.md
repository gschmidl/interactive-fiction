# Abenteuer (German Colossal Cave, 350 points)

"Suchen Sie Ihr Glück in der GIGANTISCHEN HÖHLE": Gary Palter's portable *Adventure* as Harris
Computer Systems Division adapted it in 1977, with its messages translated into German, from a
German Harris VULCAN site (1977-80). The original FORTRAN, compiled from the site's own job stream,
starts from the site's own new-game image. German commands of one or two words (`NORD`, `NIMM`,
`LEG`, `BESTAND`, `SCHAU`); only the first five letters count.

## Command line

`abenteuer.exe [options]` (`run.bat` adds `-u` and passes its parameters on)

| Option | Effect |
| --- | --- |
| `-u`, `--unlimited` | no prime time (on weekdays from 8:00 to 18:00 only wizards may play) and no 90-minute wait before a saved game goes on |
| `--seed N` | other dice |
| `--fresh` | set up from the text database instead of the site's new-game image, as when there was none: the texts as the tape has them (a later edition); the wizard question and the instructions come first |
| `--fresh=1980` | the same from the 1980 edition of the texts, from which the site's image was set up |
| `--no-fixes` | the site's game as it was: `BRING` (restore) and `MAGIE MODUS` are never taken, because the site's image already stands at turn 1 |
| `--date YYYY-MM-DD`, `--time HH:MM` | hold the clock still |
| `-h`, `--help` | list the options |

## Recommended start

`run.bat`
