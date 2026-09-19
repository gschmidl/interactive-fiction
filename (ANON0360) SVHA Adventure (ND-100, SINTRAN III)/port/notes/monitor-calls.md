# SVHA Adventure: monitor calls beyond the SKATTEJAKT set

Analysis only. Nothing in `files\svha\` was changed. Tool: `tools\svha_scan.py`
(`scan` lists every 153xxx word, `ctx <mon,mon> [n]` disassembles around them,
`list <from> <to>` disassembles a range. The tool's own address arguments are octal,
but its displacements print as decimal.)
In this note, addresses and instruction words are octal. Counts and sizes are decimal
unless suffixed B. For example, RECL 36 = 44B, and `-200` in an instruction is -200B.

SKATTEJAKT set (MON-SPEC.md): 0 1 2 3 40 50 60 64 65 70 77 107 113 117 120 141 142 210 266 277 360 365.

## 0. Image and encodings used

* `SVHA-ADVENTURE.PROG`: header words 010000/010000/010000/134006/177777/0/0; addresses
  010000..134006 are big-endian words from file offset 0x200. The game (NORD FORTRAN
  code, literal pools, formats) runs 010000..~116200; the linked runtime library runs
  ~116250..134006.
* The last loaded words are loader-set descriptors, all **0** in this image and never
  written by code: 134001-134003 = overlay descriptor, 134004/134005 = `EXIT` hooks,
  134006 = alternate page table number.
* Memory above 134006 is used for COMMON/variables (for example 134354 and 134355 are
  set at 010251). The emulator must provide zeroed memory up to 177777.
* The instruction encodings were checked against the text of ND-06.029.1. **Appendix C of
  the OCR has wrong values**, and the manual's own worked examples prove it:
  IOX = **164000** (the appendix says 160000), RCLR = **146100** (141600),
  RINC = **146400** (146600), REXO = **145000** (140500).
  * Memory transfer instructions: bits 15-11 are the opcode, bit 10 = ,X, bit 9 = I,
    bit 8 = ,B, and bits 7-0 are a signed displacement. STZ 000000, STA 004000,
    STT 010000, STX 014000, STD 020000, LDD 024000, STF 030000, LDF 034000,
    MIN 040000, LDA 044000, LDT 050000, LDX 054000, ADD 060000, SUB 064000,
    AND 070000, ORA 074000, FAD/FSB/FMU/FDV 100000/104000/110000/114000,
    MPY 120000, JMP 124000, JPL 134000.
  * Conditional jumps are 13x000, with bits 10-8 = JAP JAN JAZ JAF JPC JNC JXZ JXN.
  * SKP is 140000 + cond(bits 10-8) + sr(5-3) + dr(2-0).
  * Register operations: SWAP 144000, RAND 144400, REXO 145000, RORA 145400,
    RADD 146000 (bit 8 = AD1, bit 9 = ADC, bit 7 = CM1, bit 6 = CLD). COPY = 146100,
    EXIT = 146142.
  * Argument instructions: SAB/SAA/SAT/SAX 170000/170400/171000/171400 and
    AAB/AAA/AAT/AAX 172000-173400.
  * Bit instructions: 174000 + op(bits 10-7) + bit(6-3) + reg(2-0), where register 0 = STS.
  * Shifts: 154000 + type(bits 10-9: ROT/ZIN/LIN) + reg(8-7: T/D/A/AD) + signed
    6-bit count.
* ND-100 return convention (ND-60.050.06 section 3.6): register-parameter calls return
  to P+1 on **error with A = error number** and to P+2 (skip) on success.
  Standard-call-format calls (A -> parameter list: 104, 113, 117, 120, 141...) return
  to P+1 with the status in A. Every call site below agrees with this.

## 1. Every 153xxx word in the image

"Reached" means reached by this game's actual use of the runtime, from its 5 OPENs,
2 CLOSEs, 58 FIO statements and library stub calls.

| MON | address(es) | verdict | reached? | context |
|---|---|---|---|---|
| 0 | 133775 | code | yes | STOP/END: `LDA (133775); JPL I ->134004 (EXIT hook); MON 0` |
| 1 | 116350 | code | yes | runtime line editor `SAT 1; MON 1; JMP err; AND 177` |
| 1 | 133334 | code | **yes** | FIO in-byte for T >= 100B. Unformatted READ of **DATAFIL** (unit 60) byte by byte |
| 1 | 133346 | code | no | FIO formatted-READ-from-terminal editor. SVHA has no terminal FIO READ |
| 2 | 116403 116406 116411 116416 116423 116464 116467 116472 116517 116542 | code | yes | echo from the line editor: BS/SP/BS, `^`, BEL, CR, the typed character |
| 2 | 116502 116505 116510 | code | no | `'_' CR LF` kill echo: no jump reaches 116500 |
| 2 | 116621 | code | yes | OUTCH(unit,char) stub, 3 call sites. 010005 sends OUTCH(1, 31B = ^Y) |
| 2 | 133360 133363 133366 133402 133423 | code | no | FIO terminal-READ editor |
| 2 | 133622 | code | yes | FIO out-byte (all WRITEs) |
| 3 | 116332 | code | yes | line editor, A = -1 |
| 3 | 116642 | code | yes | ECHOM(dev,strat,table) stub, called once as ECHOM(1,-1) at 010261 |
| **4** | 116334 | code | **yes** | line editor, A = 0 |
| **16** | 116322 | code | **yes** | line editor, T = 0 |
| **33** | 117751 122070 122306 130565 | code | no (guarded) | only if word 134006 != 0 |
| **34** | 117235 120135 121424 122055 122267 130523 133772 | code | no (guarded) | `LDA I (134006); JAZ *+2; MON 34` |
| **41** | 121640 | code | no | FIO WRITE to a disk file 100B-117B: PRINT-file check |
| **43** | 120266 | code | **yes** | CLOSE statement: CLOSE(60) at 010115, CLOSE(-1) at 011053 |
| **43** | 117545 117645 | code | no | OPEN re-open paths (ACCESS='D'/'SPECIAL'/READ-statement modes) |
| 50 | 117506 | code | yes | OPEN statement, 5 times |
| **62** | 117677 | code | no | OPEN with ACCESS=READ/WRITE/DIRECT/SEQUENTIAL on a disk file |
| 64 | 116426, 130544 | code | on error | line-editor I/O error; FORTRAN run-time error |
| **73** | 120263 | code | no | CLOSE of a buffered file in write state |
| **76** | 117517 | code | **yes** | OPEN with RECL=: A = 36 (the four text files) |
| **76** | 117637, 117710 | code | no | retry path; buffered-file setup (A = 2000B) |
| 77 | 121510 | code | no | FIO REC= on a plain file. None of the 58 FIO control words has bit 6 (REC=) |
| **104** | 116644 | code | **yes** | HOLD stub, 6 call sites |
| 113 | 116646 | code | yes | CLOCK stub, 1 site (020524) |
| 117 | 116602 | code | **yes** | RFILE stub, 1 site (017773, in a loop) |
| 117 | 117540, 133534, 133702 | code | no | OPEN probe; buffered-file read |
| 120 | 120251, 133524, 133663 | code | no | CLOSE flush; buffered-file write. **SVHA never writes a file** |
| 141 | 122337, 127411 | code | no | end-of-record IOSET, only for devices 20B 21B 25B 32B 33B 34B 40B 41B |
| 142 | 130711 | code | on error | run-time error message |
| **164** | 125561 | **DATA** | - | inside the floating-constant table 125524-125567 (6-word groups `0,0,a,b,c,d`); 153164 is a mantissa word |

**Disguised calls (also in SKATTEJAKT at 42274/42355):** 116671 = `161042 161065` and
116755 = `161007 161065`. Their register setup matches MON 42 (old OPEN: T = 3,
A -> `OVLY\'`), MON 7 (RPAGE loop: T = file, A = block, X += 400B) and MON 65 (QERMS)
as the error return. These are 153xxx + 6000B, in the overlay loader, which is only
active when word 134002 != 0. Word 134002 is 0, so the code is dead. The shim should
trap 160000-163777 as an illegal instruction with a diagnostic, and not treat it as MON.

