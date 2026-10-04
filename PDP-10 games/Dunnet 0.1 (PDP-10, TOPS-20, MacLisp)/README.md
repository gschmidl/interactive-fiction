# Dunnet 0.1 (dungeon, 1982)

Ron Schnell's "dungeon version 0.1 (lisp)", written in MacLisp at MIT in 1982: the game he
remembered ten years later when he wrote Dunnet for Emacs. A dirt path, an electric fence and a
special phrase lead to a computer room at MIT with a DECSYSTEM-20 console. Through its TOPS-20
command processor you telnet over Chaosnet into a dungeon with a solar room, a buried nugget of gold
and a kitchen. It is an early, unfinished version: 10 points, an "Under development" room, and an
FTP command that never got finished. The port is a small MacLisp interpreter with the archived
source built in, so the original text runs as written.

Numbers come out the way MacLisp printed them, in octal: the full score is "10 points out of a
possible 10".

At the TOPS-20 prompts (`@`, `TELNET>`, `*`) commands work as on TOPS-20: `?` lists the choices, ESC
completes a word and shows its guide word, keywords can be abbreviated, DEL deletes, ^U starts the
line again, ^R retypes it. In the dungeon, `~` followed by `c` closes the connection and puts you
back at the console.

## Command line

`dunnet.exe [options]`

| Option | Effect |
| --- | --- |
| `--no-fixes` | run the archived source exactly, bugs included: a single word in capitals is not understood, "take all" in the solar room never ends, examining things in an empty room fails, "~c" does not leave the dungeon, and DIRECTORY fails with nothing carried |
| `--debug` | report problems while loading the source on standard error |
| `-h`, `--help` | list the options |

A Lisp error in a command prints MacLisp's message (for instance `;PLIB:SASSOC UNDEFINED FUNCTION`
from the unfinished FTP `send`) and the game goes on with the next command.

## Recommended start

`run.bat`
