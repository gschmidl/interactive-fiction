# ABENTEUER - German Colossal Cave, Harris VULCAN (24-bit), c. 1979-83

**Status: STAGED 2026-09-19 - not ported, nothing built.** Files here are verified copies (md5) of the originals named below.

Source: `F:\bits\Harris\vulcan\fast.tap` (all 14 Harris tapes are in `..\_Harris_VULCAN_work\tapes\`).

`fast.tap` is a Harris FAST disc save in three huge records (4.8 MB, 4.5 MB, 16.7 MB). Text is 8-bit, three
characters per 24-bit word, high bit often set, `~` and control bytes are line/word padding. Masked to 7 bits:
- FORTRAN source with German FORMATs at ~15.85-15.96 MB (`DU BIST UEBER MEINE SKALA HINAUS`, `MIT',I5,' ZUEGEN.'`,
  `ICH KANN DEIN ABENTEUER SICHERN`, `DIESES ABENTEUER WURDE VOR KAUM ... MINUTEN GESICHERT`)
- the German text database at ~16.77-16.84 MB (`DU BEFINDEST DICH INNERHALB EINES GEBAEUDES`, `MAGISCHES WORT: "XYZZY"`,
  `WILLKOMMEN ZUM ABENTEUER! MOECHTEST DU HINWEISE?`, magic-mode / hours texts, so it is the 350 wizard version)
- 186 x `DU BIST`, 58 x `YOU ARE IN` - the English original (catalogue name ADVENTUR) is in the same save.
`recovered\fast_tap_strings_15.8M-17.0M.txt` = every printable run of that region with its byte offset.

The site catalogue (vulcan_infomaster_errs.tap @~5306692, "Es gibt folgende Spiele ( 0000PLAY*name )") lists
ABENTEUE "Suchen Sie Ihr Glueck in der GIGANTISCHEN HOEHLE" and ADVENTUR "... oder lieber auf Englisch in COLOSSAL CAVE ?".

## What a port needs
A reader for the Harris FAST / file format to cut the files out cleanly (source, data file, their names), then a normal
FORTRAN source port. Harris FORTRAN packs 3 characters per word (A3), so Hollerith constants need the usual care.