## 2. Extra calls: specification and shim behaviour

Rule for all of them: change only the registers documented as outputs. MON 16's caller
relies on T being preserved.

### MON 4 - BRKM / SetBreak (genuine, executed)
* **Entry:** A = break strategy (<0 none, 0 = every character, 1 = control characters
  (the default), 2 = MAC, 3-6 system tables, 7 = user table in X (8 words, bit set =
  break), 8 = last user table, 9 = count only). D = maximum characters (strategies
  >= 3). X -> table. T = device, used by RT programs only; background programs use
  their own terminal.
* **Exit:** return to P+1. No error return and no skip: SVHA's next instruction after
  MON 3 is `COPY DA`, then MON 4, then `LDX`.
* **SVHA:** once per line input (116334), with A = 0 and T = 0/1. It is never restored.
* **Shim:** record the strategy for the console. Strategy 0 means MON 1 on the terminal
  must return **each keystroke immediately** (raw, unbuffered).

### MON 3 - ECHOM (already in the set; SVHA-specific use)
* A < 0 turns echo off. SVHA calls `ECHOM(1,-1)` at start-up (010261) and A = -1
  again before every input line. It never restores echo.
* The runtime line editor at 116275 (3 game call sites: 016251, 031656, 037136) echoes
  everything itself with MON 2:
  * DEL (177B) or ^A (1) deletes a character. It outputs BS SP BS on a VDU and `^`
    otherwise, and BEL at the start of the buffer.
  * ^Q (21B) or ^K (13B) kills the line. On a VDU it outputs BS SP BS back to the start.
  * CR echoes a bare CR and returns. The buffer is blank-padded.
  * Any other control character gives BEL.
