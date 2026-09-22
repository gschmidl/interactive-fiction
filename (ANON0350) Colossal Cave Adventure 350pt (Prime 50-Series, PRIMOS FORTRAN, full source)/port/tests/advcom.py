#!/usr/bin/env python3
"""Convert the Prime's own ADVCOM - the initialised COMMON blocks the game
starts from - into the port's advcom.dat.

ADVCOM is what SVCOMN wrote: the eleven ranges of COMMON the main program
names with ADDR and SIZE, one after the other.  Two things differ from the
port's memory:

  * the Prime is big-endian;
  * compiled -INTL its INTEGER is four bytes but its LOGICAL is still two,
    packed with no padding.  That is exactly why the file is eighteen words
    shorter than the port's eleven ranges: one halfword for BLKLIN, twenty
    for HINTED, six for DSEEN and nine for the nine LOGICAL scalars of
    /MSCCOM/ - 36 halfwords, and the script checks that it comes out right.

So the conversion is field by field.  The field list is read out of the
source: the COMMON, DIMENSION and LOGICAL statements of the main program say
what is in each block and in what order, and the ADDR/SIZE pairs say where
each of the eleven ranges begins and ends.  Nothing is hand-copied.

    python advcom.py ADVCOM advcom.dat
"""
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, '..', '..', 'src_original', 'ADVENTURE.UFD',
                   'ADVENTURE.FTN.txt')


def statements(path):
    """The main program's declarations, continuations joined."""
    out = []
    for line in open(path, encoding='latin-1'):
        line = line.rstrip()[:72]
        if not line or line[:1] in ('C', 'c', '*', '$'):
            continue
        if len(line) > 5 and line[5] not in (' ', '0') and out:
            out[-1] += line[6:].strip()
        else:
            out.append(line[6:].strip())
        if out[-1].startswith('CALL LDCOMN'):
            break
    return out


def declarations():
    """(order, dims, logicals) - the COMMON blocks in declaration order, the
    array sizes, and which names are LOGICAL."""
    order, dims, logical = [], {}, set()
    for s in statements(SRC):
        m = re.match(r'COMMON\s*/([A-Z0-9]+)/\s*(.*)', s)
        if m:
            order.append((m.group(1), [x.strip() for x in
                                       m.group(2).split(',') if x.strip()]))
            continue
        m = re.match(r'DIMENSION\s+(.*)', s)
        if m:
            for d in re.findall(r'([A-Z0-9]+)\(([0-9, ]+)\)', m.group(1)):
                n = 1
                for x in d[1].split(','):
                    n *= int(x)
                dims[d[0]] = n
            continue
        m = re.match(r'LOGICAL\s+(.*)', s)
        if m:
            logical.update(x.strip() for x in m.group(1).split(',')
                           if x.strip())
    return order, dims, logical


def ranges():
    """The eleven (first, last) pairs, from the ADDR and SIZE calls."""
    first, last, out = None, None, []
    for s in statements(SRC):
        m = re.match(r'CALL ADDR\(([A-Z0-9]+)(?:\(\d+\))?,CMADRS', s)
        if m:
            first = m.group(1)
        m = re.match(r'CMSZES\(\d+\)=SIZE\([A-Z0-9]+(?:\(\d+\))?,'
                     r'([A-Z0-9]+)(?:\((\d+)\))?\)', s)
        if m and first:
            out.append((first, m.group(1), int(m.group(2) or 0)))
            first = None
    return out


def fields():
    """The eleven ranges as lists of (name, words, is_logical)."""
    order, dims, logical = declarations()
    flat = []
    for block, names in order:
        for n in names:
            flat.append(n)
    out = []
    for first, last, upto in ranges():
        i, j = flat.index(first), flat.index(last)
        got = []
        for n in flat[i:j + 1]:
            size = dims.get(n, 1)
            if n == last and upto:
                size = upto        # SIZE(MTEXT(1),MTEXT(34)) and friends
            got.append((n, size, n in logical))
        out.append(got)
    if len(out) != 11:
        sys.exit('advcom: found %d ranges, expected 11' % len(out))
    return out


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    blocks = fields()
    prime = sum(sum(s * (1 if lg else 2) for _, s, lg in b) for b in blocks)
    port = sum(sum(s for _, s, _ in b) for b in blocks)
    data = open(sys.argv[1], 'rb').read()
    if len(data) != prime * 2:
        sys.exit('advcom: %s is %d bytes, the field list says %d'
                 % (sys.argv[1], len(data), prime * 2))
    if port * 2 - prime != 36:
        sys.exit('advcom: the layouts differ by %d halfwords, expected 36'
                 % (port * 2 - prime))

    out, at = bytearray(), 0
    for b in blocks:
        for name, size, lg in b:
            for _ in range(size):
                if lg:
                    v, = struct.unpack_from('>h', data, at)
                    at += 2
                    v = 1 if v else 0
                else:
                    v, = struct.unpack_from('>i', data, at)
                    at += 4
                out += struct.pack('<i', v)
    if at != len(data):
        sys.exit('advcom: used %d of %d bytes' % (at, len(data)))
    open(sys.argv[2], 'wb').write(out)
    print('advcom: %d Prime halfwords -> %d port words in %s'
          % (prime, port, os.path.basename(sys.argv[2])))


if __name__ == '__main__':
    main()
