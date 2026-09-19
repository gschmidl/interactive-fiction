# DUNGEON - Jonathan H. Reed, 6/83, Prime PL/I subset G

**Status: STAGED 2026-09-19 - not ported, nothing built.** Files here are verified copies (md5) of the originals named below.

Source: `F:\bits\Prime\sbd\SBD003.zip` -> `sbd003_games_6-7-85.tap` (SIMH tape, PRIMOS MAGSAV, labelled SBD003 121885).
Same set of tapes that gave Ankh and Tower (SBD001/002). `src_original\` holds the files cut out with
`..\_bits_sweep_work\tools\primex.py`: each file raw as on tape (high-bit ASCII, 0x91 nn = run of blanks, lines padded
to even length) and, where it is text, a decoded `.txt` beside it. `*.ufdhdr` = directory header records.

`PL1G_GAMES>DUNGEON.PL1G` (34 KB, one file, complete): a 10x10x10 dungeon crawl with typed commands - move, attack, items,
help, checktrap, where, untrap, search, take, hide, retrieve ...; seed asked as "a wierd positive number". A crawl
rather than a parser adventure. Source port to C or via Iron Spring PL/I.
