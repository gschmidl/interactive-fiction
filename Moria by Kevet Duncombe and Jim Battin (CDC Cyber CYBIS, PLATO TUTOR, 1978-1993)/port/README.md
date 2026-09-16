# MORIA (Kevet Duncombe and Jim Battin, PLATO / CYBIS)

*"© copyright 1978,1984 by kevet duncombe and jim battin"*

Moria is the great PLATO dungeon game: a world of underground rooms and corridors
drawn as a first-person, line-by-line 3D view, with a city on top, four wilderness
terrains below it, sixty levels deep, guilds, spells, monsters and a party of players
travelling together. It was written in TUTOR at the University of Illinois and kept
running for twenty years; this copy is the CYBIS version as it was on 30 November
1993, maintained by Dave Runte, Andy Keller and others at CDSI.

`Moria.exe` contains the original lesson files and the game's dataset, word for word
as they sit on the NOS 2.8.7 CYBIS pack `DQ24_PUB1`, and runs them on the TUTOR
interpreter shared with the other PLATO ports. The TUTOR source is not translated
into another language.

| Lesson | What it is |
| --- | --- |
| `0moria` | the entry lesson: title picture, options page, characters, hall of fame |
| `0moriag` | the game: mazes, monsters, guilds, shops, spells, messages |
| `0moriad` | the `-use-` lesson: every define set, the character I/O, the utilities |
| `0moriah` | the Moria Guide Book (the help lesson) |
| `0moriac` | the 8,000-word common: monster, item, guild and plotter tables |
| `0moriads` | the dataset: 135 records of 64 words, ten characters to a record |

## Playing

Double-click `Moria.exe`. Press NEXT (Enter) at the title picture to reach MORIA
OPTIONS.

Your character belongs to your Windows user name, the way a PLATO character belonged
to a signon. `Moria.exe --name someone` plays as somebody else.

| Options page | |
| --- | --- |
| `C` | create a character (four ways to split the starting skills) |
| `R` | run your character |
| `I` | information on your character |
| `P` | change your pseudonym |
| `D` / `T` | destroy / transfer your character |
| `H` | the Moria Guide Book |
| `U` `G` `A` `F` | user list, guild info, achievements, finders of the ring |
| DATA (F2) | the credits |

**Moving.** The maze view is the six-by-three picture in the middle of the screen.

| Key | |
| --- | --- |
| `w` | one step forward, if the square ahead is open |
| `W` | through the door ahead; with no door, run forward until something interesting |
| `a` `d` | turn 90° left / right |
| `x` | turn around |

**In peace.** `J` join a group, `L` leave it, `R` run to a group member, `K` kick a
member out, `S` secure your string, `F` follow it to the end, `G` give food, water or
an item, `I` info on an item, `T` throw an item away, `U` use an item, `c` cast a
peacetime spell, `C` camp options, `n` replot the maze, `p` pray, LAB (F3) the player
list.

**Always available.** `m` message to everyone in the room, `M` whisper to a group
member, `Y` yell to any player in the game.

**In a fight.** `f` fight, `c` cast, `p` pray, `e` evade, `r` run, `t` trick, `h` yell
for help, `m` message, `M` whisper.

| PLATO key | PC key |
| --- | --- |
| NEXT / shift-NEXT | Enter / Shift+Enter |
| BACK / shift-BACK | F9 / Shift+F9 — **shift-BACK saves and leaves** |
| DATA / shift-DATA | F2 / Shift+F2 (stats / replot) |
| HELP / LAB | F1 / F3 |
| STOP / shift-STOP | F10 / Shift+F10 |
| ERASE | Backspace |

Your character, the world and everyone's records are kept in `saves\` next to the
program: `0moriac.common` (8,000 words) and `0moriads.dataset`.

## What was found in the game

- **The tables are the ones the game was last played with**: the monsters, the magic
  items, the guild data and the plotter table in `0moriac`'s common block are as the
  CYBIS system left them.
