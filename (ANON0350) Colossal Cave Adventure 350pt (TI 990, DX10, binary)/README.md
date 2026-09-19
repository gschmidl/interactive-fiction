# Adventure "thru a Cave" - TI 990 / DX10 GAMES library

**Status: STAGED 2026-09-19 - not ported, nothing built.** Files here are verified copies (md5) of the originals named below.

Source: `F:\bits\TI\990\9trkTapes\orig\ti990_games.tap.gz` (+ `titapes.txt`; `DX10_system_tapes\` = `...\orig\DX10\`,
the OS tapes an emulator would boot).

The tape is a DX10 backup of `GAMES` (`.SYN GAMES=GAMES.GAMES`, `.USE @GAMES.PROC`): NIM, PACMAN, FOOTBALL, HANGMAN, MIND
(COBOL), STARTREK, CALENDAR, TTT, ADVENTUR, LANDER, BIO, GUESS, LUNAR, MINE (Tim O'Connor, 990 FORTRAN, 4/79), BBOX, SUB,
BLACKJAC, TREK. Menu text: "ADVENTURE Wandering adventure thru a Cave"; proc `ADVENTUR(Adventure thru a Cave)` asks
"Will/did you save your game?" and takes `SAVE/RESTORE PATHNAME=ACNM(@$CAVE)`.
Adventure is a linked program image in `GAMES.PROG` (FORMAT strings at ~619975 and ~641976: "I SEE NO ... HERE",
"This adventure was suspended a mere ... minutes ago") - 350-point text, no FORTRAN source seen (other games on the tape
do have source). Port = TI 990 (TMS9900-family) + DX10 SVC emulation, or disassembly.
