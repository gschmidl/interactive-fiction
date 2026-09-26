# GoogleAdventure - C port

A from-scratch C99 rewrite of `GoogleAdventure.exe`, replacing the original
73MB self-contained .NET 6 build with a single ~90KB native executable that
has no runtime dependencies at all.

## What the original actually was

Despite the filename and the "python compiled to .exe" assumption that
kicked off this port, the shipped `GoogleAdventure.exe` was not Python/
PyInstaller - it had no PyInstaller cookie or `python3x.dll` reference
anywhere in the binary. It was a **.NET 6 self-contained single-file
apphost**: the 73MB is almost entirely the bundled CoreCLR runtime and
base class library, wrapped around one small managed assembly (internally
named `DoogleAdventure.dll`, ~48KB) that contains the actual game.

## How it was ported

1. Parsed the .NET single-file bundle manifest by hand (the well-documented
   32-byte bundle signature + header format from `dotnet/runtime`'s
   single-file-bundler design doc) to pull `DoogleAdventure.dll` back out
   of the apphost, since `DOTNET_BUNDLE_EXTRACT_BASE_DIR` only extracts
   *native* dependencies to disk - pure-managed single-file apps load their
   assemblies straight from memory, so nothing lands on disk to grab.
2. Decompiled that DLL with `ilspycmd` (the ILSpy command-line decompiler,
   installed as a `dotnet tool`), which produced clean, readable C# for
   `Game.cs`, `Item.cs`, `Obstacle.cs`, `Room.cs`, and `Program.cs`.
3. Transcribed that C# 1:1 into `src\googleadventure.c` - same data
   (13 items, 12 obstacles, ~80 rooms, 63 command aliases, 5 collectible
   letters), same control flow, same command parsing rules.

## Design notes / faithfulness

- All game data (item/obstacle/room text, exits, hints) is copied verbatim
  from the decompiled source. Nothing was rewritten, trimmed, or "improved."
- Dictionaries (`Dictionary<string,T>` for items/obstacles/rooms/aliases/
  exits) became small fixed-capacity arrays with linear, case-insensitive
  search - there are only ~80 rooms and a dozen items/obstacles, so this is
  both simpler and plenty fast for a turn-based text adventure.
- `System.Random`'s "why" flavor-text picker became `rand()`; `Stopwatch`
  became `clock()`. Neither is observable by a player beyond "some number
  of seconds" in the win screen, so exact algorithmic parity didn't matter.
- One real gotcha: .NET's `Console.WriteLine` only ever appends `\r\n` as
  its own terminator - a literal `\n` embedded inside a C# string constant
  (used for blank lines within multi-line obstacle text) passes straight
  through as bare `\n`. Windows' C runtime, however, translates *every*
  `\n` byte to `\r\n` by default when stdout is in text mode. To match the
  original byte-for-byte, stdout is switched to binary mode on Windows and
  every line-terminating `\n` in a `printf` format string was replaced
  with the `EOL` macro (`"\r\n"` on Windows, `"\n"` elsewhere); the `\n\n`
  bytes embedded inside the actual obstacle-text data are left untouched,
  exactly like the original.
- A pre-existing game-design quirk was kept as-is rather than "fixed":
  typing `use` at the lockbox (room `5736`) burns the obstacle and shows
  the clue text but does *not* award the `red o` letter - only the
  `e1337` command does that, and it requires the lockbox obstacle to
  still be present. This is exactly how the original behaves (verified
  against it), so the port reproduces it faithfully rather than papering
  over what might look like a bug.

## Colour

The browser version of the game paints the five letter friends in the Google
brand colours wherever their names appear in the prose. The port reproduces
that with ANSI SGR sequences: `red o` and `red e` in #EA4335, `yellow o` in
#FBBC05, `blue g` in #4285F4, `green l` in #34A853, plus the `G` that marks
you on the map (you are, after all, the big blue G).

Rather than tagging strings individually, every line of output goes through
one colouriser (`gprintf` -> `colorize` in `src\googleadventure.c`) that
matches the friends' names as whole words. The letter half has to be lower
case to match, so the intro's "big blue G" - the player, not the friend -
stays uncoloured, exactly as in the browser original. Flip
`g_matchCaseOfLetter` to 0 if you want that one blue too.

Colour is on by default when stdout is a console, and off when output is
redirected to a file or a pipe, so scripted diffs against the original stay
byte-for-byte clean. Windows VT processing (`ENABLE_VIRTUAL_TERMINAL_
PROCESSING`) is switched on at startup, so plain `cmd.exe` on Windows 10/11
works with no `chcp` or registry changes.

| Flag / env | Effect |
|------------|--------|
| *(none)* | auto: 24-bit colour on a console, plain text when redirected |
| `--color` | force colour on (even when redirected) |
| `--basic-color` | use the 16-colour palette instead of 24-bit truecolor |
| `--no-color` | plain text |
| `NO_COLOR=1` | plain text (overridden by an explicit `--color`) |

Stripping the SGR sequences from a coloured run yields exactly the original
uncoloured byte stream - that is checked as part of the verification below.

## Verification

Built and diffed against the original `GoogleAdventure.exe` running under
the same scripted stdin, byte-for-byte identical in every case tried:

- A full win playthrough (all 5 letters, every obstacle chain including
  the Precision/Recall "leaves" mechanic, the `OpensExit` mechanic, and
  the 5-obstacle trade chain in the cafeteria room) - identical output,
  including the exact `"It took 69 actions and 0 seconds"` line. This is
  the same 71-command sequence documented in `WALKTHROUGH.md`.
- Movement, the ASCII map renderer (multi-floor, "map" item full-reveal,
  `@`/`G` markers), inventory, grab/use error paths.
- `hello`, `wait`, `quack` (context-sensitive to room `5043`), `wrong`
  code entry, `help`, `exits`, `friends`, and `quit`/`q`.
- After the colour layer was added: the same 71-command playthrough with
  colour off is byte-identical to the pre-colour build, and the `--color`
  run is byte-identical once the SGR escapes are stripped out.

## Building

Requires any C99 compiler. Tested with a native Windows MinGW-w64 gcc
(the one bundled with Strawberry Perl, GCC 15.2) - no WSL2 needed, and
no separate runtime to ship:

```
build.bat
```

produces `bin\googleadventure.exe`.

## Files

- `src\googleadventure.c` - the entire game, single file (the ANSI colour
  layer sits at the top, above the game data).
- `build.bat` - compiles it.
- `bin\googleadventure.exe` - built output (not checked in by this repo
  layout convention, but present here for convenience).
- `WALKTHROUGH.md` - a full win path (71 commands), with an item/obstacle
  table and a line-by-line annotated version.
