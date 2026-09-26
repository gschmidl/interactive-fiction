#!/usr/bin/env python3
"""convert.py -- generate src/adv.f and src/iofil.f from the recovered
FORTRAN-10 sources in ../src_original.

Nothing is edited by hand.  Every line of the generated source is either
the original line with one of the mechanical rules below applied, or an
exact-line replacement from patches.py.  A patch that no longer matches
the line it quotes fails the build, so the two cannot drift apart.

The mechanical rules
--------------------
LAYOUT  DEC tab source form to fixed-form columns.  FORTRAN-10 read a
        leading TAB as "statement starts in column 7", and a TAB
        followed by a digit 1-9 as a continuation line.  No line of
        either source runs past column 70, so nothing the -10 ignored
        past column 72 becomes visible here.

OCTAL   "nnn -> the decimal value of those 36 bits, sign-extended.  Bit
        35 is the sign on a PDP-10, so "400000000000 is negative, and
        GETIN's masks (-M2(K), "774000000000) only work on operands that
        carry the sign the same way.

CHAR    'ABCDE' -> the decimal value of the 36-bit word holding those
        five seven-bit characters, blank-filled and sign-extended.  Not
        applied inside FORMAT or PAUSE statements.

REAL    a real constant after a relational operator -> the exact value
        the FORTRAN-10 compiler assembled for it, which is the fraction
        *truncated* to 27 bits, written as a double constant.  Every one
        of the thirteen is compared against RAN, and the checked table
        below is the bit pattern the compiler on the pack produced.

RAN     RAN( -> RAN10(, so that gfortran cannot substitute its own
        intrinsic of that name.

PAUSE   PAUSE 'text' -> CALL PAUSEM('text'), which types the operator
        dialogue FOROTS typed and reads the reply.
"""

import os
import re
import sys
from decimal import Decimal, getcontext

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
ORIG = os.path.join(os.path.dirname(PORT), "src_original")
SRC = os.path.join(PORT, "src")

sys.path.insert(0, HERE)
import patches

getcontext().prec = 60

# The real constants as the FORTRAN-10 compiler on the pack assembled
# them, read back with EQUIVALENCE and printed in octal.  Used to check
# the folding below, not to do it.
MEASURED = {
    "0.05": 0o174631463146,
    "0.1": 0o175631463146,
    "0.2": 0o176631463146,
    "0.25": 0o177400000000,
    "0.4": 0o177631463146,
    "0.5": 0o200400000000,
    "0.8": 0o200631463146,
}


def packword(s):
    """'ABCDE' as a sign-extended 36-bit word of five seven-bit chars."""
    if len(s) > 5:
        raise SystemExit("literal longer than one word: %r" % s)
    s = (s + "     ")[:5]
    w = 0
    for k, ch in enumerate(s):
        w += ord(ch) << (29 - 7 * k)
    if w & (1 << 35):
        w -= 1 << 36
    return w


def octword(digits):
    """"nnn as a sign-extended 36-bit word."""
    v = int(digits, 8)
    if v >= 1 << 36:
        raise SystemExit("octal literal wider than a word: %s" % digits)
    if v & (1 << 35):
        v -= 1 << 36
    return v


def p10single(txt):
    """The word a FORTRAN-10 real constant assembles to, and its value.

    PDP-10 single precision: sign, eight-bit excess-128 exponent, 27-bit
    fraction normalised into [0.5,1).  The compiler truncates.
    """
    from fractions import Fraction
    v = Fraction(txt)
    if v == 0:
        return 0, Decimal(0)
    e = 128
    while v < Fraction(1, 2):
        v *= 2
        e -= 1
    while v >= 1:
        v /= 2
        e += 1
    f = (v.numerator << 27) // v.denominator
    word = (e << 27) | f
    val = Decimal(f) * (Decimal(2) ** (e - 155))
    return word, val


def dconst(txt):
    word, val = p10single(txt)
    if txt in MEASURED and MEASURED[txt] != word:
        raise SystemExit("constant %s folds to %o, the -10 assembled %o"
                         % (txt, word, MEASURED[txt]))
    s = format(val, "f")
    if "." not in s:
        s += ".0"
    return s + "D0"


