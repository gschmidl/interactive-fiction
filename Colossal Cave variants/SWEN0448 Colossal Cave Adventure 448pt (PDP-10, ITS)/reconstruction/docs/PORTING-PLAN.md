# Porting ADV448 — approach

## What was recovered

From `GAMES;` on the ITS pack, byte-for-byte after terminal reconstruction:

| file | lines | what |
|---|---|---|
| `ADV4MA.F4` | 2576 | main program |
| `ADV4SU.F4` | 970 | 32 subroutines/functions |
| `FT01.DAT` | 2212 | database, 13 sections |

Validated: every line of both sources satisfies fixed-form column rules
(label 1-5, continuation 6, statement 7-72) with zero violations, and
every FT01.DAT record parses against the FORMAT statements the program
itself uses (`I7` headers, `1I8,18A4`, `11I7`, `I7,A5`) with zero
anomalies across all 13 sections.

## The problem

This is DEC FORTRAN-10 for a 36-bit machine. Text is not held in
`CHARACTER` variables; it is packed **five seven-bit characters per
36-bit word** and manipulated with bit arithmetic. The vocabulary is
built by `VOCAB('GRATE',1)` — a 5-character literal used as an integer.
Masks such as `"774000000000` select 7-bit fields; `SHIFT(A(J),7*(K-1))`
steps a character at a time; `'@@@@@'` (o'100' repeated) folds case.

## The decision: emulate the packing, do not modernise the logic

The obvious route — the one taken for the Univac Crystal Caves port — is
to convert packed characters into real `CHARACTER` variables. That is a
large rewrite of working game logic and puts fidelity at risk.

There is a much smaller and more faithful option. **Keep 7-bit-per-
character packing, in `INTEGER*8`.** Then every mask, every `SHIFT`,
every `.AND.`/`.XOR.` in the game logic keeps working exactly as written,
because the bit layout is unchanged — a 36-bit value simply sits in a
64-bit register.

The only thing that genuinely breaks is the compiler's own `A` edit
descriptor, since gfortran packs 8-bit characters. So the work is
confined to I/O:

1. `-fdefault-integer-8`, so a word holds the 36-bit value.
2. Replace every `A`-format read/write (~30 sites) with helpers that
   pack/unpack DEC FORTRAN-10 style: n characters left-justified in
   7-bit fields, blank-filled to 5.
3. Convert dialect: `"nnn` octal to `o'nnn'`, `TYPE n` to `PRINT n`,
   drop the `^L` page separators, `ACCESS='SEQIN'`.
4. Replace the genuinely machine-specific routines: `SHIFT` (36-bit
   arithmetic/logical shift), `DATIME`, `RAN`.

Game logic in `ADV4MA.F4` should need almost no change, which is the
point: what runs is the Brown University program, not a paraphrase.

## Status

Extraction complete and validated. Conversion not yet done.
