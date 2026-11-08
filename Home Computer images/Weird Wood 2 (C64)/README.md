# Weird Wood II: The Goblin Strikes Back

*Weird Wood II (The Goblin Strikes Back)*, a text adventure in Commodore 64 BASIC, last updated on
30 July 1985, known from a fanfold printout of its listing dated 21 May 1987 and a table of its 66
objects. Among other things you have to become a trustee, find a library ticket and build a matter
transmitter. The package is a Windows version of the game (`WW.exe`) with its data files.

## Command line

`WW.exe` has no options of its own. Its graphics library accepts `-geometry WxH+X+Y` for the window's
size and position, `-iconic` to start minimized, `-direct` or `-indirect` for the OpenGL rendering
context, and `-gldebug` to report OpenGL errors.

## Windowed patch (2026-11-08)

`WW.exe` (the loose copy and the one in `game.zip`) has been patched to start in a window: the
program's startup `_FULLSCREEN _SQUAREPIXELS` call (file offset 0x53F49, byte `03` → `01`) now
passes `_OFF`.

It is also DPI-aware now, so Windows no longer bitmap-stretches it (which made the text
uneven at scaling above 100%), and the font size follows the display scaling instead of being
a fixed 32 px: 20 px at 100%, 25 at 125%, 30 at 150% (cell 12×20, 15×25, 18×30). The 80×25
window is therefore always about 960×500 in scaled units, which fits smaller screens. This is
done by a small routine in the padding at the end of `.text` (VA 0x626B88), which became the
new entry point and calls `SetProcessDPIAware` before jumping to the original entry. The
constant font size before `_LOADFONT` (VA 0x454BBF) is replaced by a call that computes
`(LOGPIXELSY × 20 + 48) / 96`. The unpatched files are in the git history before this change.

## C64 disk images (2026-11-08)

`Weird Wood II (disk 1 program).d64` and `Weird Wood II (disk 2 data).d64` run the 1987 BASIC
listing on a Commodore 64, as the original did: load and run the program disk, and when it says
`PLEASE INSERT DATA DISK & PRESS RETURN`, swap in the data disk and press Return. In VICE, load the fliplist
`fliplist.vfl` (`-flipname`) and flip to the next disk (Alt+N), or attach disk 2 to drive 8
(Alt+8). Everything on them is rebuilt by
`reconstruction\c64 build\build_d64.py` from the transcription, `WWDAT.TXT` and `WW 0.DAT`; it
prints a report of each decision. Tested in VICE x64sc with true 1541 emulation: intro, room
descriptions, object lines, inventory, pictures, wizard commands, SAVE/LOAD.

The program is the transcribed listing, tokenized; VICE's `petcat` lists it back identical to the
transcription except for these deliberate bytes and changes:

* Zero-width control characters inside `""`: cursor-left in lines 808 and 828, upper-case/graphics
  mode in 23572 and 23576 (as in `WW2-OCR-notes.md`), and clear-screen in 100, 690 and 23674
  (inferred: the title screen, the page break in the message reader, the star-game header).
* The box in 3550–3558, the `7777…` under the title in line 100 (PETSCII 183) and the star-grid
  bars in 23644–23654 as PETSCII graphics; line 497's embedded shifted RETURN.
* **Lines 793 and 795**, which the printout blanks out as `REM********`, are restored as
  `IFA$="/"THENA$=CR$` and `IFA$="^"THENA$=CR$:T=10`. Every character of the room file passes
  through this decode routine, and without it the room reader (lines 732–760) never ends.
* **Line 11909** copies `WW 46` instead of `WW 44` back over the save file when you die. `WW 44` is
  the rank table (line 7809) and would make the next start fail; `WW 46` is the clean initial state.

The data disk holds:

* `WW 0`, the room file: a relative file of 565 records of 200 bytes (the Windows `WW 0.DAT`
  un-XORed). Each record is the 20-word exit table stored +1600 (lines 715–722), then the text;
  `/` ends a line and `^` ends the description. Long descriptions run on into the next one or two
  records, which the 1541 does by itself when a program reads past the end of a record (checked in
  VICE). Room 536's text fills all 160 bytes, so its padded line ends were turned into `/`.
* `WW 47`, the machine code at $C000, which is lost and was rewritten from how the program uses it:
  $C000 recolours the screen in the cursor colour (after F1/F3/F5, line 822; its first byte is $A0 as
  line 10 checks) and $C017 prints the room line of object A, for which line 196 pokes 2A+118 and
  2A+119 into $C01C and $C022, the low bytes of a word table at $C076. The 66 room lines are the ones
  at the end of `WW 42` in `WWDAT.TXT`.
* The sequential files from `WWDAT.TXT`, in PETSCII: messages `WW 1`–`WW 13` (read with `GET#`),
  pictures `WW 15`–`WW 39` (read with `INPUT#`, so every line is quoted; the PC box characters
  became PETSCII lines and corners), the tables `WW 40`–`WW 44` and `WW 48`, the save file `WW 45` and
  its clean copy `WW 46`, and the later version's `WW 49`–`WW 65`.

Where the later data and the 1987 program disagree:

* `WWDAT.TXT` comes from a 1988 revision ("M J Cox 1988") edited for the Windows port. In 1987 the
  star-game instructions are `WW 14`, with `^` for the header and `_` for "press SPACE"; they were
  taken from the later `WW 60`. The later `WW 14` ("Now, Now…") and `WW 47` (the wizard help) are
  not used.
* The noun table's entries 84 and 90–93 are plain words, which the program prefixes itself
  (lines 134–135); the port had split their first letter off as a number.
* `WW 48` has nine names, the program reads ten, so the first is repeated. Several of them (Donald
  Trump, Boris Johnson, Mo Farah) are clearly the port's.
* The save file's colours were all 0 (black on black); they are now black background and border
  with cyan text.
* The security pass in `WW 57` has the later layout, so line 3064's 14-space substitution does not
  apply.

## Recommended start

`WW.exe`, or the C64 disks above in VICE.
