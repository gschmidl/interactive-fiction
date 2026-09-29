# Adventure 1.2 (462 points) on Multics

These are the working copies of the 1980 game, with the bugs fixed, Jim's TAKE ALL / DROP ALL finished, and
new PL/I versions of the routines that did not survive. [CHANGES.md](CHANGES.md) lists every change.

Written for current Multics (MR12.8 on the DPS8M simulator). It compiles, binds and runs there.

`fortran adventure_` gives two warnings (68, lines 115 and 119): the argument `dummy` of the statement functions
`liq` and `dark` is never used. Those are Don Woods' original lines, and the 1980 build would have printed the same
warnings, so they are left as they are.

## Files

| File | What it is |
|---|---|
| `adventure_.fortran` | The game (Palter's engine plus Platt material and the 1980 local changes), corrected |
| `adventure.data` | The database, corrected |
| `adv462.pl1` | The `adv462` command, which calls `adventure_$main_` |
| `adv462_io_.pl1` | `ldcomn`, `svcomn` (save and load games), `advatt`, `advdet` (attach and detach the database) |
| `addr.pl1`, `size.pl1`, `getime.pl1` | The other routines Palter left to the site |
| `build.ec` | Compiles everything and binds it into `bound_adv462_` |
| `bound_adv462_.bind` | The bindfile |
| `recase/` | The scripts that converted the database to mixed case (see CHANGES.md) |
| `CHANGES.md` | What was changed and why |

## Building

1. Copy the files into a directory on Multics, for example `>udd>Games>adv462`. This directory becomes the
   game directory, so `adventure.data` must stay in it.
2. In that directory, run:

   ```
   exec_com build
   ```

   This runs `fortran adventure_` and `pl1` on the five PL/I sources. It then puts the six objects and
   `bound_adv462_.bind` into `bound_adv462_.archive` and binds `bound_adv462_`, whose added name `adv462`
   is the command.

   Keep `adventure.data` in the same directory as `bound_adv462_`, or in the site directory described below.

## Where the files go

| File | Where it's looked for | Where it's written |
|---|---|---|
| `adventure.data` | `>site>adv462_dir`, then the directory holding `bound_adv462_` | never written |
| `adventure.newgame` | `>site>adv462_dir`, then the directory holding `bound_adv462_` | `>site>adv462_dir` if that directory exists, else the directory holding `bound_adv462_` |
| `name.adv462` (SUSPEND/RESTORE) | the player's home directory | the player's home directory |

- **In a library such as `>aml`:** put `bound_adv462_` and `adventure.data` in the library. Create
  `>site>adv462_dir` for the image, writable by whoever will run magic mode.
- **A private copy:** needs no site directory; everything stays together.
- **Links:** the game directory is where `bound_adv462_` really is, even when players reach it through a link.

## First run: making the new-game image

With no `adventure.newgame` yet, the first run reads `adventure.data`, reports its table space, and then asks
"are you a wizard?". This is Palter's first-time setup. Answer the questions as follows:

```
are you a wizard?  yes
prove it!  say the magic word!  dwarf
that is not what i thought it was.  do you know what i thought it was?  no
(ten digits)  dwarf
oh dear, you really *are* a wizard! ...
```

Then answer the maintenance questions: hours, holiday, short game length, magic word, restart latency and
message of the day. Pressing return keeps a default.

`adventure.newgame` is then saved in `>site>adv462_dir` if it exists, otherwise in the game directory. You need `sma` access to whichever it is. Later games start
from it straight away. Magic mode ("magic mode" as the first command) saves a new one.

The 1980 defaults still apply on Multics:

- **Prime time.** On weekdays from 8:00 to 16:59, the cave is closed to everyone but wizards; other players are
  offered a short "demo" game. Change the hours in magic mode ("do you wish to change the hours?").
- **Restart latency.** A suspended game can't be restored for 90 minutes.

## Letting others play

Give other users:

- `r` access to `adventure.data` and `adventure.newgame`;
- `re` access to `bound_adv462_`;
- `s` access to the directory.

For example:

```
set_acl adventure.data r *.*.*
set_acl adventure.newgame r *.*.*
set_acl bound_adv462_ re *.*.*
```

Players type `>udd>Games>adv462>adv462`, or add the directory to their search rules and type `adv462`.

SUSPEND *name* saves a game as `name.adv462` in the player's home directory. RESTORE *name* continues it.

## Compiler notes

The 1980 program was compiled by the "new" Fortran compiler of late 1979, which is the current one. These points
were checked when porting; none of them caused trouble on MR12.8:

- **`and`, `or`, `xor`.** The program defines its own functions with these names, and today's compiler also has
  typeless built-ins of the same names that do the same thing. The compiler accepts the definitions.
- **Free-form source.** The 1980 listing had three statements longer than 72 columns, so it was compiled in the
  default free form, not with `-card`. The working copy has none, but free form is still the intended way.
- **The `!` character.** It now starts a comment, but here it only appears inside `1h!` Hollerith constants and
  quoted strings.
- **Storage.** The compiler's default storage class is automatic. All program units are in one source segment,
  which the compiler documentation says keeps their values.

The PL/I side depends on the following:

- **Argument types.** Arguments arrive from Fortran without descriptors:
  - logical as `bit (1) aligned`;
  - integer as `fixed bin (35)`;
  - `fname` as ten words each holding one character.
- **Library routines.** It uses `initiate_file_`, `terminate_file_`, `hcs_$fs_get_path_name`,
  `user_info_$homedir` and `iox_`.
