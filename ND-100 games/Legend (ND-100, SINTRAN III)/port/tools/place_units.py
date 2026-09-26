"""Find where each intact BASLIBR-H00 unit was loaded in a linked program
image, by its literal and relocated words."""
import os
import struct
import sys
from brf import parse, units, layout

WORK = os.environ.get('ND100_WORK',
    os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', '_ND100_work'))
LIB = os.path.join(WORK, 'files', 'basic', 'BASLIBR-H00.BRF')


def image(path):
    d = open(path, 'rb').read()
    w = struct.unpack('>%dH' % (len(d) // 2), d)
    first, last = w[2], w[3]
    mem = {}
    for k in range(last - first + 1):
        mem[first + k] = w[0x100 + k]
    return mem, w[:7]


def lib_units():
    b = open(LIB, 'rb').read()
    r1, _ = parse(b, 64, 2076)
    r2, _ = parse(b, 2818)
    return units(r1) + units(r2)


def place(mem, unit):
    words, entries, lbr = layout(unit)
    lits = sorted((rel, v) for rel, (k, v) in words.items() if k == 'LF' and v not in (0, 0o177777))
    if len(lits) < 4:
        return None
    r0, v0 = lits[0]
    hits = []
    for addr, val in mem.items():
        if val != v0:
            continue
        pb = addr - r0
        ok = True
        for rel, (k, v) in words.items():
            got = mem.get(pb + rel)
            if k == 'LF' and got != v or k == 'LR' and got != (v + pb) & 0xffff:
                ok = False
                break
        if ok:
            hits.append(pb)
    return hits


if __name__ == '__main__':
    mem, hdr = image(sys.argv[1])
    print('header', [oct(x) for x in hdr])
    rows = []
    for u in lib_units():
        words, entries, lbr = layout(u)
        hits = place(mem, u)
        if hits:
            rows.append((hits[0], lbr, entries, max(words) if words else 0, len(hits)))
    for pb, lbr, entries, size, n in sorted(rows):
        print('%06o-%06o %-30s %s%s' % (pb, pb + size, ','.join(lbr), ' '.join('%s=%06o' % (e, pb + r) for e, r in entries.items()), '' if n == 1 else ' (%d places)' % n))
