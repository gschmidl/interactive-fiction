# src_original — provenance and reconciliation

Source medium: the RP06 pack `t10.dsk` from **"TOPS-10 in a Box" v1.1** (Jimmy Maher, 2011),
directory `DSKB:[1,2]`. Three independent extraction routes were run and reconciled here.

## Verdict in one line

**All three routes produced byte-identical output for all five text files.** The winner is
therefore not a choice between conflicting candidates but a confirmed consensus, independently
re-derived a fourth time during this reconciliation.

## What each route did, and what each is authoritative for

| route | method | independent of | authoritative for |
|---|---|---|---|
| **A** | from-scratch TOPS-10 filesystem reader (HOME to MFD to UFD to RIB to retrieval pointers) run offline against the image | the running OS | file extents; the non-contiguity of `ADVENT.EXE`; truncating the binaries to words-written |
| **B** | content-only scan: whole image decoded as packed 7-bit text, runs located by content signatures, **no** directory or RIB field parsed | the filesystem layout | that the answer does not depend on any filesystem assumption |
| **C** | **booted the pack** under SIMH and ran a FORTRAN-10 program *on the PDP-10* that opened each file `MODE='DUMP'` and typed the 36-bit words as octal | the offline decoders | the true filenames (`DIR`) and the exact byte count (`DIRECT/DETAIL` "Words written") |

Route C is treated as the reference because TOPS-10 itself reported both the names and the
lengths. Routes A and B independently confirm the bytes, which matters because route C's payload
had to cross a terminal.

## Checks performed during reconciliation (not taken on trust from any route)

1. **Re-unpacked route C's raw octal dumps from scratch** (`work/route_c/out/raw/*.oct`) with
   an independent implementation. Reproduced all five delivered files exactly.
2. **Re-read the blocks straight out of `t10.dsk`** at the extents routes A and B report,
   unpacked 5 x 7-bit per 36-bit word, and got the same five files exactly. This closes the loop
   between the machine's view of the pack and the image's view.
3. **Low bit of every word is 0** in all five files (36 bits = 5 x 7 + 1 spare). Nothing was lost
   by the 5-character-per-word unpacking — the one assumption all three routes share.
4. **No NUL byte occurs anywhere inside any file**; NULs appear only as padding in the final
   partial word, and everything past "Words written" is NUL. So the trailing-NUL strip is a
   measured fact, not a heuristic — route B's `rstrip` and route A's `.RBSIZ` land on exactly the
   byte TOPS-10's own word count predicts.
5. **Case survived.** `ADVENT.FOR` contains 926 lowercase characters. Route C's line-printer and
   console channels would have folded case and mangled tabs; it rejected both and used the octal
   dump instead. The delivered file proves that was the right call.
6. **Tabs survived.** `ADVENT.FOR` line 3 is stored `C<SP><SP>CURRENT LIMITS:` (two spaces) and
   lines 5-12 are `C<TAB>`. A console capture with `SET TTY TAB` compresses the former into a tab
   and corrupts 111 lines; the delivered file has the spaces.

## Byte counts reconcile exactly with TOPS-10's own accounting

`words written x 5`, minus the NUL padding inside the last word:

| file | words written (DIRECT/DETAIL) | x5 | NUL pad | delivered bytes |
|---|---|---|---|---|
| ADVENT.FOR | 16372 | 81860 | 3 | **81857** |
| ADVENT.DAT | 11091 | 55455 | 0 | **55455** |
| IOFIL.FOR  |   107 |   535 | 1 |   **534** |
| ADV.F4     |  2806 | 14030 | 3 | **14027** |
| ADV.DAT    |  4098 | 20490 | 2 | **20488** |

## Line endings, tabs, padding — the routes do **not** disagree

Zero disagreement on any of these. Reported as found, preserved verbatim:

