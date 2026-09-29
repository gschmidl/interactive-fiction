# ZENO (Dave Mitchell, 1983)

*ZENO - an investigation of the limits to REX*, by Dave Mitchell (ZENO at WINVMB), January-February
1983, for IBM's VM/CMS on a 3270 terminal. You are alone on a deep-space station: the air is going
bad, the lift does nothing, something is coming aboard, and the only thing that can save you is the
computer in the corner, which you have to program yourself in a language called SIMPL/E with a
full-screen editor while the clock runs. `Zeno.exe` runs the author's own REXX source with the Regina
REXX interpreter on a 3270-style screen (F1-F12 are the PF keys, Esc is PA1). `SAVE name` and
`RESTORE name` keep your programs and notes in a `saves` folder.

## Command line

`Zeno.exe [options]`

| Option | Effect |
| --- | --- |
| `-3279` | the real 3279 terminal colours (protected blue, intensified white, input green, intensified input red) |
| `-fast` | do not hold the start-up panel |
| `-original` | run the unpatched 1983 program. Otherwise four small fixes apply: a REXX error, such as a SIMPL/E division by zero, no longer ends the session, and `SAVE`/`RESTORE` no longer indents every program by two more columns |
| `-debug` | write `zeno.log` |
| `-dir DIR` | take the game files from DIR |
| `-script FILE` | run without a console, taking the input from FILE (for testing) |
| `-verify` | with `-script`, paint the real console screen (for testing) |

Any other argument lists the options.

## Recommended start

`Zeno.exe`, or `Zeno.exe -3279` for the original colours.
