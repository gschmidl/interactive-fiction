#!/usr/bin/env python
"""Disassemble a FisK core image.

    dis.py <fisk.dmp> <octal-address> [count]

Addresses are the ones the program itself uses: 0o74..0o156673 is the low
segment, 0o400000 upward is the high segment.  The right-hand columns show
the word as SIXBIT and as five 7-bit characters; the game's message database
sits in the low segment obfuscated with XOR 0o64, so use -x to see it.
"""
import sys
import pdp10dis as D

LOWWORDS = 56704
LOWBASE  = 0o74
HIBASE   = 0o400000


def load(path):
    w = [int(l, 8) for l in open(path) if l.strip()]
    m = {}
    for i, x in enumerate(w[:LOWWORDS]):
        m[LOWBASE + i] = x
    for i, x in enumerate(w[LOWWORDS:]):
        m[HIBASE + i] = x
    return m


def pr(s):
    return "".join(c if 32 <= ord(c) < 127 else "." for c in s)


def main():
    args = [a for a in sys.argv[1:] if a != "-x"]
    xor = 0x34 if "-x" in sys.argv[1:] else 0
    if len(args) < 2:
        sys.exit(__doc__)
    m = load(args[0])
    start = int(args[1], 8)
    n = int(args[2]) if len(args) > 2 else 40
    for a in range(start, start + n):
        x = m.get(a)
        if x is None:
            print("%06o/ ---" % a)
            continue
        s7 = "".join(chr(((x >> (29 - 7 * i)) & 0x7f) ^ xor) for i in range(5))
        print("%06o/ %012o  %-26s |%s| |%s|"
              % (a, x, D.dis(x, a), pr(D.s6(x)), pr(s7)))


main()
