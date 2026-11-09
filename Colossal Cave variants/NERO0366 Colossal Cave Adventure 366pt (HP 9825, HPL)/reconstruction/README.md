# NERO0366: Adventure for the HP 9825 (Nelson & Robinson, HP Desktop Computer Division)

Everything here was read off `adventure.dsk` (CRC D05BC0F2) in MAME's hp9825t.

- `listings/`: all 26 HPL programs, listed by the 9825 itself to an emulated HP 9871
  printer. `→` is HPL's assignment arrow; `#` is ≠.
- `data/`: the database decoded back into advent.dat-style text; `data/raw/` holds the
  source sections kept on the disk; `data/saves/` the game-state files; `records/` a
  record-by-record dump of every data file.
- `tools/`: `hpldisk.py` (disk directory + HPL data-file reader), `decode.py`, `dumpall.py`.
- `port/`: Windows console port (runs on Windows 7 too): the HPL programs translated to C,
  checked against the original in MAME; see `port/README.md`.
- `adventure.dsk`: copy of the disk (from eXo's NERO0366\HP-9825 Terminal).

## The game

- "The Adventure Game -- Colossal CAVE 1/14/80" (`ADVENT`, line 0). The intro credits
  Willie Crowther, Don Woods and Gary Palter, and names "Eric Nelson and Craig Robinson,
  Hewlett-Packard Desktop Computer Division, Ft. Collins, Colorado" as the developers.
- An expanded version: 144 locations (boat, reservoir, island beyond the 350-point
  game's 140), 212 messages, ranks up to 365 points ("Master Adventurer Class A").
- `ADVENT` sets up and chains to `A1`; `A1`, `A2` and `A3` chain to each other
  (`chain "A2",0,n`) with the command dispatched by the third argument. `A1T` is a
  near-copy of `A1`. Saves go to `ADVSAV`; `ADVNEW` is the new-game state built by `ADVMAK`.
- Other programs: `SECnP` build the data files from the source sections, `SECnHT`/`SEC12T`
  test them, `TRVLP` re-blocks the travel table into `TRVL`, `DLIST`/`DLISTR` are disk
  listers, `CHANGE` is "Search & Destroy Program 781031 mjr", `Funct` a function library.
- `ADVKEY` (special-function keys): f0 = `wtb S,15`, f6 = `wtb S,12` (raw bytes to the
  output device; 12 = form feed).

## Output device (for wiring up a wider printer)

```
33: cll 'Readrest';16→S→Y;ent "Enter output selectcode",S
34: if not flg13;ent "What is your printer's width?",Y
```

All game text goes through `wrt S`, word-wrapped to `Y` columns (`A1`–`A3`, "Print",
around line 44); commands are typed on the 9825 (`ent "?"`, shown on the LED display) and
echoed to the same device. Select code 16 is the built-in strip printer (the default);
any other select code works. The autoboot's "2, EXECUTE, CONTINUE" only evaluates 2 on
the display and then continues with no entry, so `S` stays 16.

Tested: an HP 9871 daisy-wheel printer on a 98032 GPIO card at select code 6
(`-slot1 98032_gpio -slot1:98032_gpio:gpio hp9871 -prin1 out.prn`, cfg port
`:slot1:98032_gpio:SC` value 5), answering 6 and 80: the whole game prints to `out.prn`
at 80 columns. MAME's select-code setting is off by one (value 5 = select code 6); value 6
(select code 7) clashes with the disk's HP-IB card and the drive stops answering.
A 98036 RS-232 card (`-slot1 98036`, default select code 11) also coexists with the disk.

## Data files

Directory: 16-byte entries from 0x100 (name, start sector, sectors, length, type);
types 0x0800 program, 0x0400 data, 0x2400 data (the source sections), 0x0c00 keys.
Items: number = header (low 6 bits 0x1a/0x1b = +/-, exponent = header >> 6) + 12 BCD
digits; string = 0x3c (whole) / 0x1c, 0x0c, 0x2c (first, middle, last part across
records) + length + bytes; 0x001e end of record, 0x003e end of file. Records are 256 bytes.

| File | Contents | Built by |
|---|---|---|
| SEC1H | long descriptions; record = location: number, text, link to continuation record (151+) | SEC1P |
| SEC2H | short descriptions; record = location | SEC2P |
| SEC3H | travel: `K$` (start index per location, fti) + `T[760]` (verb + 1000·destination, last entry of a location negative) | SEC3P |
| SEC4H | vocabulary: `V$` (5-letter words, sorted) + `I$` (values, fti) | SEC4P |
| SEC5H | objects: record = object: number, name, 7 pointers to state-message records (75+) | SEC5P |
| SEC6H | messages, chained like SEC1H (continuations from 225) | SEC6P |
| SEC10H | ranks | SEC10P |
| SEC12H | magic messages | SEC12P |
| TRVL | `T` in 32-number records | TRVLP |
| ADVNEW, ADVSAV, BIGSAV, BSTSAV, LSTSAV, OKSAV | game state (`ADVENT` "Readrest" order) | ADVMAK / the game |

`data/CHECK.txt`: every built file that still has its source section on the disk
(sections 2, 3, 4, 5, 10, 12 and TRVL) decodes identical to it. Sections 1 and 6 survive
only in built form; sections 7, 8, 9 and 11 only as source.
