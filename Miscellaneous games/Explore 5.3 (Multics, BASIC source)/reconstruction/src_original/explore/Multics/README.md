# Multics Explore

## What's included

Distributed for the maintainer to install into AML:
- `bound_explore_.s.archive` — source archive (BASIC + PL/I).
- `bound_explore_.archive` — object archive (with bind file); produces
  `bound_explore_`, installed in `>aml`.
- `explore.data` — room descriptions, exits, objects, creatures, vocabulary.
- `explore.help` — in-game help text.
- `hours.data` — base cave hours + (rot13) sorcerer word + message of the day.
- `winners.data` — the historical hall-of-fame tablet.
- `explore.info` — the Multics `help explore` reference.
- `explore_setup.ec` — the site setup exec_com (see Installation).
- `README` — this file.

## Installation

The bound object `bound_explore_` is installed in `>aml` by the library
maintainer (found via the default search rules). The runtime data is set up by
`explore_setup.ec`, which creates an `explore_dir` inside a directory you name
and populates it. The four base data files (`explore.data`, `explore.help`,
`hours.data`, `winners.data`) must be in the working directory when you run it.

```
asp ec >aml
ec explore_setup TYPE DIR [RW_DIR]
```

- **TYPE** — `read-only` (or `ro`): install only the read-only base files;
  no writable directory, no `explore.rwdir`, no multiplayer files. The game
  runs read-only (no recorded wins, no multiplayer).
  `standard`: the base files (always read-only) plus the writable working
  files and the multiplayer registry/comms/lock files. **Everything is created
  read-only; nothing is world-writable.** The ec then prints the `set_acl`
  commands to grant `rw` to your players/sorcerer if you want recorded wins
  and multiplayer.
- **DIR** — the existing directory the `explore_dir` is installed *into*. Give
  `>site` for a normal install, or any other existing directory (e.g. a
  throwaway test directory). `DIR` must already exist; the ec creates
  `DIR>explore_dir` but never creates `DIR` itself, and aborts if it is missing.
- **RW_DIR** — (standard only, optional) where the writable files live; default
  `DIR>explore_dir>private`.

A `standard` install leaves all writable and multiplayer files **read-only**.
To enable recorded wins and multiplayer, the administrator grants `rw` on the
specific segments to the allowed players (the ec prints the exact commands),
and, to permit multiplayer, changes line 2 of `explore.rwdir` from `^multip`
to `multip`. This makes enabling multiplayer a deliberate, least-privilege
step rather than a default.

`explore.rwdir` (written for a `standard` install) is a small control file the
game reads at startup. **Line 1** is the read-write directory pathname (or
`none` to force read-only). **Line 2** is `multip` to permit multiplayer or
`^multip` (the default) for writable single-player only. This yields three
run-time states:
  - no rwdir / line 1 = `none` -> read-only (no recorded wins, no multiplayer);
  - line 1 = a path, line 2 = `^multip` -> writable single-player;
  - line 1 = a path, line 2 = `multip` -> multiplayer permitted.

Even when multiplayer is permitted, a player can only enable the per-user
`multip` mode if they have write access to the shared files; a player without
it stays single-player.

To exercise the installer without touching the live `>site`, give an existing
throwaway directory as `DIR` (e.g. `ec explore_setup standard >udd>Proj>me>etest`).

Before real play, edit `hours.data` so line 1 is the **rot13** of your chosen
sorcerer word (see "hours.data format").

### Single-player vs. multiplayer, and read-only installs

The game finds its writable directory from `explore.rwdir`. If that file is
absent, empty, or contains `none` (as with a `read-only` install), the game
runs **read-only single-player** with no errors: you can explore, read the
hours and the hall of fame, and save games (saves go to your home directory),
but wins are not recorded and multiplayer is unavailable. A `standard` install
provides a writable directory (and `explore.rwdir` pointing at it); a
`read-only` install deliberately omits it. The game also falls back to
read-only if a `standard` install's `explore.rwdir` is later removed or set to
`none`.

## Playing

Type `explore` to start. Movement is one-word compass directions (`n`, `s`, `e`,
`w`, `u`, `d`) plus `in` / `out`; you can also type a nearby room's name to walk
there. Core commands include `get`, `drop`, `throw`, `look`, `read`, `what`
(inventory), `score`, `save` / `restore`, `brief` / `full`, and `help <topic>`.
Type `commands` for the full list and `help` for information. Some words carved
on walls or whispered by cave dwellers are magic — learn and use them.

Score by carrying treasures back to the shack and dropping them there; defeating
creatures and solving a couple of puzzles also scores. A perfect game is 500
points and earns the rank of Grand Master Explorer.

Per-user files — your saved games, command abbreviations, an abbrev-editing
scratch file, and an optional `start_up.explore` command script — live in your
home directory and are created automatically as needed.

### start_up.explore (optional startup script)

At launch the game reads `start_up.explore` from your home directory, if present,
and runs the commands in it as though typed, then returns to interactive input.
It supports labels and `&`-directives — an exec_com-like facility. Typical uses:
setting modes, loading a scripted sequence, or (see below) enabling multiplayer
by default.

## Multiplayer and the `multip` attribute

Explore supports multiple simultaneous players sharing a cave, coordinated
through shared files in the read-write directory: a player registry (who is
playing, driving the `whom` command and a permanent per-player history), a
user-to-user message channel (`talk`), and a sorcerer broadcast channel. A
player in **sorcerer** mode (entered with the correct magic word from
`hours.data`) can list all players (`whom`), message them (`mail` / `talk`),
move about, and edit the cave hours and sorcerer word.

