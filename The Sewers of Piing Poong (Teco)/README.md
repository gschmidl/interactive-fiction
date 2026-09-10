# The Sewers of Piing Poong — Windows port

A playable Python port of a tiny early-1980s text adventure recovered from
the "PDP tapes" collection (`sewer.tec` + `sewer.dat`, found alongside a
batch of DEC magtape images). The original was written in **TECO** — the
DEC text editor that doubled as a programming language — driving a small
hand-rolled flat-file "database" (`sewer.dat`) that TECO searched with its
own `S` (search) command instead of anything resembling a modern parser.

## Running it

```
python sewer.py
```

Pure standard-library Python 3, no dependencies. Verbs: `MOVE <direction>`,
`TAKE`, `DROP`, `LOOK`, `INVENTORY`, `OPEN` — all abbreviate to their first
letter, exactly as the original's help text promises ("`M E`" = "`MOVE
EAST`"). `QUIT` was added for convenience (see *Deviations* below).

## How this was reverse-engineered

`sewer.tec` and `sewer.dat` are plain 7-bit text *except* that `sewer.tec`
contains 134 real ESC (0x1B) bytes — TECO's string terminator — which
don't print and made the source look like nonsense until they were
substituted back in as a visible `$` (the convention DEC's own TECO manuals
use). With that restored, the macros decode into a small, coherent engine:

- **`sewer.dat`** is one long text buffer used as a database. Records are
  delimited by literal `^` (caret) characters, and TECO's `S`/`:S` search
  command is used with a repeat-count to jump straight to the *Nth*
  occurrence of `%` — e.g. `Q7:S%` jumps to the start of room `Q7`'s record,
  because rooms are simply `%<num>^<description>^<exits>^<room's item
  list>^` back to back. Things follow the same idea:
  `%<name>^<id>^<flags>^<description>^<unused field>^`.
- **Exits** are `@`-separated clauses like `S(3,)6F4`: direction letter,
  an optional `(a,)`/`(,a)` gate (a-in-room / a-held, checked with a
  "is thing X in this room" macro), the destination room, and an optional
  suffix — `Y<n>` prints special message *n* then still moves you, `X<n>`
  prints message *n* and **ends the game** (used for the one win and the
  one death).
- **Openable objects** (the manhole, the hatch) are stored as *two* records
  sharing a name — a closed one with an `O<n>` flag naming the required
  held item, and an open one. `OPEN` swaps which record is "active" and
  drops the open one into the room you're standing in — which is exactly
  what later gated exits check for. That's how "open the hatch" and "climb
  down the hatch" are connected.

Working all of this out turned a page of dense TECO into a fairly
charming, very solvable 38-room map with three fetch-style puzzles:
- the **keys** (catacomb, room 19) open the **hatch** (room 20) into the
  pipe maze;
- the **cheese** (pipe junction, room 27), dropped in front of the rat
  (room 35), clears the way to the **crowbar** (room 36, "heavenly light");
- the crowbar opens the **manhole** back in the starting room (room 1),
  which is also the only way to win.
(There's also a girl in the villager room (9) and an armor-gated squeeze
in the catacomb (19) — neither is required for the shortest win, but the
girl is needed to survive going south from room 8, and the armor must be
*left in the catacomb* to still fit through its east wall.)

A full walkthrough (55 moves) was traced and run against `sewer.py` to
confirm the win is reachable, and the villager exits (9 → N/W/E) were
confirmed to end the game with the death message.

## Deviations from the original (all minor, all noted for honesty)

- **`QUIT`** is a new command — the original had no way to end the session
  short of winning or dying.
- **Empty `INVENTORY`** prints "You aren't carrying anything." — the
  original TECO code would have printed nothing at all.
- **Direction case**: room 9's exit data uses lowercase `n/s/e/w` while
  every other room uses uppercase; movement matching here is
  case-insensitive, which is almost certainly the intended behavior (the
  original's search was case-sensitive, so those exits may actually have
  been dead in the original — the port treats them as live and lethal,
  matching the "villagers on three sides" flavor text).
- **Missing noun on TAKE/DROP/OPEN** prints "What?" instead of triggering
  whatever an empty search string happened to match in the original.

Original files (`sewer.dat`, `sewer.tec`) are kept one level up, in
`..\` for reference, alongside a copy of the source tape files they came
from.