* **Shim:** do not echo on the host. Map the Windows Backspace key (010) to **177**,
  otherwise the game rings the bell. Restore the host console mode at MON 0.

### MON 16 - MGTTY / GetTerminalType (genuine, executed)
* **Entry:** T = logical device number. The manual says to use 1 for your own terminal
  in background programs; **SVHA passes T = 0**.
* **Exit:** error return P+1 with A = error. Skip return P+2 with **A = terminal type
  word**.
* **Terminal type word** (Developer\MON\Monitor Calls.md, "ND Terminal Types"):
  | bit(s) | meaning |
  |---|---|
  | 14 | VDU |
  | 13 | handles BS |
  | 12 | FF clears the screen |
  | 11 | cursor addressing |
  | 10 | ESC input |
  | 7-0 | model |

  Examples: 166006B = DEC VT100, 164007B = TDV-2000, 0 = not set.
* **SVHA:**
  * 116323 `COPY DT` is the error-return instruction, so T = 0 there.
  * On a skip return it sets the video flag = bit 14 AND bit 13 (T must still be 0
    before `RINC T`).
  * Video flag = 1 gives BS SP BS erasing. Video flag = 0 gives hard-copy editing
    (`^` echo; kill does not visibly erase).
* **Shim:** treat T in {0, 1} as the console. Skip-return A = **166006B** (VT100).

### MON 43 - CLOSE (genuine, executed)
* **Entry:** T = file number. T = -1 closes all files that are not permanently open;
  T = -2 closes all files, including scratch files. The two descriptions in the
  Users Guide are garbled OCR; the FORTRAN section gives this meaning.
* **Exit:** error return P+1 with A = error. SVHA answers with `JMP I` to the run-time
  error and abort. Skip return P+2 goes to `EXIT`.
* **SVHA:**
  * CLOSE(60) after loading DATAFIL: T = its file number.
  * CLOSE(UNIT=-1) at the end: the runtime first closes every connected unit 0..99
    individually (MON 43 with T = the file number), then issues MON 43 with **T = -1**.
* **Shim:** fclose. -1 and -2 both close everything. Always skip-return for -1/-2 and
  for a file that is really open. An unknown number gives an error return with A = 132B.

### MON 76 - SETBS / SetBlockSize (genuine, executed)
* **Entry:** T = file number, A = block size **in 16-bit words on the ND-100**. The
  sources are ND-60.050.06 section 3.6.1 ("A = block size (in words)") and the ND
  FORTRAN manual C.2.4 (ND-100 factor 2). The newer manual's "bytes" wording is the
  ND-500 view.
* **Exit:** error return P+1 with A = error. Skip return P+2.
* The default block size is 256 words (512 bytes). It resets when the file is closed.
* **SVHA:** right after each MON 50 whose OPEN has RECL=36: T = file, A = **36** (the four
  text files).
* **Shim:** store the block size on the open file. The following RFILE byte offset is
  `block * blocksize_words * 2`, and `<no of words>` is also in words (36 words = one
  72-byte record).