| file | line endings | tabs | form feeds | trailing WS | ^Z / EOF mark |
|---|---|---|---|---|---|
| ADVENT.FOR | **bare LF** (2948) | 2165 | 14 | 1 line | none |
| ADVENT.DAT | **bare LF** (1808) | 2675 | 0 | 0 | none |
| IOFIL.FOR  | **CR LF** (19)  | 18 | 0 | 0 | none |
| ADV.F4     | **CR LF** (744) | 624 | 0 | 0 | none |
| ADV.DAT    | **CR LF** (733) | 1067 | 0 | 1 line | none |

The mixed line endings are genuine: the two Woods-era files (written to the pack 17-May-2011)
are LF-only; the three 2007-era Crowther/DECUS files are CRLF. No `^Z` or other EOF mark exists
in any file. Nothing was normalised.

## sha256 of the files in this directory

```
a52830ca4bcc5d508290fe89b5507d26fa55441119869e9a6dd5a736ed677fa9  ADVENT.FOR
a52830ca4bcc5d508290fe89b5507d26fa55441119869e9a6dd5a736ed677fa9  ADVENT.F4   (identical copy, see below)
ab024845bb60f1ff753b25103de578120db307c672575e7435aeede4c9cc15ff  ADVENT.DAT
2bb91ef6549126f388bd6a0c55dcedefda8a92578d355ad95ebbe4fb9e49b6da  IOFIL.FOR
4e938d74eb908f67c9b46ad700bc15d54ab0f1d605156c099f0ef52d5c470e88  ADV.F4
94094e1f3cef70c0dca442609db864cd7c7ec71ff59d45fc30c0c2588c57a0f2  ADV.DAT
e8c09e83b5b75a67b10e750558ea0ab4aa18f33018c179c43656ae2c6d1a9a40  DIR.txt
```

## The filename `ADVENT.F4` is wrong, and the README is the thing that is wrong

The distribution README calls Woods' source `ADVENT.F4`. **The pack does not.** TOPS-10's own
`DIR` of `DSKB:[1,2]` lists ten files and the source is `ADVENT.FOR`:

```
ADV     F4     22  <057>   14-Aug-07    DSKB:   [1,2]
ADV     EXE    48  <057>   14-Aug-07
IOFIL   FOR     1  <057>   14-Aug-07
ADV     DAT    33  <057>   14-Aug-07
ADV     REL    38  <057>   18-May-11
IOFIL   REL     2  <057>   18-May-11
ADVENT  DAT    87  <000>   17-May-11
ADVENT  FOR   128  <000>   17-May-11
ADVENT  REL   145  <057>   18-May-11
ADVENT  EXE   324  <057>   18-May-11
  Total of 828 blocks in 10 files on DSKB: [1,2]
```

All three routes reached this independently: route C from the OS, routes A and B from the image
(route A additionally scanned all 39.4M words for a SIXBIT `ADVENT` RIB with an `F4` extension
and found none). Crowther's `ADV.F4` does exist and does match the README.

`ADVENT.FOR` is the genuine artifact. `ADVENT.F4` here is a **byte-identical convenience copy**
under the name the README and the task use. It is not a second file on the pack. If you want only
what is genuinely on the pack, delete `ADVENT.F4`.

## FORTRAN-10 sanity check of the winning source

Checked with a tab-aware FORTRAN-10 parser (DEC convention: a leading TAB advances to column 7;
TAB followed by a digit 1-9 is a continuation line).

| | ADVENT.FOR | ADV.F4 | IOFIL.FOR |
|---|---|---|---|
| lines | 2948 | 744 | 19 |
| program units (SUBROUTINE/FUNCTION + main) | **31** | 5 | 2 |
| `END` statements | **31** | 5 | 2 |
| **dangling label references** | **0** | **0** | **0** |
| unbalanced / unlabelled `FORMAT` | 0 | 0 | 0 |
| malformed `DATA` (odd slash count, unbalanced parens) | 0 | 0 | 0 |
| code lines past column 72 | **0** | 0 | 0 |
| characters outside 7-bit ASCII | 0 | 0 | 0 |

Every label referenced by `GOTO`, computed `GOTO`, `DO`, arithmetic `IF`, `ASSIGN`, `END=`/`ERR=`
and every I/O format reference is defined inside its own program unit. Nothing is dangling.

