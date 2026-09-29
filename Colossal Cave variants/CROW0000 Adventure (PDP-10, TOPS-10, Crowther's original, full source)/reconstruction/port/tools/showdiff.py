#!/usr/bin/env python3
"""showdiff.py -- account for every difference between the recovered
FORTRAN-10 sources and the gfortran sources the build compiles.

    python tools/showdiff.py         a summary table
    python tools/showdiff.py -a      every changed line, with its reason
    python tools/showdiff.py -p      just the patches and their reasons

Each changed line is re-derived here from the original by the same rules
convert.py applies.  A line that cannot be re-derived is reported as
UNEXPLAINED; there should be none, and the exit status says so.
"""

import argparse
import collections
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import convert
import patches

REASONS = {
    "patch": "exact-line patch from patches.py",
    "layout": "DEC tab source form to fixed-form columns",
    "layout+octal": 'octal "nnn to the decimal value of those 36 bits',
    "layout+literal": "packed character literal to its word value",
    "layout+octal+literal": "octal literal and packed character literal",
    "layout+constant": "real constant to the value FORTRAN-10 assembled",
    "layout+constant+RAN10": "real constant folded, and RAN renamed RAN10",
    "layout+RAN10": "RAN renamed RAN10, which gfortran has as an intrinsic",
    "layout+literal+RAN10": "packed literal, and RAN renamed RAN10",
    "layout+PAUSE": "PAUSE 'text' to CALL PAUSEM, which types the operator"
                    " dialogue FOROTS typed and reads the reply",
    "comment": "comment, unchanged",
    "blank": "blank line, unchanged",
}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-a", "--all", action="store_true")
    ap.add_argument("-p", "--patches", action="store_true")
    args = ap.parse_args()

    if args.patches:
        for name, table in (("ADV.F4", patches.ADV),
                            ("IOFIL.FOR", patches.IOFIL)):
            print("=== %s ===" % name)
            for orig, repl, note in table:
                print("  %s" % orig.replace("\t", "<TAB>"))
                for r in repl:
                    print("      -> %s" % r)
                if not repl:
                    print("      -> (deleted)")
                print("      %s" % note)
                print()
        return 0

    acct = []
    for name, table, out in (("ADV.F4", patches.ADV, "adv.f"),
                             ("IOFIL.FOR", patches.IOFIL, "iofil.f")):
        acct += [(name,) + t for t in
                 convert.convert(name, table, os.path.join(convert.SRC, out))]

    counts = collections.Counter()
    nin = nout = 0
    unexplained = 0
    for src, orig, repl, why in acct:
        nin += 1
        nout += len(repl)
        changed = not (len(repl) == 1 and repl[0] == orig)
        if why in ("comment", "blank"):
            continue
        if why == "layout" and not changed:
            continue
        if why == "layout":
            counts["layout"] += 1
        else:
            counts[why] += 1
        if why not in REASONS:
            unexplained += 1
        if args.all and why != "layout":
            print("%-10s %s" % (src, orig.replace("\t", "<TAB>")))
            for r in repl:
                print("           -> %s" % r)
            if not repl:
                print("           -> (deleted)")
            print("           [%s] %s" % (why, REASONS.get(why, "UNEXPLAINED")))
            print()

    print("%d lines in, %d out" % (nin, nout))
    print()
    print("%6s  %s" % ("lines", "why"))
    for why, n in counts.most_common():
        print("%6d  %s" % (n, REASONS.get(why, "UNEXPLAINED: " + why)))
    print()
    if unexplained:
        print("%d UNEXPLAINED" % unexplained)
        return 1
    print("nothing unaccounted for")
    return 0


if __name__ == "__main__":
    sys.exit(main())