These multiplayer features are gated by the **`multip`** mode, which is **off by
default** (`^multip`). In the default single-player mode the game touches none of
the shared files at all — so a lone player, or a player without write access to
the shared files, runs cleanly with no errors, but is invisible to other players
and to the sorcerer.

**To take part in the shared game, enable `multip`.** You can turn it on for a
session with the `multip` command, or — most conveniently — make it your default
by putting `multip` in your `start_up.explore`:
```
multip
```
With `multip` enabled, your presence is recorded in the registry (so the sorcerer
and other players can see and interact with you), and `talk` and the sorcerer
message channels become active. Enabling `multip` by default in your
`start_up.explore` is the recommended way to always participate in a shared game.

(This is a deliberate change from the original 1980 game, in which the registry
was updated every turn regardless of mode, so the sorcerer saw even players who
had not opted in. Gating all shared-file access on `multip` lets single-player
and read-only users run without needing write access, at the cost of being
invisible until they opt in. See CHANGES.)

### Security posture and future work

The multiplayer layer uses shared flat files with advisory locking (via
`set_lock_` in the `exp_lock_` / `exp_unlock_` helper), a design inherited from the original
game. It works, but it does not use Multics' native secure inter-process
communication. The recommended deployment contains this: the writable directory
is owner-only, and the host grants `rw` on the specific shared *segments* to the
players allowed into the game (on Multics a player can read/write a named segment
without any access to its containing directory). Personal files, including the
abbrev scratch file, live in each player's own home directory. The read-only game
data is elsewhere and immutable. A potential future version could reimplement the
player and sorcerer messaging using a more secure mechanism (rings and gates;
it could ride on top of mailboxes or message segments).

## hours.data format

Line-oriented. Line 1 is the sorcerer word, **rot13-encoded** (the game decodes
it at startup). Following lines give cave open/close hours and the message of the
day. A minimal always-open file with sorcerer word "hello" (rot13 `uryyb`):
```
uryyb
0000,2359
0000,2359
0000,2359
0000,2359
0000,2359
0000,2359
9999,Welcome to Explore!,01/01,New Year
```
The placeholder word should be changed before real play.

## Dependencies

Beyond the bound helpers, the game calls these standard Multics commands as
subroutines: `delete_force`, `create`, `set_acl`, `abc`, `ted`,
`send_message`, `sort_seg`, `exec_com`, and `do` (the latter for the `..` /
`m` command-escape). The PL/I helpers use
`read_password_` (for the un-echoed sorcerer-word prompt) and
`user_info_$homedir` (to locate the player's home directory).

## CHANGES from the original (reconstruction notes)

Fixed during reconstruction:
- Save/restore: the restore read-loops were bounded by record count rather than
  reading to their sentinels, and the creature read-loop jumped past its own
  `for` initialization — both corrupted a restored game. Corrected.
- The winners tablet read expected two fields but the records carry three
  (date, name, score); corrected, and the game now writes the score in
  `(nnn points)` form.
- The winning magic word teleport and win-room numbering were reconciled.
- The win-room description was trimmed so the "figure rises and vanishes"
  narration happens only on an actual win (it had been printed unconditionally).
- The security-alarm loot relocation and the room object display were tidied
  (loot is swept into the vault; barrier objects no longer print blank lines).
- The compiler's forward-reference limit was reached by the command dispatcher;
  three handlers were relocated to resolve references earlier.
- Room numbering and object point values were recovered to match the 5.3 game
  (max score 500; treasures and creatures worth 15 each; two +10 puzzle
  bonuses).

Design changes for this release:
- **Multiplayer gating.** Shared-file access is now gated on the existence
  and content of >site>explore_dir>explore.rwdir. If the file does not exist
  or contains "none" as its first line, the game is entirely read-only;
  new winners' names are not added to the registry and the sorcerer cannot
  edit the game hours from within the game. If the first line points to
  a directory, that directory should contain copies of the game hours file
  and winners database that are writeable by any admin and any permitted
  player, respectively. If the second line contains `multip`, then multiplayer
  mode is permitted; if it is missing, "^multip", or anything else, then
  multiplayer mode is not permitted. Multiplayer mode requires additional files
  writeable by any users permitted to enable `multip` in the game. Originally
  all users were visible to the sorcerer and multiplayer mode was always
  possible but off for players by default; in the current version only
  players who enable `multip` are visible to the sorcerer or each other.
  (Objects and creatures in the game are still specific to each player's
  own game; `multip` only facilitates mutual visibility and communication.
- **Registry as permanent history.** The player registry gained a last-login
  date and an active flag, giving a permanent per-player history; `whom` shows
  current players, and a clean quit marks the player inactive. This subsumes the
  original's separate startup log.
- **Configuration.** The read-write directory is found from a one-line
  `>site>explore_dir>explore.rwdir` file, so the game needs no built-in paths;
  read-only data is read from `>site>explore_dir`. No configuration segment.
- **New helpers.** The `exp_getpw_`, `exp_home_`, and `exp_canwrite_` helpers
  are new in 2026; `exp_lock_` / `exp_unlock_` (set_lock_-based advisory
  locking, held across each critical section) replace the original's
  `exp_trfcc_`.  The other helpers previously existed and were likely all in
  BASIC; most have been re-created in BASIC for fidelity.
- **Break key disabling.** The original game had calls to a non-standard
  quit_off/quit_on tool to disable use of the break key in critical parts of
  the program (which was also controlled by the per-player modes ^quit/quit,
  which is now non-functional).

Known limitations:
- An abnormal exit (break-out, disconnect) leaves the player's registry entry
  marked active; it is recoverable by the sorcerer or by pruning stale entries
  (identifiable by an old last-login date).
- The `blob` creature from version 4.3 survives as dead code (its kill message
  is unreachable in 5.3/6.0); left in place as a historical artifact.