### MON 104 - HOLD / SuspendProgram (genuine, executed)
* **Entry (standard format):** A -> parameter list [&count, &unit]. Unit 1 = basic
  time units (20 ms), 2 = seconds, 3 = minutes, 4 = hours.
* **Exit:** return to P+1. The stub is `MON 104; EXIT`, so the value is ignored.
* **SVHA calls:**
  * HOLD(1,2) after loading (010122).
  * HOLD(var,var) at 010313.
  * A dramatic sequence at 035502-035543: HOLD(5,2), HOLD(30,2), HOLD(10,2),
    HOLD(10,2). That is 55 s of waiting.
* **Shim:** `Sleep()`, with A = 0 on return. Suggest a GNU-style `--no-hold` option or a
  keypress to cut it short.

### MON 41 - ROBJE / ReadObjectEntry (genuine code, not reached)
* **Entry:** T = file number, A = address of a 32-word buffer (64-byte object entry).
* **Exit:** error return P+1 with A = error. Skip return P+2.
* **SVHA runtime (121636):** called only for a formatted WRITE to file numbers
  100B-117B. It tests bit 2 of word 14 (the file-type word) to decide PRINT-file
  carriage control. Either an error return or a zeroed entry means "not a PRINT file".
* **Shim:** skip-return a zeroed buffer, and log the call.

### MON 62 - RMAX / GetBytesInFile (genuine code, not reached)
* **Entry:** T = file number.
* **Exit:** error return P+1 with A = error. Skip return P+2 with **AD = byte count**
  (A = high word).
* **SVHA:** only in the OPEN path for READ/WRITE/DIRECT/SEQUENTIAL access, which sets up
  the runtime's own 1024-word buffers. That setup also runs MON 76 with A = 2000B.
* **Shim:** return the host file size.

### MON 73 - SMAX / SetMaxBytes (genuine code, not reached)
* **Entry:** T = file number, AD = maximum byte pointer (byte count - 1). The runtime
  decrements it itself at 120257-120261.
* **Exit:** error return P+1 (SVHA ignores it with `JMP *+1`). Skip return P+2.
* **SVHA:** only when closing a buffered file that was being written.
* **Shim:** record the length and truncate at CLOSE.

### MON 33 - ALTON / AltPageTable and MON 34 - ALTOFF / NormalPageTable (dead code)
* **Entry:** MON 33 takes A -> parameter list [&page table number]; SVHA passes
  &word 134006. MON 34 has no parameters.
* **Exit:** as coded in SVHA, both return to P+1. No error-return instruction follows
  them.
* Every site is guarded by `LDA I (134006); JAZ`, and word 134006 = 0 (single-bank
  program, bank 2 = 177777).
* **Shim:** no-op with a return to P+1, and log if ever hit. Being hit would mean a
  loader flag the emulator does not model.

### MON 164 - WSEG / SaveSegment - not used
Data word; see the table in section 1. For reference: A -> [&segment number], writes
changed pages of the segment back to disk. A no-op in a shim.

## 3. The SVHA FORTRAN runtime and its files

* **Compiler/runtime:** NORD-10/ND-100 FORTRAN SYSTEM (ND-60.074-style `OPEN(unit,
  FILE=, STATUS=, ACCESS=, RECL=)`, with RECL in words). The library is linked in at
  116250-134006, right after the game, and the program loads and starts at 010000.
  * Calls go through `JPL I ->116720`, followed by the routine address, a
    count/flags word and the parameter words.
  * Library entries start with `RINC P` (146402) and a descriptor word.
  * The runtime shares the call handler, OUTCH stub, unit map and the dead overlay loader
    with SKATTEJAKT. The OPEN keyword runtime, CLOSE, FIO and line editor differ.
* **48-bit floating point is required.** The RFILE stub copies parameters 1-3 with
  `LDF ,X 0 / STF ,B -200` and parameters 4-5 with LDD/STD. The floating accumulator
  must therefore be 3 words (A,D,T), per ND-06.029.1 "48-bit Floating Point". A 32-bit
  LDF breaks RFILE.
