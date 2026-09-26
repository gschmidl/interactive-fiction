# DAVESCAVE - "Dungeons and Dragons written by Dave Parker", MITRE Corp.

**Status: PORTED 2026-09-21 (first pass).** `port\davescave.exe` - see
`port\README.md`. The tape turned out to be a plain Unix tar archive; the
game's source, a VAX-11 FORTRAN IV-PLUS compile listing of the very same
source (9-Sep-1980), and its word table are cut out into `src_original\`.
The dice are FORTRAN's `RAN(I1,I2)`, which is RANDU: taken instruction by
instruction from the VAX run-time library (FOR$IRAN in FORRTL.EXE). No system
that can compile it was at hand, so there is no reference transcript yet.

Files here are verified copies (md5) of the originals named below.

Source: `bitsavers.org/bits/DEC/pdp11/trax/tape6_traxSrc_3-29-86.tap` - a SIMH tape image of a V7 tar archive (10240-byte
records, 3278 members: `rsx/`, `stl/`, `res/`, `trax/`, `pics/`). The game is in `res/`, a VAX/VMS directory:
`PROGRAM DAVESCAVE / C DUNGEONS AND DRAGONS WRITTEN BY DAVE PARKER / C DISTRIBUTED BY: MITRE CORP. DEPT W-45
WESTGATE PARK MC LEAN, VA.`, "VERSION 1.0, DATED SEPT 1980". "WELCOME TO DAVE'S DUNGEON / ENTER YOUR THREE
INITIALS", two-letter commands, character saved between plays. A dungeon crawl (same genre as DND), not a parser
adventure. The same tape also carries Adventure 350 and Dungeon text (stock DECUS).

`src_original\`:
- `davescave.for` (37420 bytes, md5 d16e248b04df523146fe15d679bf1b25) - the source
- `davescave.ftl` (104320 bytes, md5 86a36d38ae734d7fabb6b5f39c0612bf) - VAX-11 FORTRAN IV-PLUS V1.3-22 listing
  of DAVESCAVE.FOR.3, 9-Sep-1980, with storage maps
- `textfile.dad` (1018 bytes, md5 7e799bb9033819e430c95a91ab520e0a) - the word table, PDP-11 FORTRAN
  unformatted (126-byte segments)
