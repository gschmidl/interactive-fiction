# Port: dungeon version 0.1 (MacLisp, TOPS-20, 1982)

`dunnet.exe` is a small interpreter for the MacLisp the game is written in, with the archived source
built in. It loads `src_original/foo.lsp` unchanged, then (unless `--no-fixes`) `src/fixes.lsp`, and
calls `(dungeon)`, as a player typing `(dungeon)` at MacLisp would have.

## The source

`src_original/` is Ron Schnell's GitHub repository `Quogic/DunnetPredecessor` at the commit named in
`COMMIT`. The files turned up at MIT in 2017, in a backup of Schnell's user directory found by
Brad Parker. `foo.lsp` is the game:
several files run together, with the room data, the parser, the verbs and the TOPS-20 console
(`tops20`). `tops20.lsp` is a shorter version of the console alone (no FTP, no DIRECTORY) and is
not used. The source holds one control character, the ^G in `conn-term`'s `(princ "^G")`, which rings
the bell.

The game loads libraries that are gone: `isis:comred` (the interface to the TOPS-20 COMND JSYS),
`isis:macros`, `isis:chars`, `isis:misc` and `<nessus.s.lisp.lib>plib`. The interpreter provides what the
game uses from them; `LOAD` and `VALRET` do nothing.

`reference/comred.1` is Lars Brinkhoff's 2018 stand-in for COMRED, written so the game would run
on ITS (`PDP-10/its`, `src/libdoc/comred.1`). It documents the API the game needs. It is not the
original and does not complete with ESC.

## MacLisp

The interpreter covers the forms and functions `foo.lsp` uses, with MacLisp's behaviour where the
game depends on it. Each point below was checked on real MacLisp (LISP 2156 on ITS, built from
`PDP-10/its` in SIMH):

- Numbers are read and printed in octal (`ibase` and `base` 8). The room numbers in `foo.lsp` only
  make sense that way (`;10` labels the 8th room, `(move 10)` is southwest), and the score prints
  as "10 points out of a possible 10". A trailing point means decimal; 8 and 9 are taken as digit
  values (`19` = 17).
- A dot inside a token splits it: `(ne .ne)` reads as `(NE . NE)` and `(get.take)` as
  `(GET . TAKE)`. That is why `ne`, `up`, `north`, `south`, `west`, `score` and `get` work.
- Every variable is dynamically bound: `score` prints the `turns` counter of `dungeon`'s DO loop.
- `/` quotes the next character, also inside strings ("N//S passage" prints "N/S passage").
- `EXPLODE` of a symbol that needs quoting gives `|`, the characters, `|`. `list-words` relies on
  that: it drops the first and last character of the line's EXPLODE.
- `READLINE` returns the line as an uninterned symbol, without the line end.
- `PRINC` returns T. The inventory command does `(princ name (crlf))`, so CRLF's value, T, is
  passed as the output file, and T is the terminal. Any other file argument is the error
  "LOSING OUTPUT FILE SPECS".
- Small fixnums are EQ. `CAR`/`CDR` of NIL is NIL, of another atom an error ("ILLEGAL DATUM").
  `MEMBER` of an atom is an error ("ARGUMENT MUST BE A PROPER LIST"): `examine` in a room without
  objects runs into it.

A Lisp error prints MacLisp's message, for instance `;PLIB:SASSOC UNDEFINED FUNCTION`. MacLisp
would then stop in a breakpoint. The port abandons the command instead: errors inside `listen`,
`list-words` and `parse` (the command, and the parsing of the line) and inside the `~` interrupt
handler end only that call, so the game carries on with the next command.

## COMRED

COMRED gave the console the feel of TOPS-20, where the COMND JSYS parses a command field by field
while it is typed. The port follows COMND's conventions:

- `(let-comred prompt body)` starts a command line: it prints the prompt and runs the body once.
  It is a block for RETURN (`tops20` returns from it).
- `(comred spec)` parses the next field: a keyword of the spec, abbreviated to any unique prefix,
  or the built-in `confirm` (end of line) and `text-string` (one word, upcased).
- ESC completes a keyword; the following `(comred-force-guideword text)` then shows "(text)".
  A guide word typed in parentheses is skipped.
- `?` lists the keywords that match what has been typed, in COMND's sorted order, under the spec's
  help text, and retypes the line.
- DEL deletes; back into a field already parsed, the line is parsed again from the start (COMND's
  reparse), and a guide word goes as a whole. ^U starts the line again, ^W deletes a word, ^R
  retypes.
- A field that does not match prints "?" and the spec's error text, and the command starts again
  at its prompt. An empty line just prompts again.

Everything here is COMND convention, not knowledge of the lost library: its exact messages and
the layout of its help listings are not known.

## Fixes (`src/fixes.lsp`)

Each is the original function with the marked change, loaded after `foo.lsp`; `--no-fixes` leaves
them out.

- `list-words`: a single word in capitals ("N") was an error, because MacLisp prints such a line
  without bars and the first and last characters dropped are then letters. Letters and digits now
  form the words, anything else separates them.
- `take`: "take all" in the solar room looped forever on the silicon and the symbol, which cannot be
  taken. It now takes each movable object once.
- `examin`: examining something not carried in a room without objects called MEMBER on 0.
- `conn-term`: called READCHR, which does not exist, so "~c" never left the dungeon. Now READCH.
- `console`: set F instead of FOO in one case, so the imp could seem up after the chaosnet
  connection was dropped.
- `tops20`: DIRECTORY left FILES unset (an error) with nothing carried, and a stale count after the
  inventory shrank.

Left as they are, because finishing them would mean inventing: FTP `connect` asks for hosts from a
command spec (`ftpcon`) that was never defined, so every host is refused; FTP `send` calls
`plib:sassoc` and reads `hostroom`, neither of which exists; `int` and `(tops20 'back)` are never
called; the check for `'bally` (MIT-SALLY) never matches.

## Checked against MacLisp

`foo.lsp` was typed into MacLisp on ITS (only its library loads changed, the ^G left out because
it quits the ITS reader) and played with the same commands as the port in `--no-fixes` mode. The
transcripts agree, apart from ITS display artifacts and one extra "I don't know that word!" at the
start: started from the MacLisp prompt, the first READLINE returns the rest of the `(dungeon)`
line, which also puts the turn count one ahead.

The dungeon was compared the same way, with a test-only CONSOLE on ITS that puts the player in
the dungeon's first room: the whole walk to the kitchen, the score and QUIT agree.

The TOPS-20 part itself could not be compared: ITS has no TOPS-20 console, and with the 2018
stand-in COMRED the game cannot leave `TELNET>`. That library's LET-COMRED repeats its body until the body
returns something other than NIL, while the game's TELNET loop only ends if the body runs once,
as in this port.

## Building and testing

`build.bat` (or `build.sh` with mingw-w64 and python3) embeds the two Lisp files with
`src/mksources.py` and compiles `src/dunnet.c` into a static `dunnet.exe`.

`tests/consoleplay.py` runs the game in a hidden Windows pseudo console with real key events: the
prompt, line editing, `?`, ESC with guide words, reparse after DEL, an error, TELNET to the dungeon,
"~c", QUIT and Ctrl-Z.
