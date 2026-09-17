"""Compare the program structure of ADVENTURE 6.1/9 with 6.1/3.

6.1/3 carries LINK's whole symbol table, so its modules and their bounds
are exact.  6.1/9 carries none, so its routines are found the way a
FORTRAN-10 traceback finds them: by the SIXBIT name compiled into the word
in front of each entry point.  That rule was checked against 6.1/3, where
it finds every one of the 119 game modules.

    python modules.py        writes ../modules.txt
"""
import os, re
from pdp10 import *

OUT = os.path.join(os.path.dirname(HERE), "modules.txt")

# Instructions a FORTRAN-10 (or hand-coded MACRO) entry point starts with.
ENTRY_OPS = {0o200, 0o201, 0o202, 0o205, 0o210, 0o254, 0o260, 0o261, 0o265, 0o304,
             0o400, 0o402, 0o403, 0o474, 0o476, 0o500, 0o504, 0o505, 0o541, 0o550,
             0o551, 0o552}
NAME = re.compile(r"^[A-Z][A-Z0-9.%$]{0,5} *$")
# Words in 6.1/3's INITLZ tables that happen to read as SIXBIT names.
PLAUSIBLE = re.compile(r"^[A-Z][A-Z0-9]*[.%$]?$")


def entries_by_header(mem, lo, hi):
    out = []
    for a in range(lo, hi):
        x = mem.get(a, 0)
        if not x or not NAME.match(six(x)):
            continue
        nxt = mem.get(a + 1, 0)
        if (nxt >> 27) not in ENTRY_OPS:
            continue
        name = six(x).strip()
        if not PLAUSIBLE.match(name):
            continue
        out.append((a + 1, name))
    return out


# Entry points without the FORTRAN-10 prologue, each looked at by hand:
# FOO is the module of wizard stubs, ASTACK/RSTACK and PAREOL are
# small routines that never save AC16, and PUNCT, INKEEP and I2 are ENTRY
# statements that 6.1/3's symbol table also lists.
REVIEWED = {"FOO", "ASTACK", "RSTACK", "PAREOL", "PUNCT", "INKEEP", "I2"}


def real_entries(mem, lo, hi, known):
    """Keep a candidate when it starts with the prologue every compiled
    subprogram has -- MOVEM 16,x with x just below the entry, where the
    argument pointer is saved -- or when 6.1/3 knows the name, or when it is
    one of the hand-checked entries above.  What that throws away are call
    sites (MOVEI 16,args / PUSHJ) and data that happens to read as SIXBIT."""
    out = []
    for a, n in sorted(set(entries_by_header(mem, lo, hi))):
        x = mem.get(a, 0)
        prologue = (x >> 23) == (0o202 << 4 | 0o16) and a - 0o1000 < (x & 0o777777) < a
        if prologue or n in known or n in REVIEWED:
            out.append((a, n))
    return out


def main():
    m3, s3, _ = build("6.1/3")
    syms = symbols(m3)
    glob = {n: v & 0o777777 for k, n, v in syms if k == 1}
    mods3 = sorted((v & 0o777777, n) for k, n, v in syms if k == 0 and n != "JOBDAT")
    lib3 = glob["RESET."]                       # FORLIB starts here
    size3 = {}
    for i, (a, n) in enumerate(mods3):
        if a >= lib3:
            break
        size3[n] = mods3[i + 1][0] - a

    m9, s9, _ = build("6.1/9")
    # 6.1/9's FORLIB starts where FORINI (JSP 16,) lives: the start code's first JSP target.
    lib9 = m9[s9 + 1] & 0o777777
    known = set(glob) | set(n for _, n in mods3)
    e9 = real_entries(m9, 0o140, lib9, known)
    size9 = {}
    for i, (a, n) in enumerate(e9):
        nxt = e9[i + 1][0] if i + 1 < len(e9) else lib9
        size9.setdefault(n, []).append((a, nxt - a))

    names3 = set(size3)
    names9 = set(size9)
    lines = []
    w = lines.append
    w("ADVENTURE 6.1/9 (new-adventure.exe, 13-Feb-81) against 6.1/3 (ADVENTURE.EXE, the 751 port)")
    w("")
    w("6.1/3: %d game modules from LINK's symbol table, library from %06o." % (len(size3), lib3))
    w("6.1/9: %d named entry points from SIXBIT name headers, library from %06o." % (len(e9), lib9))
    w("Sizes are words from one entry to the next, so they include the")
    w("module's formats, literals and argument blocks.  6.1/3's MAIN. is")
    w("unnamed in the code; 6.1/9's main program starts at %06o." % s9)
    w("")
    w("%-8s %8s %7s    %8s %7s" % ("routine", "6.1/9 at", "words", "6.1/3 at", "words"))
    for n in sorted(names3 | names9, key=lambda n: (min(a for a, _ in size9[n]) if n in size9 else 10**7, n)):
        a9 = " ".join("%06o" % a for a, _ in size9.get(n, []))
        z9 = " ".join("%d" % z for _, z in size9.get(n, []))
        a3 = "%06o" % dict((b, a) for a, b in mods3).get(n, 0) if n in size3 else ""
        z3 = "%d" % size3[n] if n in size3 else ""
        w("%-8s %8s %7s    %8s %7s" % (n, a9 or "-", z9 or "", a3 or "-", z3))
    w("")
    w("Only in 6.1/9: " + " ".join(sorted(names9 - names3)))
    w("")
    w("Only in 6.1/3: " + " ".join(sorted(names3 - names9)))
    w("")
    w("In both:       " + " ".join(sorted(names3 & names9)))
    open(OUT, "w", newline="\n").write("\n".join(lines) + "\n")
    print("\n".join(lines[-6:]))
    print("wrote", OUT)


if __name__ == "__main__":
    main()
