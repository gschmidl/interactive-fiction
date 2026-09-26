# The original program and data

There is no source for Adventure on the GAMES tape (`..\archive_original\ti990_games.tap.gz`; other games there do have
source). The files here were cut out of that tape by DX10 itself, then checked against DX10's own listings.

| File | What it is | md5 |
|---|---|---|
| `ADVEN.proc.bin` | procedure segment ADVEN (ID >01) of `.GAMES.PROG`: >B294 bytes loaded at >0000 (code, FORMATs, the FORTRAN run-time) | 7750f9aabd3ba23e833e2ca44632a241 |
| `ADVEN.task.bin` | task segment ADVEN (ID >08): >2494 bytes loaded at >B2A0 (transfer vector WP >B2A0, PC >B2A6, end action >5D48; the data) | a029ac36758e845a73653121e85ead86 |
| `CAVE.bin` | the data file `.GAMES.FILES.CAVE`: a relative record file of 28 records of 1728 bytes, the texts scrambled. Record 27 holds the wizard's settings: hours, holidays, the 90-minute wait, the magic word | 32f964363beff10d1a14112b95f8e791 |
| `PROC\ADVENTUR`, `PROC\CAVE$`, `PROC\IN` | the SCI command procedures that start the game (from `.GAMES.PROC`), as SF (Show File) listed them | |
| `GAMES.PROG.map` | MPF (Map Program File) of the program file | |

## How they were extracted (2026-09-21)

The reference machine is Dave Pitts' sim990 3.3.0 for Windows (`sim990win-3.3.0`)
with its DX10 3.7 disk. The run copy is in `..\..\..\_work\_TI990_work\run370`: `dx10run.cfg` attaches the tape on MT01 and
puts the system console on telnet port 2099. The tools are in `..\..\..\_work\_TI990_work\tools`.

1. On the console:
   - log on (Esc `!`);
   - set the clock with `IDT`, using a 4-digit year (a year of 2084 made MPF fail with 030E INTERNAL ERROR);
   - `RD MT01 .GAMES` (Restore Directory) restores the tape to `.GAMES.GAMES.*`;
   - `AS GAMES=.GAMES.GAMES` and `.USE .S$PROC,.GAMES.GAMES.PROC` let `ADVENTUR` run as it did.
2. `MPF` gives the map. The segments are then read straight out of the disk image `dx10_370.dsk`: a 16-byte header, then
   288-byte sectors, 3 sectors to an ADU.
   - The program file's directory starts at the file's record 0 (sector 46314 in that run).
   - Task entries are 16 bytes each at record 7 + >D0: length, flags, record, date, load address, priority/overlay,
     procedure, end.
   - Procedure entries follow the same layout.
   - ADVEN's task is at record >204 and its procedure at record >612, as plain images.
3. `CAVE`'s directory entry gives 56 ADUs from ADU >3BFE, in one extent. A second restore to `.GAMES2` put everything
   at other sectors; all three files came out byte for byte the same.
4. The segments were then checked with DX10's `SPI` (Show Program Image) on the console (`tools\spi990.py`): every word
   SPI printed matches.
   - SPI prints `SAME` for runs of rows. On re-dumping, those rows are zeros.
   - SPI takes the length as a signed number, so the procedure was dumped in two halves (>0000 + >7000, >7000 + >4294).
