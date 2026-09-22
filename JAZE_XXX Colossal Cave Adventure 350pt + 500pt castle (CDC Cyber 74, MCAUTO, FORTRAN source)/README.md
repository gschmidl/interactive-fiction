# Adventure, MCAUTO Cyber 74 - BEG (350) and ADV (500, castle)

**Status: PORTED 2026-09-21 (first pass).** `port\advent.exe` - see
`port\README.md`. Verified against the original: FTN 4.7 on a Cyber 173 under
NOS 1.3 (DtCyber) compiles `ADVENT.txt` as it stands, and four recorded
sessions - both caves, the dwarves, the castle and the Black Wizard - replay
byte-identically. The game prints 350 for BEG and 500 for ADV, as this
folder's name says.

Files here are verified copies (md5) of the originals named below.

Source: `C:\Users\gschm\Downloads\nd\jase\` (3 files, complete copy).
- `ADVENT.txt` - CDC FORTRAN, Blackett IAS base with Gary Palter's wizard and prime-time machinery,
  "converted ... for use on the MCAUTO Cyber 74 by systems programmers Tony Jarrett and Paul Zemlin ... 12/17/78".
  Asks "BEG or ADV" (Black Wizard of the High East Tower); PFGETs =DATABS1/=DATABS2 from UN=XSY913.
- `001.2.txt` = BEG database (=DATABS1), the standard 140-room 350 cave.
- `001.1.txt` = ADV database (=DATABS2), 160 rooms: Egyptian room (73), iron door (78), castle 141-160 north-east
  of the forest, evil wizard in the dungeon (149), glass ball, castle-cave door, treasures 65-73 (ruby, oil paintings,
  ivory talisman, jade statue, black opals, Dead Sea scrolls, ermine robe, crown, scepter); scoring totals 500. Not
  our DG 500 and not eXo's "0500-Point" (Breen's Pascal game).

The text is a transcription in which three of the site's characters came out
as others: `%` is a colon, `\` a question mark, `^*` an exclamation mark
(`^` is never seen without the `*`). The fourth, `"`, stands for both the
apostrophe and the double quote and cannot be undone. The port prints what was
meant; `port\README.md` has the counts.

**Lost:** `=AMAINT`, the wizard's parameter file (prime time, magic word,
magic number, latency). `POOF` refuses to start without it (`STOP 77`), but
keeps the values it used to set as comments, and the port's copy is written
from those - weekdays 08.00-17.59 are prime time, and the magic word is DWARF.
