# CP-V Adventure - David Platt's original of "Adventure 550", Xerox Sigma CP-V, 1979

**Status: STAGED 2026-09-19 - not ported, nothing built.** Files here are verified copies (md5) of the originals named below.

Source: `F:\bits\SDS\sigma\ladc\LADC_*` (Honeywell Los Angeles Development Center SST tapes). Each LADC folder has the SIMH
tape (`.tap.gz`, EBCDIC), a zip of the files already converted to ASCII, and the reader's notes. The Adventure files are
identical on most tapes; the distinct builds kept here (first tape of each group is in `archive_original\`):

- `LADC_0012` - same Adventure files also on: LADC_0012, LADC_0162, LADC_0211, LADC_0231, LADC_0273, LADC_0332, LADC_0342, LADC_0468, LADC_1650, LADC_3361, LADC_6143, LADC_7095
- `LADC_0054` - same Adventure files also on: LADC_0054
- `LADC_0080` - same Adventure files also on: LADC_0080
- `LADC_0347` - same Adventure files also on: LADC_0347

`src_original\<tape>\` = the members named in `$$$ADV.txt` (CP-V `:` is written `$` in the zip's file names):
- `ADVINFO` - Platt's release notes: 7/20/79 "the full source for CP-V Adventure", 12/1/79 patch (CACHESI REFs F:10/F:11,
  D:MOVES cylindrical-room loophole). Mentions the CP-6 / PL-6 rewrite using the same database.
- `ADVSI`, `ADV:C` (COMMON include) - the runtime interpreter, ANS FORTRAN; `CACHESI SVARSI DECSI PRIMESI GETACCTSI
  BREAKSI ENCSI FASTREADSI MASHSI OUTSWPSI` - Sigma assembler (AP) helpers; `ADVJOB`, `ADVLYNX` - build job and link
- `MUNGESI`, `MUNGE:C`, `MUNGEJOB`, `MUNGELYNX`, `REGMETA` - the database translator ("munger")
- `D:TEXT D:PLACE D:OBJECTS D:OBJSYN D:VERBS D:ACTION D:MOVES D:LABELS D:INIT D:REPEAT D:BITS D:VARS D:DEFINE D:NULLS` -
  the cave in Platt's adventure language (TEXT/PLACE/ACTION/IFEQ/IFNEAR/BIT/CALL...), `COMPILE_CAVE` jobs build ADVT + ADVI
- game text announces "the brand-spanking-new B00 version of CP-V Adventure ... almost twice as big as before"
Differences between the kept builds (diff `src_original` to see them):
- LADC_0012 = the 7/20/79 + 12/1/79 release described in ADVINFO (35 members)
- LADC_0054 = same except D$OBJECTS differs and D$OBJSYN is not a separate member
- LADC_0080 = earlier SST: no ADVINFO, no D$BITS; CACHESI, D$ACTION, D$INIT, D$LABELS, D$REPEAT differ
- LADC_0347 is the smaller, earliest set (no D$BITS / D$MOVES, shorter D$TEXT / D$PLACE / D$OBJECTS / D$LABELS).

## What a port needs
Source port: the FORTRAN is portable apart from CP-V I/O (M:SI/M:BO, keyed files) and the AP routines, which are small
(bit packing, save/restore variables, account lookup, string decode) and must be rewritten. Alternative: write a new
interpreter for the D: language. eXo's "0550-Point" is a later Z-machine/C descendant, not this code.

## Reference added 2026-09-19 (github.com/Quuxplusone/Advent, commit d38e825)
`reference\PLAT0550\` = Platt's later portable release as reconstructed by Mike Arnautov (mipmip.org orig550.tgz): `ADVENTURE.ACODE` (A-code, NEWS says September 1984), `munge.f`, `exec.f`. Same language five years on - use it to read the 1979 D: files and as a behavioural cross-check (see feedback_check_against_another_port).
