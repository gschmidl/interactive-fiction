# DUNGEON - Jonathan H. Reed, 6/83, Prime PL/I subset G

**Status: PORTED 2026-09-20, refined 2026-09-21.** `port\dungeon.exe` - see `port\README.md`. Verified byte-identical to the
original running on real PRIMOS 23.4 under p50em, in three recorded sessions (`port\tests\runall.sh`).
Files in `src_original\` are verified copies (md5) of the originals named below.

Source: `bitsavers.org/bits/Prime/sbd/SBD003.zip` -> `sbd003_games_6-7-85.tap` (SIMH tape, PRIMOS MAGSAV, labelled SBD003 121885).
Same set of tapes that gave Ankh and Tower (SBD001/002). `src_original\` holds the files cut out with
`..\_bits_sweep_work\tools\primex.py`: each file raw as on tape (high-bit ASCII, 0x91 nn = run of blanks, lines padded
to even length) and, where it is text, a decoded `.txt` beside it. `*.ufdhdr` = directory header records.

`PL1G_GAMES>DUNGEON.PL1G` (34 KB, one file, complete): a 10x10x10 dungeon crawl with typed commands - move, attack, items,
help, checktrap, where, untrap, search, take, hide, retrieve ...; seed asked as "a wierd positive number". A crawl
rather than a parser adventure.

The tape also carries `SEG_GAMES>DUNGEON.SEG`, Reed's own compiled runfile, which is what the port was compared
against. It is a PRIMOS *segment* directory (a directory of numbered runfile segments), so primex.py cannot name its
members: `src_original\SEG_GAMES\DUNGEON.SEG\` holds them as it cut them out (40 files, the PL/I run-time's messages
and Reed's text among them). It used to be staged on its own as "Dungeon (Prime 50-Series, PRIMOS, binary)", taken
for the DECUS FORTRAN Dungeon; merged here 2026-09-21. To run it, restore the tape under PRIMOS as `port\README.md`
describes and type `SEG DUNGEON`.
