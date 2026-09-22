# Adventure! - Robert A. Howell, Atari 8-bit BASIC, 1982 (HOWE0301)

**Status: EMULATION ONLY (user's decision, 2026-09-19) - not ported; it runs as is in Altirra (see below).** Complete copy of the named directory of https://github.com/Quuxplusone/Advent at commit d38e82550600144e3547d6472bc80dbf49ca214b (2026-09-15), md5-verified against the clone.

`archive_original\HOWE0301\HOWE0301.atr` (92,176-byte Atari disk image; originally "Adventure! (1982)(Howell, Robert A.)(US)
[req OSa][BASIC].atr" from archive.org "Atari 8bit files [ATR] part 01") + the repository's README.
A cut-down Crowther & Woods: the "all different" maze shares one description, aboveground is four identical rooms,
15 treasures + magazines give 281 of 301 points and nobody has shown how to get the rest.
Not in eXo (its "0300-Point" is the C64/CPC game). Runs as is: `atari800 -atari -basic HOWE0301.atr` (needs OS rev A) -
so this is an eXo-style emulator entry unless the BASIC is extracted and ported.

**2026-09-19: emulation only, not to be ported (user's decision).** Checked in eXo's Altirra 4.40: it boots to the title
screen ("Do you want INSTRUCTIONS?") with 800 hardware, OS rev A and Atari BASIC rev A inserted *as a cartridge*:
`Altirra64.exe /hardware:800 /kernelref:REVANTSC.rom /memsize:48K /cartmapper 1 /cart REVA.ROM /disk HOWE0301.atr`.
`/basic` alone does nothing on 800 hardware (the 800 has no internal BASIC), and the disk then drops into the DOS 2.0S
menu. OS rev A is needed because AUTORUN.SYS replaces the E: handler with a table that jumps straight into OS-A editor
routines ($F369, $F3FB, $F633...) to type `RUN "D:INIT"`; INIT then runs `D:ADVENTUR` (642-line Atari BASIC program).
