# ZENO (1983)

*An investigation of the limits to REX* — Dave Mitchell, ZENO at WINVMB,
January–February 1983.

You are alone on a COMPUZZ deep-space station. The air is going bad, the
lift does nothing, something is coming aboard, and the only thing that can
save you is the computer in the corner — which you have to program
yourself, in a language called SIMPL/E, using a full-screen editor, while
the clock runs.

This is a native Windows build. There is no emulator, no virtual machine
and no network port: `Zeno.exe` runs the author's own REXX source directly.

## Playing

Double-click `Zeno.exe`, or run it from a terminal.

| Key | Does |
| --- | --- |
| **F1–F12** | the 3270 PF keys — the legend at the foot of each screen says which are live |
| **Enter** | transmit (submit the command line, or the whole screen in the editor) |
| **Tab / Shift-Tab** | next / previous input field |
| **Arrows, Home, End** | move within and between fields |
| **Insert** | toggle insert mode |
| **Esc** | PA1 |

Everything typed goes into a field. On the room screen there is one field,
the command line at the bottom. In the editor there are twenty-five: twelve
program lines, twelve line-command boxes down the right, and the command
line underneath.

`SAVE name` and `RESTORE name` on the room command line keep your programs
and notes between sessions, in `saves\`.

### Where to start

The author's own suggested opening, from `ZENO SCRIPT (1983).txt`:

```
go to lift          press the button      read the notebook
go back to room     go to computer        active
air                 list                  e lift
help                quit                  lift
terminal            active                ENABLED
```

Four characters is enough for any noun — `butt` is the button, `note` the
notebook. There are books lying around the station, and they are the manual.

## Options

| Option | Effect |
| --- | --- |
| `-3279` | authentic 3279 terminal colours — protected blue, intensified white, input green, intensified input red |
| `-original` | run Dave Mitchell's unpatched `ZENO EXEC`, crashes and all |
| `-fast` | do not hold the start-up panel |
| `-debug` | write `zeno.log` |
| `-dir DIR` | take game files from `DIR` |

The default build applies four small fixes to the exec, listed in
`ZENOFIX.diff`. Without them a REXX error — most easily a SIMPL/E
expression that divides by zero — ends the session outright, and every
`SAVE`/`RESTORE` cycle indents all of your programs two more columns.
`-original` gives you the 1983 behaviour exactly.

## What is here

```
Zeno.exe              the game
regina.dll            Regina REXX 3.9.7, which interprets it (LGPL)
game\ZENO.EXEC        Dave Mitchell's source, unmodified
game\ZENOFIX.EXEC     the same, with the four fixes
game\ZENO.IOS3270     the panel definitions, unmodified
game\ZENO.INITDATA    starting world, programs and messages, unmodified
ZENOFIX.diff          exactly what was changed in the exec, and why
NOTES.md              how the port works, and how IOS3270 was decoded
src\                  the C source of the host, and the build
saves\                your saved games
```

The three `game\` files that are not `ZENOFIX.EXEC` are byte-for-byte the
files from the author's own package.
