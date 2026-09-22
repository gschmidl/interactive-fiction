# ABENTEUER - German Colossal Cave, Harris VULCAN (24-bit), 1977-80

**Status: PORTED 2026-09-21 (first pass).** `port\play.bat` plays the site's own game; see `port\README.md`.

This is Gary M. Palter's portable Adventure (MIT) as Harris Computer Systems Division adapted it ("HCSD VERSION
01.112777 DICK REYNOLDS"), with its messages translated into German, from a German Harris VULCAN site. The site
catalogue (vulcan_infomaster_errs.tap @~5306692, "Es gibt folgende Spiele ( 0000PLAY*name )") lists two games:
- ABENTEUE: "Suchen Sie Ihr Glueck in der GIGANTISCHEN HOEHLE"
- ADVENTUR: "... oder lieber auf Englisch in COLOSSAL CAVE ?"

Source: `bitsavers.org/bits/Harris/vulcan/fast.tap`, a Harris FAST disc save in three huge records (4.8 MB, 4.5 MB,
16.7 MB), copied verified (md5) to `archive_original\fast.tap`. All 14 Harris tapes are in
`..\_Harris_VULCAN_work\tapes\`.

## Files

The files were cut out of the save with `..\_Harris_VULCAN_work\tools\harris_text.py`. It reads the FAST
format's 672-byte blocks of numbered lines. A line is n, the text, NUL padding, then 8n; a byte of 128 or
more stands for a run of 256-b blanks.

- **`src_original\J.ADV.txt`:** the job stream that built PLAY*ABENTEUE. Lines 5-3538 are the FORTRAN.
  The rest is Harris assembler for the site routines (ADDR, SIZE, SHIFT, LOWSIX, ABORT, GENRAT, ATTACH,
  ASSIGN, IO), and it ends `$VU.RS PLAY*ABENTEUE`.
- **`src_original\database\TAPE1 ... TAPE1012`:** the section files the program reads when it sets itself
  up. TAPE7-9 holds sections 7-9; TAPE8 is a separate copy of section 8. They are a later edition than the
  game the site set up in June 1980, with five text edits (see `port\README.md`).
- **NEUSPIEL**, the game as the site had it, is not copied out as a file: `port\src\neuspiel.py` reads it
  from the tape. It is a game saved at its first command (`SICHR MEIN`) on Friday 13 June 1980 at 14:34.
- **`src_original\english_ADVENTUR_session_1980.txt`:** the file MASTER, a printed session of the English
  PLAY*ADVENTUR from 3 March 1980, run as a batch job. It is the only trace of the English game in the save
  (the "YOU ARE IN" hits of the first survey); its program and database are not there.
- **`recovered\fast_tap_strings_15.8M-17.0M.txt`:** every printable run of the save's text region with its
  byte offset, from the first survey.
