# DOD — Norsk Data ND-100, SINTRAN III — native Windows port

    dod.exe

`dod.exe` is an ND-100 minicomputer with just enough SINTRAN III in it to
run one program: **DOD**, a short Swedish dungeon game in ND BASIC by the
same ENB who wrote ADVENTURE-ENB, from the floppy TROTYL (1983).  It was
compiled from its source with the BASIC compiler of the Swedish computer club
DNF on SINTRAN III, and what runs is that program, instruction for
instruction, on an emulated CPU.

                         *************************
                         ***********DOD***********
                         *************************

                   VILL DU HA INSTRUKTIONER(J/N)?

## The game

"Because of the mad hermit's spells, you and your company have ended up in
the middle of the caves of evil.  Your task is to find the way out and
survive.  You are fifteen, all armed to the teeth."

Each turn leads somewhere at random: the harpies' chamber, Medusa's, the
trolls' hall, the orcs' headquarters, a hall with a river to cross, or
nowhere in particular; a giant demon may fall on you on the way.  Sooner or
later a passage leads out, and every one of the company still alive then is
a point.

Answer each question with one of:

| | |
| --- | --- |
| `F` | framåt, forward |
| `H` | höger, right |
| `V` | vänster, left |
| `A` | attack |
| `M` | the magic lightning (it only works now and then) |
| `FLY` | flee (only in need) |
| `TA` | take |

At the river: `E` one by one, `A` all at once.  Letters work in either case
(the terminal is in capital-letter mode, as the game knows no small ones);
the Swedish letters are shown as themselves (`--ascii` shows them as
SINTRAN stored them).

The source holds more than the game ever reaches: giant rats, a treasure
chamber, sleeping zombies, the zombies' leader and a balrog in the temple of
evil, the wizard Ostomar's tower, the vampires and the hydra's lair.  The
lines that led to them (530-560) are lost; the port does not make them up
again.

## The port's fixes

The source as recovered had two copies of lines 570-590 and was missing line
3120, the way out.  The port fixes those and the bugs a player could not be
expected to understand (`basic\`, each marked `Port fix 2026`; details in
`NOTES.md`):

- the way out led to the lost line 3120, and so into whatever lay in memory
  below the program (on the reference machine, a question for the terminal
  type, and the game gone).  Now it leads to "DU ÄR UTE I FRIHETEN" and the
  points, which nothing else reached;
- crossing the river one by one, the dead of every crossing before were
  counted again, so a company could be "all dead" with people left; and
  every one crossing had the company's number, not their own;
- killed to the last by the trolls, the harpies or the orcs, the game ended
  without a word (now: "NI ÄR ALLA DÖDA", the game's own words, which
  nothing reached);
- going on past Medusa, "five died", and none did.

`dod --prog build\original\DOD-ENB.PROG` runs the program as recovered.

## Options

| option | meaning |
| --- | --- |
| `-d`, `--data DIR` | where the program is (default: `data\` next to the port) |
| `--ascii` | show the Swedish letters as SINTRAN stored them: `} { \| ] [ \` |
| `-Z`, `--clock SECONDS` | a fixed clock: the uptime stays 0, and the same answers meet the same dice (the tests use this) |
| `--uptime UNITS` | start the uptime at UNITS: one game for each value |
| `--raw` | pass the terminal bytes through untranslated (the tests use this) |
| `--no-hold` | do not wait where the program pauses |
| `--prog FILE` | run another one-bank `:PROG` file |
| `-v`, `--verbose` | log monitor calls on standard error |
| `-T`, `--trace` | trace every instruction on standard error |
| `-h`, `--help`, `--version` | |

## Files

| | |
| --- | --- |
| `dod.exe` | the port (`make`, gcc) |
| `data\DOD-ENB.PROG` | the program: `basic\DOD-ENB.SYMB` through the BASIC compiler, linked with the club library and ND BASIC's run-time library |
| `basic\` | the source with the port's fixes, and `fixes.diff` |
| `..\src_original\` | TROTYL's `DOD-ENB:SYMB` as recovered |
| `build\` | the compiler's listings and object files, the libraries; `build\original\` the program as recovered |
| `src\` | the emulator (shared with the other ND-100 ports) |
| `tests\` | replays of sessions recorded on SINTRAN III, the fixes shown |
| `tools\` | the build on the reference machine, the random player |

How the program was built and checked against SINTRAN III: `NOTES.md`.