def layout(line):
    """DEC tab source form -> fixed-form columns."""
    if line == "":
        return "", "blank"
    if line[0] in "Cc*":
        return line, "comment"
    m = re.match(r"^(\d+)\t(.*)$", line)
    if m:
        lab, rest = m.groups()
        if len(lab) > 5:
            raise SystemExit("label wider than five columns: %r" % line)
        return lab.ljust(5) + " " + rest, "layout"
    m = re.match(r"^\t([1-9])(.*)$", line)
    if m:
        return "     " + m.group(1) + m.group(2), "layout"
    if line.startswith("\t"):
        return "      " + line[1:], "layout"
    raise SystemExit("line starts with neither TAB, label+TAB nor C: %r" % line)


RELREAL = re.compile(r"(\.(?:GT|LT|GE|LE|EQ|NE)\.)([0-9]+\.[0-9]+)")
CHARLIT = re.compile(r"'([^']*)'")
OCTLIT = re.compile(r'"([0-7]+)')


def rules(stmt):
    """Apply the mechanical rules to one statement (columns 7 on)."""
    why = []
    up = stmt.upper()

    if re.match(r"\s*PAUSE\s+'", stmt):
        stmt = re.sub(r"(\s*)PAUSE(\s+)('[^']*')", r"\1CALL PAUSEM(\3)", stmt)
        return stmt, ["PAUSE"]

    isfmt = "FORMAT" in up

    if not isfmt:
        n = len(OCTLIT.findall(stmt))
        if n:
            stmt = OCTLIT.sub(lambda m: str(octword(m.group(1))), stmt)
            why.append("octal")
        n = len(CHARLIT.findall(stmt))
        if n:
            stmt = CHARLIT.sub(lambda m: str(packword(m.group(1))), stmt)
            why.append("literal")

    if RELREAL.search(stmt):
        stmt = RELREAL.sub(lambda m: m.group(1) + dconst(m.group(2)), stmt)
        why.append("constant")

    if re.search(r"\bRAN\s*\(", stmt):
        stmt = re.sub(r"\bRAN(\s*\()", r"RAN10\1", stmt)
        why.append("RAN10")

    return stmt, why


def convert(name, table, out):
    text = open(os.path.join(ORIG, name), encoding="latin-1").read()
    lines = text.replace("\r", "").split("\n")
    while lines and lines[-1] == "":
        lines.pop()

    pat = {}
    for orig, repl, note in table:
        pat.setdefault(orig, []).append((repl, note))
    used = {k: 0 for k in pat}

    outl = []
    acct = []
    for ln in lines:
        if ln in pat:
            repl, note = pat[ln][0]
            used[ln] += 1
            for r in repl:
                outl.append(r)
            acct.append((ln, list(repl), "patch"))
            continue
        fixed, why = layout(ln)
        if why in ("blank", "comment"):
            outl.append(fixed)
            acct.append((ln, [fixed], why))
            continue
        head, stmt = fixed[:6], fixed[6:]
        stmt, w = rules(stmt)
        new = head + stmt
        outl.append(new)
        acct.append((ln, [new], "+".join(["layout"] + w)))

    missing = [k for k, v in used.items() if v == 0]
    if missing:
        sys.stderr.write("convert.py: these patches no longer match "
                         "%s:\n" % name)
        for m in missing:
            sys.stderr.write("    %r\n" % m)
        raise SystemExit(1)

    banner = [
        "C  %s -- GENERATED by tools/convert.py from" % os.path.basename(out),
        "C  ../src_original/%s.  Do not edit: every change belongs in" % name,
        "C  tools/convert.py or tools/patches.py.  Run",
        "C      python tools/showdiff.py -a",
        "C  for a line-by-line account of every difference from the",
        "C  original.",
    ]
    with open(out, "w", newline="\n") as f:
        f.write("\n".join(banner + outl) + "\n")
    return acct


def main():
    os.makedirs(SRC, exist_ok=True)
    a = convert("ADV.F4", patches.ADV, os.path.join(SRC, "adv.f"))
    b = convert("IOFIL.FOR", patches.IOFIL, os.path.join(SRC, "iofil.f"))
    nin = len(a) + len(b)
    nout = sum(len(x[1]) for x in a) + sum(len(x[1]) for x in b)
    print("convert.py: %d lines in, %d out (src/adv.f, src/iofil.f)"
          % (nin, nout))


if __name__ == "__main__":
    main()