* **EXR SA** (140650) is used 33 times for computed GOTO (A = index + `JMP I 1`).
* **Tables:**
  * Unit table at 132732+unit holds the file number (0 = not connected; then the unit
    number is used as the device number, so unit 1 = terminal).
  * Buffered-file table: 8-word entries at 133076. It is used only by READ/WRITE-statement
    access modes, and SVHA creates none.
* **OPEN (117231):**
  * The runtime copies FILE= up to the first blank (maximum 63 characters) and appends
    `'`.
  * For STATUS='NEW' it wraps the name in `"..."`.
  * MON 50 is called with X -> name, A -> `SYMB'` (default type) and T = access code.
  * Access codes: W=0 R=1 WX=2 RX=3 RW=4 WA=5 WC=6 RC=7.
  * READ maps to 3; WRITE, DIRECT and SEQUENTIAL map to 2 and set a runtime buffering
    flag.
  * D and SPECIAL map to 8, which means: open with 2, probe with a zero-length RFILE,
    and on error 242B re-open sequentially.
  * After OPEN: MON 76 if RECL= was given; then unit table := file number.
* **SVHA's five OPENs** (strings at 011120ff, parameter pools at 010200ff):

  | unit | FILE= | STATUS | ACCESS -> MON 50 T | RECL -> MON 76 | host file |
  |---|---|---|---|---|---|
  | 60 | `(GAMES)SVHA-DATAFIL` | OLD | R -> 1 | none | SVHA-DATAFIL.SYMB (35932 bytes, **binary** big-endian words) |
  | 20 | `(GAMES)SVHA-STLOTEXT` | OLD | RX -> 3 | 36 | SVHA-STLOTEXT.SYMB (493 x 72) |
  | 30 | `(GAMES)SVHA-KOLOTEXT` | OLD | RX -> 3 | 36 | SVHA-KOLOTEXT.SYMB (442 x 72) |
  | 40 | `(GAMES)SVHA-OBJETEXT` | OLD | RX -> 3 | 36 | SVHA-OBJETEXT.SYMB (563 x 72) |
  | 50 | `(GAMES)SVHA-TILFTEXT` | OLD | RX -> 3 | 36 | SVHA-TILFTEXT.SYMB (601 x 72) |

  The (GAMES) user prefix and the default SYMB type must map onto these host names.
* **DATAFIL** is read by unformatted sequential READ (FIO control word 5000B, format
  parameter 0, 25 statements at 011366-012175) and then closed.
  * Each byte is **MON 1 with T = file number**. MON 1 on a disk file must be raw 8-bit:
    no 7-bit mask and no CR/LF handling. EOF is an error return with A = 3.
* **Text files** are read only by `CALL RFILE(unit, 0, buf, rec, 36)` (subroutine 017756,
  753 call sites).
  * The stub maps the unit to a file number and calls MON 117 with A -> [&file, &0, buf,
    &block, &36].
  * Records are 72 bytes, with no CR/LF, at offset block*72. The function value (A) is
    ignored by the caller.
* **No file is ever written.** There is no WFILE, no W/WX open and no save file. MON 120,
  73 and 62 are never reached.
* **Terminal output:** FIO emits LF *before* each record and CR **with bit 7 set
  (215B)** after it. `+` suppresses the LF and `1` gives FF. The line editor echoes a
  bare CR on Enter, so MON 2 to the terminal must mask bit 7 and treat CR and LF as
  separate carriage motions.

## 4. Open questions
1. MGTTY with T = 0: does real SINTRAN III take 0 as "own terminal" or give an error
   return? An error return would give SVHA hard-copy editing. The recommendation of
   166006B is a UX choice, not proven.
2. OUTCH(1, 31B = ^Y) at start-up (010005): which terminal function did the author's
   terminal give EM? The other OUTCH sites (010300, 010320) use variables not resolvable
   statically. Likely a clear/home code. Map it once the model is known, otherwise drop
   it.
3. 161xxx pseudo-MON words: the evidence says "MON n + 6000B" in the loader's overlay
   path, but the trap mechanism is unknown. They are harmless while 134002 = 0.
4. The parameter-word encodings used by the call handler (004xxx/005xxx/016xxx/044xxx/
   1656xx) were read only as far as needed (P-relative constants and descriptors).
   The flag word of CLOSE at 011053 and the HOLD/OUTCH variable arguments were not
   resolved.
5. Error code 242B (the OPEN probe) is not identified. It is in an unreachable path.
