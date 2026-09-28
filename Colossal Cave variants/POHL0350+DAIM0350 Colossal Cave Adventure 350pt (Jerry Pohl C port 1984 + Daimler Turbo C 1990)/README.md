# Jaeger / Pohl C Adventure (12 June 1984) and Daimler's Turbo C 2.0 descendant (June 1990)

**Status: PORTED 2026-09-19, refined 2026-09-21 - `port\play-pohl.bat`, `port\play-daimler.bat`, see
`port\README.md`.** Five original bugs fixed behind `--no-fixes`; Daimler's port matches his own DOS program
(`archive_original\advtc2.zip`, run under DOSBox) byte for byte. `archive_original\adv.arc` is Pohl's 1984 DOS build,
an earlier revision than the 1990 source ported here. Staged the same day: complete copy of the named directory of https://github.com/Quuxplusone/Advent at commit d38e82550600144e3547d6472bc80dbf49ca214b (2026-09-15), md5-verified against the clone.

- `src_original\POHL0350\` - PC-SIG disk 259, "Author Version: 03/90": BDS C conversion by J. R. Jaeger, Unix
  standardisation by Jerry D. Pohl, `ADVENT.DOC` dated 12 JUNE 1984. Behavioural differences from Woods: dwarves are
  re-placed at random every turn instead of walking, and a precedence bug in `turn.c` line 36 means "A little dwarf with a
  big knife blocks your way!" can never appear.
- `src_original\DAIM0350\` - if-archive `advtc2.zip`: the same code after Martin Heller's OS/2 conversion (30-Aug-1988) and
  Daimler's Turbo C 2.0 conversion; disables READ, EAT, FILL "no room". Its text files are Pohl's 1984 ones (those of
  `archive_original\adv.arc`), changed only in the welcome (message 65), which credits Heller and Daimler: their small
  slips are the 1984 originals, which Pohl's own 1990 revision above corrects, not typos added by Heller or Daimler. An
  Archimedes port of November 1988 (AARC0350 in the collection) has the same 1984 text.
This is the C family that Hall's 7.0 (sibling folder) grew from. eXo's DOS 350 is a different program (the RT-11 FORTRAN
port with AINDX/ATEXT); these two ports are in the collection as POHL0350 and DAIM0350. Port = compile for Windows.