### Odd but original — *not* extraction damage

* `DATA MASKS/"4000000000,"20000000,.../` — a leading `"` is FORTRAN-10's **octal constant**
  prefix, not a string delimiter. Same in `ADV.F4` (`J="200000000000`). Correct for the dialect.
  A naive checker reads these as unterminated string literals; they are not.
* `TYPE 1` / `1 FORMAT()` in `HOURS` — a genuinely empty FORMAT, legal in FORTRAN-10.
* `ADVENT.FOR` line 2508 is `<TAB>STOP<SPACE>` — one trailing space, the only one in the file.
  An author's typo, preserved.
* `ADV.F4` line 740 is `<TAB>j = 0` — the file's **single** lowercase character, mirroring
  `J = 0` five lines earlier. Crowther's typo, preserved.
* `ADV.DAT` has an interior **blank line** between location 30 and location 31 (route B places it
  mid-block, far from any block boundary — so not a splice artifact), and **location 26 has no
  description and no travel table** although section 3 uses 26 as a destination. Both are
  properties of Crowther's unfinished 1976 game, not of the extraction.
* `ADVENT.FOR` contains 14 form feeds used as page breaks. Kept.
* `ADVENT.DAT` parses into sections 1,2,3,4,5,6,7,8,9,10,11,12 then the `0` terminator, in order;
  every section-3 travel destination has a section-1 description.

## Remaining doubt

1. **One assumption is shared by all three routes**: that text is packed five 7-bit characters per
   36-bit word, high-order first, low bit spare. It is not independently provable from the data
   alone — but the low bit is 0 in every word of every file, every resulting character is printable
   ASCII or TAB/LF/CR/FF, there are no NULs except as final-word padding, and the result is
   coherent English and well-formed FORTRAN. Route C's console `TYPE` channel, where the OS itself
   did the word-to-character conversion, agreed byte-for-byte for `ADVENT.DAT` and `IOFIL.FOR`.
2. **Nothing here was compiled.** No route rebuilt `ADVENT.FOR` + `IOFIL.FOR` into a working
   binary. All verification is static. Route C did boot the pack and run the *shipped* `ADVENT.EXE`
   successfully, which proves the pack works — not that the recovered source rebuilds.
3. **The shipped `ADVENT.EXE` is not a clean build of this source.** `POOF` in `ADVENT.FOR` sets
   `WKDAY="00777400` — the original restricted cave hours (prime time 08:00-17:59). The README
   states the shipped executable has been configured for unfettered 24-hour access. So the binary
   was altered after compilation (via the game's own magic mode, which rewrites the saved core
   image); the source is unmodified. Recompiling will give you the restricted-hours game.
   `MAGIC='DWARF'` and `MAGNM=11111` in the source match the README's documented magic word and
   number.
4. **These are 2011 copies, not 1977 artifacts.** `DIRECT/DETAIL` dates `ADVENT.FOR` and
   `ADVENT.DAT` to 17-May-2011 with protection `<000>`; the `ADV.*` files are 14-Aug-2007 with
   `<057>`. Maher transferred the Woods source onto the pack when building the box. The pack is
   the provenance, not the origin.
5. **A cosmetic disagreement about `ADVENT.EXE`'s layout, with no effect on output.** Route A
   describes it as two extents (two retrieval-pointer groups, cluster 26936 skipped); route C, by
   block hashing, describes 21 contiguous runs. Different accounting of the same thing — both
   produced the identical 41472 words. The binaries are not in this directory; the words live in
   `work/route_a/out/*.u64` (truncated to words-written, which is correct) and
   `work/route_c/out/*.words` (same words plus all-zero block padding; verified here that C's file
   is A's file followed by zeros, for all five binaries). There is no lossless *byte* rendering of
   a PDP-10 save file; any 8-bit spelling is a convention.
6. The working copy `work/t10.dsk` has been booted and written (logins, a scratch compile, deletes).
   The pristine distribution under `D:\SynologyDrive\eXo\emulators\TOPS-10\` was never written.
   The ten distribution files themselves are untouched.
