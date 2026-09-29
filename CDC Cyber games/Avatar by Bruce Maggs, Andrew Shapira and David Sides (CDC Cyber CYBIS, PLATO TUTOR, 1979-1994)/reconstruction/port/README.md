# AVATAR (Bruce Maggs, Andrew Shapira and David Sides, PLATO / CYBIS)

*"Copyright 1979, 1980, 1981, 1982, 1983, 1984 — Bruce Maggs, Andrew Shapira, David Sides,
Tom Kirchman, Greg Janusz, Mark Eastom"*

Avatar is the largest and longest-lived of the PLATO dungeon games: a city of shops, banks,
guilds and a morgue above fifteen levels of dungeon, with twelve guilds, ten races, hundreds
of monsters, items and spells, and up to fifty players in the world at once. It was written
at the University of Illinois from 1979 and kept running for fifteen years; this copy is the
CYBIS version as it was on 7 March 1994, maintained by Dave Runte at CDSI.

`Avatar.exe` contains the original lesson files, the game's four namesets and its dataset,
word for word as they sit on the NOS 2.8.7 CYBIS pack `DQ24_PUB1`, and runs them on the TUTOR
interpreter shared with the other PLATO ports. The TUTOR source is not translated into
another language.

| File | What it is |
| --- | --- |
| `2avat` | the title page: M. St. Clair's dragon, drawn line by line, and the gothic logo |
| `2avatar` | the entry lesson: characters, the menu, transfers, the first-start reset |
| `2upstairs` | the city: the character sheet, store, bank, morgue, guilds, spells |
| `2darkmoor` | the dungeon: the map view, movement, encounters, combat, treasure |
| `2avatuse` | the `-use-` lesson: every define set, the players' common bank, the utilities |
| `2avatcom` | the 8,000-word common `dungeoncom`: races, classes, items, spells, monsters, guilds |
| `2avathelp` | the help lesson, reached by keyword |
| `2avatmisc` | the lesson list, the access list and the loadable character sets |
| `2avatstat` | the statistics lesson |
| `2avatnset` | nameset: the characters, keyed by PLATO signon and group |
| `2avatname` | nameset: the pseudonyms, keyed by pseudonym and owner |
| `2avatgld` | nameset: the guild log |
| `2avatst` | nameset: the compiled statistics |
| `2avatmap` | dataset: the dungeon levels, one 320-word record each (`levels = 15`) |

## Playing

Double-click `Avatar.exe`. At the title page press NEXT to go straight into your character,
or any other function key for the menu. `Avatar.exe --name someone` plays as somebody else;
your character belongs to your Windows user name the way a PLATO character belonged to a signon.

| Menu | |
| --- | --- |
| NEXT | create a character, or run the one you have |
| HELP | the help lesson |
| DATA | the list of users |
| LAB | character options (destroy, pseudonym, information) |
| SHIFT-NEXT | transfer a character |
| SHIFT-DATA | the statistics lesson |
| SHIFT-HELP | the title page |
| SHIFT-BACK | leave |

**In the city.** `b` bank, `m` morgue, `s` general store, `g` guilds, `q` spells, `t` the
dungeon, HELP for that list.

**In the dungeon and the city**, as the game's own HELP page lists them:

| Key | |
| --- | --- |
| `M` `Y` `T` `H` | message / yell / track / untrackable |
| `+` `-` `K` | move in the company / leave it |
| `p` `l` | pick up / drop a body |
| `U` `P` `G` | use items / use powers / give gold or an item |
| `i` `S` `I` | see items / see spells / level information |
| LAB, shift-LAB | status information, company information |
| COPY, shift-BACK | the players in this area, character options |

**Only in the dungeon:**

| Key | |
| --- | --- |
| `a` `x` `d` `w` | turn left / around / right / forward |
| `A` `X` `D` `W` | turn that way and move |
| `t` | take the stairway you are standing on |
| `j` `f` `%` `x` | join / fight / autofight / autospell |
| `1` `2` `3` `4` | choose the group to fight |
| `o` `q` `Q` | open / exit / abandon a treasure chest |

| PLATO key | PC key |
| --- | --- |
| NEXT / shift-NEXT | Enter / Shift+Enter |
| BACK / shift-BACK | F9 / Shift+F9 |
| DATA / shift-DATA | F2 / Shift+F2 |
| HELP / shift-HELP | F1 / Shift+F1 |
| LAB / shift-LAB | F3 / Shift+F3 |
| COPY / TERM / STOP | F7 / F4 / F10 |
| ERASE | Backspace |

The world, your character and everyone's records are kept in `saves\` next to the program:
`2avatcom.common` (8,000 words) and a copy of each nameset and the dataset as they change.

