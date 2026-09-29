# "Generic Adventure -- Version:7.0, July 1994" - Robert R. Hall, C, shipped with MINIX 1.x (HALL0501)

**Status: PORTED 2026-09-19, refined 2026-09-21 - `port\run.bat`, see `port\README.md`.** A RETREAT crash and
three game-ending placeholders fixed, RESTORE hardened against short and foreign files.
Staged the same day: complete copy of the named directory of https://github.com/Quuxplusone/Advent at commit d38e82550600144e3547d6472bc80dbf49ca214b (2026-09-15), md5-verified against the clone.

`src_original\HALL0501\`: `advent.c turn.c verb.c itverb.c english.c travel.c vocab.c score.c initial.c database.c
utility.c setup.c`, headers, `advent1-4.txt` (database), Makefiles.
Hall put Long's / McDonald's game (content = McDonald 6.6, 551 points, Infocom-style parser) on top of Jerry Pohl's C
port. Differences from McDonald: the ledge west of Lost River leads only to the low room (no second troll solution);
acting "with your bare hands" drops everything; inherits Pohl's teleporting dwarves that never block the way.
eXo plays 551 only as O'Dwyer's Z-machine port of McDonald's FORTRAN, so this C lineage is not represented.
Port = compile (K&R-ish C; see feedback_knr_implicit_int_pointers).
