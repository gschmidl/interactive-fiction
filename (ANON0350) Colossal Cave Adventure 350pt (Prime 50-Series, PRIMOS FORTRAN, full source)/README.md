# Colossal Cave Adventure 350pt - PRIMOS FORTRAN, wizard version with source

**Status: STAGED 2026-09-19 - not ported, nothing built.** Files here are verified copies (md5) of the originals named below.

Source: `F:\bits\Prime\sbd\SBD003.zip` -> `sbd003_games_6-7-85.tap` (SIMH tape, PRIMOS MAGSAV, labelled SBD003 121885).
Same set of tapes that gave Ankh and Tower (SBD001/002). `src_original\` holds the files cut out with
`..\_bits_sweep_work\tools\primex.py`: each file raw as on tape (high-bit ASCII, 0x91 nn = run of blanks, lines padded
to even length) and, where it is text, a decoded `.txt` beside it. `*.ufdhdr` = directory header records.

- `ADVENTURE.UFD` - `ADVENTURE.FTN` (100 KB, "C ADVENTURES / CURRENT LIMITS: 9800 WORDS OF MESSAGE TEXT...", SUBROUTINE MAIN,
  `$INSERT SYSCOM>KEYS.F`, WIZCOM hours/magic), `ADVSUB.FTN`, variants `BADVENTURE.FTN`, `IT.FTN`, `ADVENTURE.FTN.001`,
  `ADVSUB2.FTN`; build CPL/COMI files (`C_BUILD`, `C_COMPILE`, `C_LOAD`, `C_SEG`, `C_SHARED`); `COMMON` = the 60 KB text
  database; `ADVCOM>ADVCOM` = the initialised common image; binaries `AD4000`, `TW4000`, `ADVENTURE`
- `SCOTT` - a second user's copy with `ADVENTURE.LIST` (compiler listing) and BUILD.COMO
- `RUN_GAMES\ADVENTURE`, `BADVENTURE` - the installed R-mode runfiles
Straight source port candidate (gfortran), same family as the TOPS-10 350.
