# Legend — Norsk Data ND-100, SINTRAN III — native Windows port

    legend.exe

`legend.exe` is an ND-100 minicomputer with just enough SINTRAN III in it to
run one program: **LEGEND v10.0**, Magnus Lundin's multi-player fantasy game
in ND BASIC for the Swedish computer club DNF, "(C) A Hedström & M Lundin
84-87".  The source (22 May 1987), the rooms and the club's players survive
on the author's own backup floppies.  The program was compiled from that
source with the club's BASIC compiler and linked with its libraries, on
SINTRAN III, and what runs is that program, instruction for instruction, on
an emulated CPU.  v10.0 was three days old; the port fixes the bugs that
stopped it being played (see *The port's fixes* below).

           Legend    v10.0         20.14   17/ 9 1998

    Din signatur:

The game is in Swedish.  (The year is SINTRAN's: the reference machine runs
28 years behind the calendar, so the weekdays agree.)

## The game

"Det här spelet går ut på att döda och slå ihjäl folk, så mycket man bara
kan, bli stor, stark och duktig, och helst av allt: utrota ALLA fiender!"
Two brotherhoods are at war: the noble **Orden** and the bloodthirsty
**Klanen**.  You create players, wander a village of a hundred rooms (a
church, an inn, a market street, a shop with a bank and a temple, cellars
where you need a torch), fight what jumps out at you and the players you
meet, gain experience and levels, and at level 11 rise to a higher rank.
Other players stand where the club's members left them in 1986-87: asleep
(you may attack them), dead (carry them to the temple to bring them back),
or awake and dangerous.

- **Din signatur** — up to four letters: who you are.  Your players are
  kept under it.
- **Vilket scenario? (1-9, 0)** — nine separate worlds, each with its own
  players; 0 quits.
- **Menu**: A remove a dead player, B create a player, C choose one and play,
  D quit, E a player's status.
- **Creating a player**: *Slagskämpe* (fighter), *Lärling* (apprentice),
  *Magiker* (magician) or *Präst* (priest); Orden or Klanen; a name.  You
  start in the market street with 500 gold and two food parcels.
- At level 11 a Slagskämpe becomes *Krigare*, a Lärling *Trollkarl*, a
  Magiker *Mystiker*, a Präst *Munk*, and on up to *Konung*.

Commands (a short form in brackets):

| command | |
| --- | --- |
| `NORR` `SÖDER` `ÖSTER` `VÄSTER` `UPP` `NED` (`N` `S` `Ö` `V` `U` `D`) | go; `GÅ N` too |
| `TITTA` (`TIT`), `SE` | look |
| `UTGÅNGAR` | the exits and where they lead |
| `STATUS` (`STA`), `LISTA` (`I`) | your player; what you carry |
| `DÖDA` *name* (`DÖ`) | attack someone in the room (a prefix of the name will do) |
| `BLIXT` *grade* (`BLI`) | throw lightning, grade 1-5, costs magic (Lärling, Magiker and their ranks) |
| `HELA` (`HEL`) | heal yourself (Lärling, Präst and their ranks) |
| `ÄT`, `TÄND` | eat a food parcel; light a torch |
| `PLOCKA` *name* (`PL`), `TAPPA` | pick up a dead player, put him down |
| `VILKA` (`VIL`), `VEM` (`VE`) | who is in this world; who is playing |
| `BYT` | change your surname (costs gold) |
| `VÄLJ` | switch to another of your players |
| `SOVA` (`SOV`), `SLUTA` (`SLU`) | stop playing, asleep or awake |
| `AVLIVA` | end a player of yours for good |
| `POST` | write to "The Boss" (kept in `data\LEGEND-MSG-LU.DATA`) |
| `HJÄLP` (`?`) | the command list |
| `CLS` | clear the screen |

When something *hoppar fram och utmanar dig* ("jumps out and challenges
you") you cannot leave until one of you is dead; the church is a sanctuary.
Every command makes you hungrier: eat before you starve.  In the shop (north
of the start) answer `?` for its menu: food and torches, the bank, the temple
where the dead you carried in can be revived, and 0 to go back out.
Attacking a member of your own brotherhood makes you an outlaw (*laglös*).

`TELEPORT` and `POSITION` belong to the high ranks.  `PAUS`, `TID`, `@` (a
SINTRAN command) and `*` (log out) are there too; the port has no SINTRAN
commands to run and no one to log out, so `@` does nothing and `*` ends the
game.

## Keys

| key | effect |
| --- | --- |
| letters | in any case: SINTRAN turns them into capitals where the game wants them (`@TERMINAL-MODE`) |
| Å Ä Ö | the Swedish `] [ \` (small ones become capitals too) |
| Enter | ends the line; on an empty command line it repeats the last command |
| Backspace | rubs out the last character (ND BASIC's DEL) |
| Ctrl-Q | rubs out the whole line |
| Esc | nothing: LEGEND keeps it switched off |

## The port's fixes

v10.0 had been rewritten from v9.11 days before these backups were made, and
some bugs were older still.  The port fixes these in the BASIC source
(`basic\`, every change marked `Port fix 2026` and listed in
`basic\fixes.diff`), compiled again with the club's compiler:

- **A world with a dead player in it could not be loaded again** ("\*\* Error
  271", Bad character on input): the dead were saved with an empty (NUL)
  line where v9.11 wrote 0.  Every one of the nine worlds has dead players.
- **No two-word command worked** (`DÖDA RAMBO`, `PLOCKA X`, `BLIXT 3`): the
  line was split at the wrong place (`I` instead of `I19`).
- **After any two-word command a fighter could not attack again**, and
  lightning was never thrown: the lightning flag was set for every command
  *but* `BLIXT`.
- **Five short forms gave "Hmmmmmmm, nåt mystifikuskt fel"** (`SLU`, `HEL`,
  `PO`, `TIME`, `STOP`), and `POSITION` was never recognised: an off-by-one in
  the command table.
- **`TAPPA` never put anything down** (it compared *Ta Mig Med Snabeln* with
  *TA MIG MED SNABELN*), and a body you carried stayed behind if it was the
  world's first player.
- **The shop compared amounts as text**: "Hur många (1-10)?" took only 1 or
  10, and 95 gold could not be put in the bank.
- **Every Magiker was shown as *Laglös*** in the menu's status, and **a wrong
  class number added an empty player** to the world.
- **Rising in rank at level 11 made the wrong class** (a Slagskämpe became a
  Präst).
- **`VÄLJ` switched to a player who did not exist**, and **`POSITION` never
  asked which position**.
- **Quitting a full world (20 players) without a player of your own ended in
  "\*\* Error 332"**.

And one fix outside LEGEND: the club library that survives (1985) has
`GUSNA` take the wrong argument, so the game never learnt which SINTRAN user
was playing and threw out every member on its list of guests — the author,
`LU`, among them.  The library is fixed by one word (NOTES.md).

`legend --prog build\original\LEGEND-LU.PROG` plays the recovered source as
it was, linked with the library as it was, bugs and all.

## Options

| option | meaning |
| --- | --- |
| `-d`, `--data DIR` | the game's files (default: `data\` next to the program) |
| `--swedish` | show the national characters as Ä Ö Å ä ö å Ü ü É é (default) |
| `--ascii`, `--norwegian` | show them as `[ \ ] { \| }`, or as Æ Ø Å æ ø å |
| `--raw` | pass the terminal bytes through untranslated (the tests use this) |
| `--no-hold` | do not wait where the program pauses |
| `-Z`, `--clock SECONDS` | fix the clock at SECONDS since 1970; RANDOM no longer stirs in the time, so the same typing plays the same game |
| `--terminal N` | the terminal number the game shows (default 1) |
| `--prog FILE` | run another one-bank `:PROG` file |
| `-v`, `--verbose` | log monitor calls on standard error |
| `-T`, `--trace` | trace every instruction on standard error |
| `-h`, `--help`, `--version` | |

The worlds are kept in `data\`: players, the who-is-playing file, the
accounting log and the letters to The Boss.  To start again from the club's
worlds, run `python tools\make_data.py`.

## Files

| | |
| --- | --- |
| `legend.exe` | the port (`make`, gcc) |
| `data\LEGEND-LU.PROG` | the program: `basic\LEGEND-LU.ZYMB` through the BASIC compiler, linked by NRL with the club library and ND BASIC's run-time library (both in `build\`) |
| `data\RUMFIL-n-LU.DATA` | the rooms (the village is RUMFIL-8; 0-4 hold an unfinished wilderness nothing leads to) |
| `data\SPELARE-n-LU.DATA`, `SAKKARE-n-LU.DATA` | each world's players and things (there are no things); the members' own names are blanked to `Namnlös`, the game's own word for it (NOTES.md) |
| `data\INSTR-LEGEND-LU.DATA`, `HELPFIL-LU.DATA`, `BROAD-LEGEND-LU.DATA` | the instructions, `HJÄLP`, and the message from the games master |
| `basic\` | the source with the port's fixes, and `fixes.diff` |
| `..\src_original\` | LEGEND v10.0 as recovered (`LEGEND-LU.ZYMB`), v10.0 of 19 May and v9.11, the data as the author left it, his mode files |
| `build\` | the compiler's listing and object file, the rebuilt `BASLIBR-H00.BRF` and fixed `LIBRARY-MJ.BRF`; `build\original\` the program from the source as recovered |
| `src\` | the emulator (shared with the Skattejakt, SVHA and Mordor ports) |
| `tests\run.py` | replays games recorded on SINTRAN III and compares every byte |
| `tools\` | the random player, the reference machine's drivers, the BRF tools, the data converter |

How the program was rebuilt — including the sector of the BASIC run-time
library that was lost from the only floppy that has it — and what SINTRAN III
the program needs: `NOTES.md`.
