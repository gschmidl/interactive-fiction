# WELL0550 Windows port

`src_original\` is David Welbourn's transcription of Ken Wellsch's sources and A-code database as
downloaded (plover.net/~davidw/uwadventure.html, transcribed March 2015 from 1986 printouts).

`port\` builds them for Windows with MinGW-w64 gcc (`build.bat`, or `build.sh` from an sh):

- `src\portcompat.h` is force-included into every file: `index`/`rindex`, `srandom`/`random`,
  `getuid`, and binary `open`/`creat` for the key and record files.
- Three source changes, each marked "Windows port": `MUNGE\FIO.C` fills in `stdin`/`stdout`/`stderr`
  at first use (they are not constants on Windows), `ADV\CACHE.C` declares its static functions
  before use, and `ADV\EXEC.C` declares `clock` as `time_t`.
- `ADV\ADV.C` gains `-u`: the player starts as a wizard (bit OLORIN of ADMIN), the game's own way
  past the 30-minute wait after `SAVE`. `bin\run.bat` passes it.
- The opening-hours check needs no change: `prime()` always returns 0, as in the 1985 release.

`build.sh` compiles `munge.exe` and `adv.exe` into `bin\` and runs `munge` on `COMCAVE` and
`ADV_DB\`, writing `bin\adv.key` and `bin\adv.rec` (byte-identical on every build).

Tested: a short walkthrough, `SAVE` then an immediate `RESTORE` with `-u`, and 12 runs of 150
random commands without a crash.
