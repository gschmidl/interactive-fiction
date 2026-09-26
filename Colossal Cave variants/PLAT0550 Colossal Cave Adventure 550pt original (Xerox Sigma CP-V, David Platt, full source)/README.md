# CP-V Adventure - David Platt's original of "Adventure 550", Xerox Sigma CP-V, 1979

**Status: PORTED 2026-09-21 (first pass).** `port\adv.exe` - see `port\README.md`. The interpreter (ADVSI) and the
database translator (MUNGESI) are the original ANS FORTRAN, converted for gfortran from the tape itself; the Sigma
assembler helpers are rewritten from their sources; the cave is compiled by the original translator from the original
D: files, exactly as COMPILE_CAVE did.

Source: `bitsavers.org/bits/SDS/sigma/ladc/LADC_*` (Honeywell Los Angeles Development Center SST tapes). Each LADC folder has the SIMH
tape (`.tap.gz`, EBCDIC), a zip of the files already converted to ASCII, and the reader's notes. The Adventure files are
on most of the tapes (first tape of each group is in `archive_original\`):

- `LADC_0012` - same Adventure files also on: LADC_0012, LADC_0162, LADC_0211, LADC_0231, LADC_0273, LADC_0332, LADC_0342, LADC_0468, LADC_1650, LADC_3361, LADC_6143, LADC_7095
- `LADC_0054` - same Adventure files also on: LADC_0054
- `LADC_0080` - same Adventure files also on: LADC_0080
- `LADC_0347` - same Adventure files also on: LADC_0347

## The zip conversions are incomplete - use `src_original\LADC_0012_tape\`

**The ASCII files in the LADC zips drop every line that crosses a tape block** (a CP-V keyed record split over two
2048-byte blocks is two segments with the same key; the converter kept neither) and every control character: 175
lines of the Adventure members are missing or damaged, and COMPILE_AND_LIST_CAVE is not there at all. MUNGESI loses
`SUBROUTINE SWAP(I, K)` and the whole `DATA $OPTIONS` statement (the names of the language's 56 instructions), ADV:C a
line of its COMMON (the zip has a stray `}` there), D:MOVES `MOVE WEST,FOREST` among 22 lines, D:TEXT 37 lines
including the bells of the welcome, PRIMESI a branch and a line of the posted hours. `src_original\LADC_0012\` (the
zip's files) is left as it was, for reference only.

**`src_original\LADC_0012_tape\` is the complete set**, read from the raw tape with `port\src\cpvtape.py`: all 36
members named in `$::ADV` (`$$$ADV.txt`), including `COMPILE_AND_LIST_CAVE` and the binary `REGMETA` that the zip
lacks. Text is UTF-8; the five EBCDIC control codes in the cave's text appear as Unicode control pictures (BEL, BS, CR,
NAK, and LF for CP-V's "line feed only", X'20'); the Xerox braces and brackets (X'B2'-X'B5') as `{ } [ ]`.
`MANIFEST.txt` gives each member's line count and md5, `KEYS.txt` the EDIT line numbers of the two members that are
not simply 1.000, 2.000, ... - their fractional lines (12.100 in CACHESI, 2093.100-.400 in D:MOVES) are exactly the
12/1/79 patches ADVINFO describes. The port reads the tape image itself, not these files.

**The four tape groups are one release.** Keyed line by line, LADC_0054, LADC_0080 and LADC_0347 agree with LADC_0012
wherever they have a line; their "differences" are tape blocks that did not survive (LADC_0347's D:MOVES starts at line
60, its ADVSI lacks 72 lines ...), made worse by the zip conversion. LADC_0054 has no D:OBJSYN member, LADC_0080 no
D:BITS or COMPILE_AND_LIST_CAVE. LADC_0012 is complete (7/20/79 release + 12/1/79 patches).

Members (CP-V `:` is written `$` in file names):
- `ADVINFO` - Platt's release notes: 7/20/79 "the full source for CP-V Adventure", 12/1/79 patch (CACHESI REFs F:10/F:11,
  D:MOVES cylindrical-room loophole). Mentions the CP-6 / PL-6 rewrite using the same database.
- `ADVSI`, `ADV:C` (COMMON include) - the runtime interpreter, ANS FORTRAN; `CACHESI SVARSI DECSI PRIMESI GETACCTSI
  BREAKSI ENCSI FASTREADSI MASHSI OUTSWPSI` - Sigma assembler (AP) helpers; `ADVJOB`, `ADVLYNX` - build job and link
- `MUNGESI`, `MUNGE:C`, `MUNGEJOB`, `MUNGELYNX`, `REGMETA` - the database translator ("munger")
- `D:TEXT D:PLACE D:OBJECTS D:OBJSYN D:VERBS D:ACTION D:MOVES D:LABELS D:INIT D:REPEAT D:BITS D:VARS D:DEFINE D:NULLS` -
  the cave in Platt's adventure language (TEXT/PLACE/ACTION/IFEQ/IFNEAR/BIT/CALL...), `COMPILE_CAVE` jobs build ADVT + ADVI
- game text announces "the brand-spanking-new B00 version of CP-V Adventure ... almost twice as big as before"

## Reference added 2026-09-19 (github.com/Quuxplusone/Advent, commit d38e825)
`reference\PLAT0550\` = Platt's later portable release as reconstructed by Mike Arnautov (mipmip.org orig550.tgz): `ADVENTURE.ACODE` (A-code, NEWS says September 1984), `munge.f`, `exec.f`. Same language five years on - use it to read the 1979 D: files and as a behavioural cross-check (see feedback_check_against_another_port). Its munge.f settled one question of Xerox syntax for the port: `IF (c) s1; s2` makes both statements conditional.
