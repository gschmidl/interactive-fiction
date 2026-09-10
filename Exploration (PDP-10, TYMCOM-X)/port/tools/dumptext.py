#!/usr/bin/env python3
"""Dump printable runs from the repaired EXPLOR core image, with addresses.
usage: dumptext.py [substring]"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from img import MEM, a5
def printable(x): return all(32 <= ord(c) < 127 for c in a5(x))
def runs(minw=4):
    out, cur, st = [], [], None
    for a in sorted(MEM):
        if printable(MEM[a]):
            if not cur: st = a
            cur.append(a5(MEM[a]))
        else:
            if len(cur) >= minw: out.append((st, ''.join(cur)))
            cur = []
    if len(cur) >= minw: out.append((st, ''.join(cur)))
    return out
if __name__ == '__main__':
    pat = sys.argv[1].upper() if len(sys.argv) > 1 else None
    for a, t in runs():
        if pat is None or pat in t.upper():
            print("%06o  %s" % (a, t))