## What was found in the game

- **The world is the one Avatar was last played in.** The 8,000-word common holds the ten
  races, twelve guilds, the item, spell and monster tables, the encounter tables and the guild
  rank names ("Wanderer", "Wayfarer", …) exactly as CYBIS left them on 7 March 1994.
- **The dungeon is the real dungeon.** Unlike Moria, Avatar does not generate its levels: the
  fifteen levels are hand-built maps stored in `2avatmap`, and that is what you walk through.
- **Forty-six characters are still in the character nameset**, with their pseudonyms — Grima
  Wormtounge, Thorin Oakenshield, Paksenarion, Doctor Zip, Radagast the Brown, Tinkerbell,
  Aeon Flux, Trout-boy!, \*Vinegaroon, Harvey Human and the rest. They cannot be played, because
  a character belongs to the hash of a PLATO signon, but the guild masters are theirs: the
  statistics lesson still reports Grima Wormtounge as the Nomad guild master at level 164.
- **The statistics are the 1993 ones**: "Statistics were last compiled on 12/19/93 at 11.08.25."
- **The game reset itself on first start**, as it was written to: `newsys` in `2avatar` notices
  that the system id no longer matches, closes the game, sweeps the city and the dungeon,
  rebuilds the pseudonym nameset from the character records, zeroes the counters and reopens.

## Differences from the original

- **You are always alone.** Avatar had fifty player slots, companies with a leader, tracking,
  messages and yells. All of it runs, but nobody else is in the game.
- **The notesfile** (SHIFT-LAB) was a PLATO group notes file (`2avatarn`), not a lesson. It is
  in `..\src_original\` — 21 blocks of players arguing about guilds and spectral dragons — but
  it cannot be read here.
- **Access.** CYBIS lessons ask a *custom access list* what a user may do. 2avatmisc's list
  names the 1994 authors and operators; everyone else matches its Other/Other entry, which is
  the level it calls **"Gamer"** — option 46, "enter game normally". That is what the port gives
  every player, so you go straight into the game and not to the author options page.
- **One bug of the original is kept.** The last source spells the "detect invisibility" bit
  `b.dinvi` in four places instead of `b.dinvis`. On CYBIS those four statements were condense
  errors and did not execute, and they do not execute here either: invisibility detection has
  no effect in combat, and coming back from the dungeon to the city does not clear poison,
  disease or paralysis.
- **Random numbers.** CDC's generator is not reproduced, so which monster you meet and what
  it rolls differ from 1994. The dungeon itself does not, because it is stored, not generated.
- **Pauses keep their original lengths**, and the monsters still act on the game's own timer.

## Making it run

Avatar needed, on top of the CYBIS TUTOR the other ports already used:

- **Namesets.** A nameset is "a special form of a dataset composed of named sets of records":
  a 30- or 10-character name, 24 bits of information, and a chain of records belonging to it,
  with `-datain-`/`-dataout-` relative to the name chosen by `-setname-`. The commands are
  documented in the CYBIS lesson `a0nameset`; the file layout was read out of Avatar's own four
  namesets, and the port keeps that format so the files stay readable by anything else.
  `-setname- -getname- -addname- -rename- -delname-`, and `zinfo znindex znscpn znsmaxn znsmaxr
  znsnams znsrecs zwpr zrecs zfile zftype`.
- **CDC negative zero.** A word of sixty one bits is zero in arithmetic but all ones to the bit
  operators. `getslot` takes `comp()` of the free-slot word and finds the lowest set bit, so
  without it nobody ever gets a player slot.
- **`lessnum` and `fromnum`**, the positions of the current and the previous lesson in the
  `-leslist-`, which is how `secure` decides whether you may be in the lesson at all.
- **`unitop`**, a unit that does not erase the screen when it becomes the main unit — the city
  and the dungeon draw the character panel first and then jump to it.
- **`c` as a comment** in the command field, `$` ending a unit's local declarations, a label
  that carries a command (`1done branch …`), `floating : var` with blanks, octal constants
  written in groups (`o0377 7777 7763 7777 4177`), and `block from;to;count` with semicolons.
- `-set-`, `-compute-`, `-access-`, `-lname-`, `-findl-`, `-finds-`, `-inserts-`, `-recname-`,
  and `-storage-` that grows when a later lesson asks for more.

## Source and build

`..\src_original\` holds the fifteen files as `.words` (every word, read by following the NOS
track chain) and decoded `.txt` listings; the five data files also as `.dataset`. The engine is
shared with the other PLATO ports and is not published here. `build.bat`
regenerates `src\game_data.c` and relinks.