- **664 characters are still in the dataset**, in 130 of its 135 records: their skills,
  their gold, the magic items they carried and the square of the maze they were standing
  on. They cannot be played — a character belongs to the hash of a PLATO signon and group
  — and their names and experience were already gone before the pack was made, cleared by
  the game's own intersystem transfer. The counters agreed: one entry, two games played.
- **The game reset itself on first start**, as it was written to: `newsys` in `0moriad`
  notices that the system id no longer matches, zeroes the entry statistics, re-hides the
  reaper's ring and returns to the title page.
- **The 1993 maintenance is all there**, including the update notice for characters
  created before it ("Welcome! Things have changed a bit...") and the lines marked in
  the source with the editor's name.

## Differences from the original

- **You are always alone.** Moria had fifty player slots, groups with a guide,
  messages, whispers and yells between players. All of it runs, but nobody else is in
  the game, so joining, whispering and yelling find no one.
- **The notesfile** (`N` on the options page) was a PLATO group notes file (`0morian`),
  not a lesson. It is in `..\src_original\` but cannot be read here, so `N` does
  nothing.
- **The mazes are not the same mazes.** Moria generates every room, stairway and
  monster from a seeded random number generator (`-seed-`), so a level looks the same
  each time you come back to it. CDC's generator is not reproduced here, so the world
  is generated consistently but differently from the 1993 one. One visible consequence:
  a new character starts on the square the game calls the stairs to the Wilderness, but
  the stairways are now somewhere else in the city, so you have to walk and find one.
- **The © sign** on the copyright line is blank, as in the other ports.
- **Pauses keep their original lengths**, and the monsters still attack on the game's
  own timer while you think.

## Making it run

Moria is the hardest of these games to run. It needed, on top of the CYBIS TUTOR the
other ports already used:

- **Array operations.** CYBIS arithmetic on whole arrays: `Guide « select(Guide=me,3,Guide)`
  assigns to fifty slots at once, and `Sum(Busy*(Guide=myguide))` adds up a whole
  column. Also `arrayseg`/`arraysegv` (arrays of bit fields) and arrays with bounds,
  `Map(0;6)`.
- **CDC floating point in bit operations.** The name hash is `sin()` values combined
  with `$diff$` and `$ars$`, which only works on the CDC 60-bit floating format
  (1.0 = 17204000000000000000), so the interpreter now uses those bits.
- **Negative shift counts.** On the CDC a negative `$ars$` shifts left circularly; the
  compass directions (`mod4`) depend on it.
- **`-seed-`** for repeatable rooms, `-timel-` for the monsters' clock, `-transfr-`,
  `-findall-`, `-in-`, `-text-`, `-cdate-`, `-putv-`, `-leslist-` and `<n>` lesson
  references, `-initial-`, `-dataop-`, `-stop-`, `-notes-`, `-abort-`.
- **Graphics**: `-rorigin-`/`-rdraw-` for the castle on the title page, `-gorigin-`,
  `-polar-`, `-gvector-` and `-gdraw-` for the clock on its tower, and `-paint-` for
  the coloured floors.
- **The CYBIS alternate font.** A CYBIS terminal keeps loadable characters in its own
  slot order, which is not the TUTOR code order: a space is slot 0 (blank), `(` is slot
  0144, and ACCESS a..o take the free slots 047, 051, 052, 053, 055... Without that the
  3D view is full of stray picture pieces. The order was read from the `showchars` and
  `standard` character sets in the CYBIS library lesson `charsets` and checked against
  Moria's own monster list.
- **Unit arguments that come back**: `do day2mo(today; ind)` with `-return-`, `args`,
  and names with dots in them (`Dw4.1`, `v0.to.v1`).

## Source and build

`..\src_original\` holds the seven files as `.words` (every word, read by following the
NOS track chain) and decoded `.txt` listings. The engine is shared with the other PLATO
ports, in `D:\tools\IFBackup\_PLATO_work\engine`. `build.bat` regenerates
`src\game_data.c` and relinks.
