# ADVENTURE 448 — a Windows port

The **448-point Adventure**: Crowther and Woods by way of Brown
University, ported to MIT's ITS. The source says so itself:

```
C  Modified for the Brown University system, April 1978
C     Dave Wallace '78
C     Dave Nebiker '79
C     Eric Albert  '80
C     LES  WU      '82 (ALSO MADE CONVERSIONS FOR A NBS-10)
C  Modified for the ITS system, July 1979 by EJS@MC
```

Unlike the other PDP-10 games in this collection, this one did not have
to be emulated. The **FORTRAN source survived on the ITS pack**, so what
builds here is the original program, compiled — not a binary running
under a shim, and not a rewrite.

## Build

Needs gfortran and Python 3.

```
make
```

produces `bin\adv448.exe`. It reads `FT01.DAT` from the working
directory; `make` copies it into `bin\`.

## Play

```
bin\adv448.exe
```

It asks whether you are a wizard (say no), whether this is a restarted
game, and whether you want instructions. See **Differences** below for
why the ITS original did not ask the first of those.

## How the 36-bit machine was handled

This is DEC FORTRAN-10 for a 36-bit word. Text is not in `CHARACTER`
variables; it is packed **five seven-bit characters per word** and worked
on with bit arithmetic — `VOCAB('GRATE',1)` passes a five-character
literal as an integer, `"774000000000` selects the first character,
`SHIFT(A(J),7*(K-1))` steps along one character at a time.

The obvious port converts all of that to `CHARACTER` variables. This one
does not. **It keeps the PDP-10 layout**, in an `INTEGER*8`: character k
in bits 36-7k..30-7k, which is exactly what the program's own
`DATA MASKS/"4000000000,"20000000,"100000,"400,"2,0/` describes — the low
bit of each of the five fields, at 29, 22, 15, 8 and 1. Because the bit
layout is unchanged, every mask, shift and `.AND.`/`.XOR.` in the game
logic keeps working as written, and `adv4ma.f` — the whole game — needed
no logic changes at all.

Words are held **sign-extended**, because bit 35 is the sign on a -10 and
the program tests it: `A5TOA1` puts the blank between its second and
third words only `IF(C.LT.0)`, which is true exactly when the word's
first character is `100` octal or above. Without sign extension the game
says `I SEE NO LAMPHERE.`

What the compiler genuinely cannot do is the `A` edit descriptor over
that layout, since gfortran packs eight-bit bytes. So the ~20 A-format
reads and writes go through `C2W`/`WSTR` in `src/runtime.f`, which
reproduce FORTRAN-10's rule: n characters left-justified in the word,
blank-filled to five. Four routines were machine-specific and are
replaced there too — `SHIFT`, `RAN`, `DATIME`, and `GETIN`.

Everything else is mechanical and done by `tools/convert.py`:
`"nnn` octal to decimal, `TYPE n` to `PRINT n`, `'ABCDE'` to its packed
constant, writes to unit 5 (the -10's terminal) redirected to unit 6.
`tools/patches.py` holds the I/O replacements, each one listed.

## Two changes to the program, and why

**`DO 1037 L=1,20` became `DO 1037 L=1,9`.** `TK1` is `DIMENSION 9` and
the record is read with `FORMAT(11I7)`, which fills exactly nine — the
loop ran off the end of the array. It survived on the -10 only because
the word after `TK1` happened to be zero. The author's own sibling loop
over the same record shape (labels 1070/1071) uses `1,9`, and exactly one
record in section 3 fills all nine fields — the one that trips it. A
location's travel options simply continue on the next record.

**`-finit-local-zero`.** FORTRAN-10's loader zeroed core and the program
relies on it: `PLAC` and `FIXD` are only assigned for objects listed in
section 7, and the setup loop then tests `PLAC(K).NE.0` for all 100.

## Differences from the ITS original, and why

The port was checked against the real thing by running the same commands
against `GAMES;TS ADV448` under SIMH and diffing the transcripts. A
15-move session came out **identical**, line for line. A longer session
matched 21 of 23 lines, and the two that differed were a swap: the long
room description where the original gave the short one, and vice versa.

That is the abbreviation counter, and it is a property of the ITS binary
rather than a fault here. `ABB` is zeroed at one place only — inside the
database-setup block that `IF(SETUP.EQ.2)GOTO 1` skips — and `START`
never touches it. `TS ADV448` is a *saved core image* with the database
already loaded, so it begins every game carrying whatever `ABB` counters
were left in core when it was built. This port runs the setup and zeroes
them, as the source does.

The same explains the startup: the ITS image goes straight to "Is this a
restarted game?", while this port first reads `FT01.DAT`, reports what it
loaded, and calls `MAINT` — which is what the program does on a first
run, before anyone saved an image of it.
