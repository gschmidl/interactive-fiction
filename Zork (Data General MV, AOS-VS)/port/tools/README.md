# Tools

Three small Python programs that were what actually made the 32-bit work
possible.  They read the AOS/VS `:UTIL` files that came with the Thissala
export.  `st.py` looks for them in the sibling Thissala project,
`../../../Thissala (Data General Eclipse, AOS-VS)/src_original`; set
`THISSALA_SRC` to point somewhere else.

## `st.py` — read a `.PR` and its `.ST`

`load_pr(path)` applies the loading rule from `../NOTES.md` and returns the
memory image as a list of words.  `read_st(path)` parses a linker symbol
table: 20-byte records of

    [flags][name length][value: 4][FFFF][12 zero bytes][name][pad to even]

Almost every program in `:UTIL` shipped with its `.ST`, and the values are
ring-qualified addresses in exactly the range the games use.

## `mvdis.py` — disassemble

    python mvdis.py SCOM 7F0B2 7F0F0      # a utility, with its symbols
    python mvdis.py path/to/ZORK.PR 75530 75600

Decodes with the same table and masks as `../src32/mvops.h`, so it is a
faithful mirror of what the emulator will do — including the mistakes, which
is the point: when the emulator stops on something, disassembling the same
address here shows the surrounding code in the same terms.

## `symmatch.py` — name a stripped program's routines

    python symmatch.py "path/to/FERRET.PR" > ferret.sym

Neither game shipped a symbol table, but both are linked against the same
AOS/VS runtime as the utilities, which did.  For every symbol in every donor
`.ST` this takes the twelve words at that address and looks for the same
twelve words in the target; a unique hit names the routine.  It found 144 in
FERRET.PR and 54 in ZORK.PR, which is how the runtime in both turned out to
be PL/I — `P?PUT`, `P?GET_LINE`, `I.START`, `O.SIGNAL`, `X.DC` — and how
`I.DISPLA`, `I?SALLOC` and the rest could be read as what they are rather
than as anonymous addresses.

`mvdis.py` reads a `.sym` file back if you pass it through the `syms`
argument of `dis()`; see the calls at the bottom of the file.
