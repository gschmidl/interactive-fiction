# Explore 5.3 (Multics BASIC) - Windows console port

`explore.exe` runs Explore 5.3, Jim Lippard's 1980 Multics BASIC game, the way its author runs it away from Multics:
his BASIC, unchanged, on his Multics BASIC interpreter MBasic, with his runner and his native helpers - here with a
Perl of its own, so nothing has to be installed. Run `run.bat` and type `help` in the game; `explore.exe --help`
lists the options.

## The folder

| | |
|---|---|
| `explore.exe`, `run.bat` | `src\launcher.c`: starts `perl\bin\perl.exe explore.pl` with the arguments as given, in the same console, waits, and returns its exit code. It clears PERL5LIB and the other Perl variables, so an installed Perl cannot lend it modules. |
| `explore.pl` | the author's runner (`perl\explore` in his Perl distribution 1.2), with the Windows changes |
| `lib\` | the author's MBasic 1.2 (lexer, parser, executor, files, ...) and `Explore\Builtins.pm`: the helpers that were PL/I on Multics, and stand-ins for the Multics commands the game calls |
| `share\` | the game: `explore.basic`, the ten `exp_*_.basic` helpers, `explore.data`, `explore.help`, `hours.data`, `winners.data` - the author's files byte for byte (`build.sh` checks) |
| `perl\` | Strawberry Perl 5.32.1 (64-bit), cut down to what the game loads: `perl.exe`, `perl532.dll`, the three gcc runtime DLLs it needs, 40 library files and 7 XS DLLs. The licences are in `perl\licenses`. |
| `saves\` | made by the first run, and not in git: the player's files |
| `windows.diff` | every change to the author's files, against `..\src_original` (written by `build.sh`) |

## Playing

The game names its files the Multics way, and the runner maps the names:

| Multics | here |
|---|---|
| `>site>explore_dir` | `share` (only read) |
| `>site>explore_dir>private` | `saves`, the writable directory (the name `explore_setup.ec` gives it on Multics) |
| `>site>explore_dir>explore.rwdir` | `saves\explore.rwdir` |
| `>udd>Explore>Player` | `saves`, the player's home directory |

The game runs in its home directory, as a Multics process did, so a file named without `>` is there too. In `saves`:
- `hours.data` and `winners.data`, copied from `share` by the first run. A win adds the winner to the tablet in the
  cave and to the rock; the sorcerer can change the hours, the magic word and the news.
- `explore.rwdir`, written by the first run: `>site>explore_dir>private` and `^multip`. With `none` on line 1 the
  game plays read-only; `multip` on line 2 permits the multiplayer mode (players are told apart by their Windows
  account names).
- `NAME.explore`: a saved game. `save NAME` saves and ends the game; `restore NAME` brings it back and deletes the
  file.
- `ACCOUNT.explore_abbrev`: the abbreviations (`ab`, `.a`, ...; `help abbrev`), ACCOUNT being the Windows account
  name, which is the game's `usr$`. `.e` edits them in Notepad (or `%EDITOR%`).
- `start_up.explore`, if you write one: commands the game runs before it reads the keyboard.

The sorcerer's word is "hello" (`sorcerer`). The hours in `hours.data` keep the cave open, except for the minute
23:59: the game compares the time with the closing time "2359" and wants it earlier. The original control arguments
work: `-brief`, `-version`, `-no_version`, `-modes`, `-ab NAME`, `-pn FILE`, `-ts`, `-ns`. Ctrl+C ends the game,
unless `stm ^quit` switched the break key off. `..COMMAND` and `m COMMAND` run a command in `cmd` (Multics ran a
Multics command); `send` answers that it cannot send.

## The changes (`windows.diff`)

None touches the game's BASIC.

1. **MBasic, `usr$`.** Perl on Windows has no `getpwuid` (calling it dies); the account name is `USERNAME`.
2. **MBasic, `dim`.** MBasic executed `dim`: every time the statement ran it made the arrays anew, empty. Multics
   BASIC does not. "A dim statement has no effect when executed", and it "can appear either before or after the
   first use of an array" (AM82-01, the Multics BASIC manual, February 1981, p. 5-9); an array's bounds "do not
   change during execution", and its elements are set to 0 or "" "at the start of the program" (2-4, 2-5). In
   Explore the control argument `-abbrev` (`-ab`) depends on it: lines 18890-18990 read the abbreviations into
   `x()`, `x$()` and `y$()` before line 460 dims them, so MBasic emptied them again, and more than ten made it stop
   with "Subscript out of bounds" (an array that is not yet dimensioned has bound 10). Now the arrays a program
   dims are made when control enters it (the main program at the start, a subroutine at each call), and `dim`
   does nothing.
3. **`exp_home_`** returns the Multics pathname `>udd>Explore>Player`, which the runner maps to `saves`. The game
   cuts its home path at the first blank (line 45, `call "exp_before_": h9$, h9$, " "`), and a Windows path is
   often full of them - this repository's `Miscellaneous games` is one.
4. **`exp_getpw_`**, the sorcerer's hidden word: there is no `stty -echo`; `Term::ReadKey` turns the console's
   echo off and on again.
5. **`send_message`**: Unix `write` does not exist, and a list-form piped `open` dies on Windows; it says it cannot
   send.
6. **`ted`** (`.e all`, `.e last`): Notepad, or `%EDITOR%`, instead of `ed` or `vi`.
7. **The runner**: the folders beside the program, `--home`, the Multics names above, `explore.rwdir` in `saves`,
   the start in the home directory, `-h`, and output that is not held back (a prompt such as "? " shows before the
   game reads).
8. **The end of input.** From a pipe or a file it ends the game; MBasic read an empty line there, over and over,
   and the game asked for ever. A console has no end: a read cut short there - by Ctrl+Z Enter, or by Ctrl+C in
   `^quit` mode, which Windows also ends a console read with - is an empty line, as before, and the game asks
   again.

## Found in 5.3 and left alone

- `save` and `restore` without a name are "I don't know the word". The command table (lines 6230 and 6240) takes
  only `save NAME` and `restore NAME`, so the code for the default name SAVED_GAME_ (8970-9030, 9390-9430) is never
  reached, although `info`, `help save` and the author's info segment offer it. The 1980 printout has the same
  lines.
- `get all` also takes the invisible objects that stand for the abyss, the fissure and a rock (`abyss`, `fissure`,
  `rock-1`: objects with an empty description in `explore.data`, as in the 1980 listing of the 6.0 database,
  `..\doc\explore.data.6.0`), and
  they are then carried and dropped like anything else. The 2026 guard at lines 7005 and 7445 covers only `get`
  without a name; `get all` (7280-7350) has none.

## Building

`build.sh` (or `build.bat`) needs MinGW-w64 gcc and Strawberry Perl; set `SPERL` to its `perl.exe` unless it is the
first `perl` on PATH (Git Bash's own Perl is not a Windows Perl). It

1. compiles `src\launcher.c` into `explore.exe`;
2. runs `src\mkruntime.pl` on that Perl to make `perl\`. The list of files is measured: the game is played twice
   under the Perl (`src\deps.pl` records every module and DLL it loads), through most commands to a SAVE and from
   the RESTORE to QUIT;
3. checks that `share\` is the author's files and writes `windows.diff`;
4. plays a short game through `explore.exe`, SAVE to RESTORE, in a scratch folder.

## Tests

- `tests\check.py`: 24 sessions through `explore.exe` - the options and control arguments, SAVE and RESTORE,
  abbrevs (with `-ab`), the sorcerer changing the hours and the magic word, the shell escape, `send`,
  `start_up.explore`, the read-only fallback and the end of input. Then games played by `tests\drive.pl` with a
  fixed seed and clock, each twice: on the port's Perl with `lib\`, and in WSL on Linux Perl 5.36 with the author's
  unchanged MBasic and helpers from `..\src_original`. 60 random games of 150 commands (the game's own words; they
  reach 24 of the 58 rooms and score nothing), and 4 tours that use the sorcerer's `move` to visit all 58 rooms and
  take everything, kill all nine creatures with their weapons, leave the treasures in the shack and win at the
  tablet with 500 points. Every transcript is the same on both.
- `tests\consoleplay.py`: 11 checks at a real console (ConPTY): the prompt shows before a key is pressed, typing
  is echoed and the magic word is not, Ctrl+Z Enter is an empty line, and Ctrl+C ends the game in mode `quit` and
  does nothing in mode `^quit`.
- The author's own tests pass on Windows against `lib\`, run with the full Strawberry Perl: MBasic's 242 but one,
  which wants the Unix mode bits 0664 on a new file (Windows gives 0666), with `symlink` made a copy (Perl 5.32 on
  Windows has none); Explore's `08_builtins`, `09_game` and `10_abbrev`. `11_args` needs `fork`; `check.py` covers
  the arguments through `explore.exe` instead.

## Licences

The game, MBasic and the runner are Jim Lippard's, under the BSD 3-Clause licence (`..\src_original`). Perl is under
the Artistic License or the GPL; the gcc runtime DLLs are under the GPL with the GCC Runtime Library Exception, and
winpthreads under its MIT-style licence (`perl\licenses`).
