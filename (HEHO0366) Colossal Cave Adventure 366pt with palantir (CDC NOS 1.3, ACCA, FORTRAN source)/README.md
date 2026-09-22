# Adventure, CDC NOS 1.3 (ACCA) - 366 points, with the gazebo and the palantir

**Status: PORTED 2026-09-21 (first pass).** `port\advent.exe` - see
`port\README.md`. Verified against the original: FTN 4.7 still compiles this
source as it stands on a Cyber 173 under NOS 1.3 (DtCyber), and three recorded
sessions replay byte-identically, dwarves and all. FORLIB's random number
generator was measured on the machine rather than guessed, and so was the fact
that FTN evaluates both sides of `.AND.` even when the first already decides it -
which is what governs when the dwarves appear.

Files here are verified copies (md5) of the originals named below.

Source: `C:\Users\gschm\Downloads\nd\Cyb\adventure.src` + `adventure.txt`.
"Updated from SCOPE 3.4 to NOS 1.3 by Bill Hein and Shelley Hobson (ACCA)"; chain Blackett IAS -> Supnik RT-11
(21-Oct-77). Adds an overgrown path south of room 5 -> dell 141 -> gazebo 142 (elvish runes, "PKIHMN"),
treasure 65 PALANTIR (ORB), which you **PEER** into for hints - the four letter word the hint means is PEER itself,
from the dell's own description; the hours messages call the cave MIRKWOOD. Real ASCII.
Not FUNADV's source (FUNADV's additions differ).

**The point total, settled 2026-09-21 (plan step 12).** The folder said 350 and
the user said 362. The game works its own maximum out while it scores, and what
it printed on the real machine is

     YOU SCORED  32 OUT OF A POSSIBLE 366, USING    9 TURNS.

so the folder is now 366. Sixteen treasures instead of Woods' fifteen: object 65,
the palantir, sits above the chest (55) and is therefore worth 16, which takes
350 to 366. That also agrees with `_bits_sweep_work\reference\adventure-family-tree`,
which names this lineage SCOP0366 - the SCOPE 3.4 version this source says it was
updated *from* - then LIDI0366, and has no 0362 in it at all.

Two things this version does that the 350 does not: the cave is **shut** from
06.00 to 11.30 and from 13.30 to 15.30 unless you can prove you are a wizard
(the password is WORMTONGUE), and `SCORE` asks whether you would like to quit
while it is at it.
