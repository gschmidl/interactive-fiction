# Wellesley Adventure V6.2 (Eric Roberts) - Windows console port of the FORTRAN source

| launcher | program | revision |
|---|---|---|
| `run.bat` | `newadv.exe` - Roberts' FORTRAN source (`..\src_original\ROBE0665`, newadv.F last modified 3 March 2010) compiled with gfortran | `-- ADVENTURE (V6.2),  3-Mar-10 --`, "a possible 655" |

The source's folder upstream is called ROBE0665, but it is the 655-point V6.2 (its `setup` counts 625). The 665-point
V6.4.2 survives only as the browser edition, ported in the sibling ROBE0665 folder; no source of it is known.

## newadv.exe

SAVE and RESTORE use `saves\newadv.sav`. `build.bat` / `build.sh`, following Roberts' Makefile (gfortran
`-std=legacy -ffixed-line-length-none`, gcc `-fcommon`, statically linked):

1. `allcom.F` includes every COMMON block; `nm` of its object gives the block sizes (`common.h`, which comes out
   identical to the one in Roberts' tree) and `volatile.fg` picks the blocks SAVE writes (`volatile.h`). `pcom.h` is
   left out: it is a stray list of the `params.h` names, not a COMMON include.
2. `setup.exe` (setup.F + advlib.F) compiles `text.dat` into the COMMON blocks and `common.c` dumps them as `data.c`.
   The result is byte-identical to Roberts' own `data.c` except the build date setup stamps into `VERDAT`; the build
   checks that and then compiles his `data.c`, which carries " 3-Mar-10".
3. `newadv.exe` = newadv.F + advlib.F + aparse.F + data.c + csub.c (wizard.F is not linked, as in the Makefile, so
   there are no prime-time restrictions).

Changes, all marked `PORT:`:

- `csub.c`: save files are opened in binary mode, and Windows headers replace `<sys/file.h>`.
- `csub.c`: the program never seeds `rand()`, so every game drew the same numbers from the C library. Roberts built on
  Mac OS X (the leftover objects in his tree are Mach-O), whose `rand()` is Park-Miller "minimal standard" with
  RAND_MAX 2^31-1; `crand_()` now uses that generator (seed 1) instead of the Windows C library's 15-bit one.
- `newadv.F`: the billboard and notebook were read from `/usr/games/billboard.txt` and `/usr/games/notebook.txt`; they
  are now read from the program's own directory (new `cgamdir_()` in csub.c). `billboard.txt` ships beside the .exe;
  there is no notebook file, so reading the notebook gives the game's own "blank" message as before.
- `csub.c`: options, read before the FORTRAN program starts: `--no-fixes` (the fix below off), `-h`/`--help`; any
  other `--option` is refused, exit 2.

**Fix 1 (off with `--no-fixes`): SAVE forgets what is in the containers.** `volatile.fg` lists `hdlcom`,
but the block is `/HLDCOM/` (HOLDER and HLINK), so SAVE never wrote it and a restored game got the containers as they
were at the start: pour out the bottle, SAVE, RESTORE, and it is full of water again. The build now also extracts
HLDCOM's entry (`holder.h`), and SAVE appends that block and RESTORE reads it back. A game saved with `--no-fixes`
lacks it, and restoring one leaves the containers as they are.

`CALL MOVE(AMULET,299,0)` and `CALL MOVE(SCROLL,LOC,0)` pass three arguments to the two-argument `MOVE` (gfortran
warns): the third is never read, so they do what `MOVE(AMULET,299)` does.

## Checked so far (first pass, 2026-09-19)

- Opening, pantry, poster, T. Sawyer's note, pit death and reincarnation, SAVE / RESTORE round trip. A Linux build
  (gfortran 12, glibc) of the same sources gives byte-identical transcripts for 8 random 400-command sessions.

## Refine pass (2026-09-21)

- Fix 1 checked both ways (the emptied bottle after SAVE and RESTORE); the options.

## How V6.4.2 differs

The sibling ROBE0665 folder's `port\TEXTS_V62_V642.md` compares this source's texts with the browser edition's.

## Still to do (refine pass)

- Deferred (user, 2026-09-21: bugs and fuzzing only): a ConPTY check, winnability via a debug mode.
