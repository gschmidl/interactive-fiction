# Explore 5.3 (Jim Lippard, Multics BASIC, 1980)

**Status: PORTED 2026-09-28 (first pass).** `port\explore.exe` (`port\run.bat`) runs the game's BASIC, unchanged,
on the author's own Multics BASIC interpreter (MBasic, written in Perl), with a Perl of its own. See
`port\README.md`.

Explore is a cave adventure in the manner of Colossal Cave. Jim Lippard wrote it between October 1979 and June 1980,
aged 14, on Multics System M in Phoenix, for the Honeywell-sponsored Explorers Post 414; version 5.3 is dated
06 June 1980. You start at a shack in a forest clearing and go down into a cave of 58 rooms to collect treasures,
kill or dodge its creatures (a troll, a dragon, a cyclops, a scylla, dwarves with knives ...) and learn its magic
words. Every treasure brought back to the shack, and every creature killed, scores; 500 points is a perfect game and
the rank of Grand Master Explorer. A tablet in the cave lists the players who won in 1979-80.

It is its own game, not a variant of Colossal Cave, and it is not in the Adventure family tree.

## Source

The author's 2026 reconstruction, from his repositories on GitHub:
- `github.com/lippard661/explore`, commit a95fb61 (2026-09-26): the game;
- `github.com/lippard661/MBasic`, commit 00c7832 (2026-09-26): the interpreter.

In September 2026 he rebuilt version 5.3 from the line-printer output he still had: the BASIC source of 5.3
(printed 06/27/80), the database of a version 6.0 that was never finished (07/14/80), a play-through of version 4.3
(10/18/82) and the winners file (11/03/80). The helper subroutines, PL/I and BASIC, were lost; he wrote them again.
He fixed the bugs listed as CHANGES in `src_original\explore\Multics\README.md` (save/restore, the winners tablet,
the alarm), gated the multiplayer files behind a configuration file, and ran the result on a Multics in the DPS8M
simulator. For other systems he then wrote MBasic, a Perl interpreter for the part of Multics BASIC the game uses,
checked against the Multics BASIC manual (AM82) and a running Multics, and a Perl distribution of the game on it.

`src_original\` holds both repositories as they were published, version 5.3 only:
- `explore\` is lippard661/explore without the 6.0 database listing and the 4.3 play-through, which are in `doc\`
  with the photographs of their printouts, the photograph of all four printouts together, the OpenBSD package
  `p5-Explore-1.2.tgz` (a packed copy of `perl\`) and `.gitignore`. It contains:
  - `artifacts\explore.basic.5.3`: the 1980 printout of 5.3 as transcribed, 2134 lines (md5
    adcb8963b3965d15a3409859003a5db5), with photographs of its first page and of the winners printout;
  - `Multics\`: the reconstruction for Multics - `src\explore.basic` (5.3 with the 2026 changes, 2298 lines, md5
    a6cb53eccd562c2b1e965aa0c2427dea), the helpers in BASIC and PL/I, the data, `explore_setup.ec` and the info
    segment;
  - `perl\`: the Perl distribution 1.2 - the runner `explore`, `lib\Explore\Builtins.pm` (the helpers that were
    PL/I, and stand-ins for the Multics commands the game calls), `share\` (the same BASIC and data as `Multics\`)
    and its tests.
- `MBasic\` is lippard661/MBasic (version 1.2) without the OpenBSD package `p5-MBasic-1.2.tgz` and `.gitignore`.

The data: `explore.data` (md5 d1abee91c9a3323877fc0d06d514742b) is the author's 5.3 database, rebuilt from the 6.0
listing; `winners.data` is the 1980 printout; `hours.data` is new, since the 1980 one is lost: the cave is always
open and the sorcerer's word is "hello" (rot13 `uryyb` on its first line).

## Documentation (`doc\`)

The two printouts that are not 5.3, from `artifacts\` of lippard661/explore at the same commit, byte for byte:
- `explore.data.6.0` (677 lines, md5 704842ff9a97856e0e732c49e3ed52db), the author's transcription of the listing
  of the database for version 6.0 ("Initial Coding 07/12/80"), printed 07/14/80 as
  `>udd>MED>Kaiser>Lippard>explore_dir>explore.data.explore`, with photographs of the printout
  (`explore.data.explore.front.jpeg`, `.back.jpeg`). No 6.0 program was written. Its format is not 5.3's: rooms
  go by name, not number, in sections headed by comments (long and short descriptions, location data, objects,
  monsters, fixed and living objects, lighted rooms, text, vocabulary). The 5.3 database in the port was rebuilt
  from it.
- `explore.play` (634 lines, md5 cb7e6fd112bd5245b302b6ee7146fb5e), a play-through of version 4.3, printed
  10/18/82 from Steve Ditton's directory as `>udd>sct>sed>explore.play`, with photographs of the printout
  (`explore.play.front.jpeg`, `.back.jpeg`). It is a complete walkthrough, from the shack to the win with 500
  points in 178 turns, so it gives the whole game away. Version 4.3 differs from 5.3 in places (the rooms are
  described in other words), and the author notes that the transcript abbreviates and changes the original here
  and there.

Everything here is under the BSD 3-Clause licence (`src_original\explore\LICENSE`, `src_original\MBasic\LICENSE`).
