# Colossal Cave Adventure 350pt - Philips P7000 (Four-Phase), IDOS

**Status: PORTED (2026-09-21, plan step 25, first half).** The site's own disc pack runs on an emulated Four-Phase
IV/70: `port/run.bat`. See `port/README.md`.

Same tape as `..\..\Quest Variants\Quest (Philips P7000, Four-Phase MFE, multiplayer)`; read that README for the tape layout.

- `archive_original/DTUX_QUEST_ADV_HGHSEC.TAP` - the tape (md5 d12617056fd166a34bdce768c9630be9).
- `src_original/P7000.PACK` - the disc pack cut from it (md5 1c3f08442a9f80d8fb491528488f7882); its README lists the
  game's files and the site's NEW and OLD job files.
- `port/` - the emulator and front end (C), build scripts, tests.

The game is a compiled FORTRAN build of the 350-point Crowther/Woods Adventure with the wizard's machinery,
"ADVENTURE   07 JUNE 1978", adapted by the site: the magic words are ZYXXY and CLUNK, the cobble crawl is dark,
and the prime-time hours are shown but never enforced. The emulator, its tools and the manuals it was built from are
in `_FourPhase_work` (`docs/ISA_NOTES.md` sums up the machine).
