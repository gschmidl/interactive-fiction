# Dungeon (Zork) - PRIMOS SEG runfile

**Status: STAGED 2026-09-19 - not ported, nothing built.** Files here are verified copies (md5) of the originals named below.

Source: `F:\bits\Prime\sbd\SBD003.zip` -> `sbd003_games_6-7-85.tap` (SIMH tape, PRIMOS MAGSAV, labelled SBD003 121885).
Same set of tapes that gave Ankh and Tower (SBD001/002). `src_original\` holds the files cut out with
`..\_bits_sweep_work\tools\primex.py`: each file raw as on tape (high-bit ASCII, 0x91 nn = run of blanks, lines padded
to even length) and, where it is text, a decoded `.txt` beside it. `*.ufdhdr` = directory header records.

`SEG_GAMES>DUNGEON.SEG` is a segment directory (54 subfiles, ~210 KB) - binary only. No DINDX/DTEXT-style data files
were found on the tape under obvious names, so the text is either linked in or missing; not checked. Runs only
under PRIMOS (p50em). Low priority: the game itself is the DECUS FORTRAN Dungeon we already have elsewhere.
