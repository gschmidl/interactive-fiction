#!/usr/bin/env python3
"""Disassemble the repaired EXPLOR core image.  usage: peek.py <octal addr> [n]"""
import importlib.util, os, sys
H = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, H)
from img import MEM, txt
dt = importlib.util.module_from_spec(
    importlib.util.spec_from_file_location("dt", os.path.join(H, "dis_tab.py")))
dt.__spec__.loader.exec_module(dt)
def dump(lo, n=24):
    for a in range(lo, lo + n):
        x = MEM.get(a, 0)
        print(" %06o: %012o  %-24s %r" % (a, x, dt.dis(x), txt(x)))
if __name__ == '__main__':
    dump(int(sys.argv[1], 8), int(sys.argv[2]) if len(sys.argv) > 2 else 24)
