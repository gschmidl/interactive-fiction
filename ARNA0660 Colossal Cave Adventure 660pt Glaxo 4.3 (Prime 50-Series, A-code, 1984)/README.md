# ADVENTURE4 - Mike Arnautov's 660-point Adventure, Glaxo version 4.3 (26 Jul 1984), PRIMOS

**Status: PORTED 2026-09-21 (first pass).** `port\adventure4.exe` (`port\play.bat`) runs the tape's database under the
executive converted from its F77 source. Eight sessions played on the original - the tape's own EXECUTIVE.SEG,
restored with MAGRST onto a scratch PRIMOS 23.4 pack under p50em - come out identical line for line. PRIMOS's RND is
measured but not yet reproduced, so the sessions keep off the dice. See `port\README.md`. Files here are verified
copies (md5) of the originals named below.

Source: `F:\bits\Prime\pulse_library.zip` -> `pulse_library.tap` (PRIMOS MAGSAV, the PULSE user-group library),
directory `PULSE>ADVENTURE4`. Files cut out with `..\_bits_sweep_work\tools\primex.py` (raw + decoded `.txt`).

`*INFO*` (saved as `@INFO@`): "stand-alone version of ADVENTURE4 (the complete 660 pt Glaxo version 4.3, 26th Jul 1984)":
1) the A-code executive - F77 source, CPL build file, SEG runfile; 2) four pseudo-binary database files ADVINIT1-4.dat;
3) the manual ADVENTURE.runi. "The A-code source and compiler will only be released to those who can demonstrate their
mastery of this version" - so the A-code *source* of the cave is NOT here, only the compiled database. Change log
14 Oct 83 .. 26 Jul 84 is in the same file.
eXo's "0660-Point" is Arnautov's modern C release; this is the 1984 original. Port = translate EXECUTIVE.F77 and read
ADVINIT1-4 as PRIMOS wrote them (16-bit words, big-endian).

## Reference added 2026-09-19 (github.com/Quuxplusone/Advent, commit d38e825)
`reference\ARNA0660\` = Arnautov's adv660 10.12 source (A-code `.acd` + generated C). The Prime tape has only the compiled ADVINIT1-4 of the 1984 Glaxo 4.3; this modern A-code is the key for decoding them.
