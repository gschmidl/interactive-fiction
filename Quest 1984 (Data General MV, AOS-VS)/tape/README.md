# The tape

`AOS-VS_QUEST_game__1984.9trk` — a SIMH-format tape image of two AOS/VS
DUMP_II files, both dumped on 14 November 1984 from `:UDD:PIPER:QUEST`:

- file 1 (13:40:48): the game — `QUEST.PR`, `QUEST_SERVER.PR`,
  `FIXUP_OBJECTS.PR` with their `.ST`s, the four data files, `CASTLE`, and
  the CLI macros and FED scripts.  Two links, `QSET.PR` and `QSET.CLI`, point
  at `:UDD:PIPER:FORTRANS`.
- file 2 (13:43:43): `QSET` — FORTRAN source, `.OB`, `.PR`, `.ST`, `.CLI`.

`CONTENTS` is the listing the extractor printed (`../port/tools/loadg.py`
reads the same format as the NADGUG library tape, without the compression).
`../src_original` holds both files' contents as extracted; `../port/data` is
file 1.
