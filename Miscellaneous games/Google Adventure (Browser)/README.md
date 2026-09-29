# Google Text Adventure (2018)

The text adventure Google hid in its search page's browser console in 2018: you are the big blue G,
searching the Google campus for the other letters of the logo - red o, yellow o, blue g, green l and
red e - each guarded by a chain of items and obstacles. `googleadventure.exe` is a native C port of
a console version of the game, with its text, rooms and rules unchanged and the friends' names in
their colours.

## Command line

`googleadventure.exe [option]`

| Option | Effect |
| --- | --- |
| `--color` | colour on even when the output is redirected (by default: 24-bit colour at a console, plain text otherwise) |
| `--basic-color` | use the 16-colour palette instead of 24-bit colour |
| `--no-color` | plain text (`NO_COLOR=1` in the environment does the same, unless `--color` is given) |

## Recommended start

`googleadventure.exe`
