# Jaeger / Pohl C Adventure (12 June 1984) and Daimler's Turbo C 2.0 descendant (June 1990)

**Status: STAGED 2026-09-19 - not ported, nothing built.** Complete copy of the named directory of https://github.com/Quuxplusone/Advent at commit d38e82550600144e3547d6472bc80dbf49ca214b (2026-09-15), md5-verified against the clone.

- `src_original\POHL0350\` - PC-SIG disk 259, "Author Version: 03/90": BDS C conversion by J. R. Jaeger, Unix
  standardisation by Jerry D. Pohl, `ADVENT.DOC` dated 12 JUNE 1984. Behavioural differences from Woods: dwarves are
  re-placed at random every turn instead of walking, and a precedence bug in `turn.c` line 36 means "A little dwarf with a
  big knife blocks your way!" can never appear.
- `src_original\DAIM0350\` - if-archive `advtc2.zip`: the same code after Martin Heller's OS/2 conversion (30-Aug-1988) and
  Daimler's Turbo C 2.0 conversion; adds typos to the data files and disables READ, EAT, FILL "no room".
This is the C family that Hall's 7.0 (sibling folder) grew from. eXo's DOS 350 is a different program (the RT-11 FORTRAN
port with AINDX/ATEXT), so neither of these is in the collection. Port = compile for Windows.
