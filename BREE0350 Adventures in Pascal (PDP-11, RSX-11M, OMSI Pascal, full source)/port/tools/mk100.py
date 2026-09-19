#!/usr/bin/env python3
"""mk100.py - rebuild ADVENTURE.100, the ASCII source of the VT100 database.

The DECUS RSX82B tape (and so ibiblio) carries ADVENTURE.100 as a byte for byte
copy of ADVENTURE.DAT: the VT100 text itself survives only in the binary
ADVTXT.100 that 100FLS made from the real file.  This undoes 100FLS:

  sections 1-6  from ADVTXT.100, split where ADVDAT.100's pointer arrays
                (LTEXT, STEXT, PTEXT, RTEXT, CTEXT, MTEXT) say each one starts
  sections 7-12 from ADVENTURE.DAT - the readme says they are the same in both
                databases, and 100FLS does not read them anyway

The proof is the round trip: 100fls.exe run on the result must give back
ADVTXT.100 and ADVDAT.100 (test/check-data.sh does that).

usage: mk100.py ADVTXT.100 ADVDAT.100 ADVENTURE.DAT > ADVENTURE.100
"""
import struct, sys

LOCSIZ, MAXTRS = 140, 64

def records(path, size):
    d = open(path, 'rb').read()
    rpb = 512 // size
    return [d[b + k * size: b + (k + 1) * size] for b in range(0, len(d), 512) for k in range(rpb)]

def main():
    txt, dat, asc = sys.argv[1:4]
    recs = [(struct.unpack('<h', r[:2])[0], r[2:]) for r in records(txt, 74)]
    ptr = [struct.unpack('<h', r)[0] for r in records(dat, 2)]
    ltext, stext = ptr[0:LOCSIZ], ptr[LOCSIZ:2 * LOCSIZ]
    o = 2 * LOCSIZ
    ptext = ptr[o:o + MAXTRS]; o += MAXTRS
    rtext = ptr[o:o + 300]; o += 300
    mtext = ptr[o:o + 100]; o += 100
    ctext = ptr[o]
    first = lambda a: min(x for x in a if x)
    starts = [first(ltext), first(stext), first(ptext), first(rtext), ctext, first(mtext)]
    assert starts == sorted(starts) and starts[0] == 1, starts
    # the last message runs while its number repeats; what follows in the
    # final block is whatever the disk block held before
    end = max(mtext)
    while end < len(recs) and recs[end][0] == recs[end - 1][0]:
        end += 1
    bounds = starts[1:] + [end + 1]
    out = []
    for sec in range(6):
        for n in range(starts[sec], bounds[sec]):
            loc, text = recs[n - 1]
            out.append(b'%d' % loc + text.rstrip(b' '))
        out.append(b'-1')
    # sections 7-12 verbatim from ADVENTURE.DAT
    lines = open(asc, 'rb').read().split(b'\n')
    seen = 0
    for i, ln in enumerate(lines):
        if ln.strip() == b'-1':
            seen += 1
            if seen == 6:
                rest = lines[i + 1:]
                break
    sys.stdout.buffer.write(b'\n'.join(out) + b'\n' + b'\n'.join(rest))

if __name__ == '__main__':
    main()
