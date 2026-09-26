#!/usr/bin/env python3
"""Compare this game's database with the one in the MSU port, message by
message.

Both are Woods' 350-point database in the same "number then text" format, and
both descend from Gary Palter's portable Adventure, so their text should agree
except where MSU changed something.  This database, though, is a hand
transcription of a 1979 printout - so any line that differs is either an MSU
edit or a slip in the transcription, and worth a look either way.

usage: python cmpdata.py [other-database]
The default is the MSU port's ADVTDATA, which was itself checked against the
original running on MVS 3.8j.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
MINE = os.path.join(HERE, '..', '.build', 'adv.data')
OTHER = os.path.join(
    HERE, '..', '..', '..',
    '(MOOR0350) Colossal Cave Adventure 350pt (IBM MVS, MSU FORTRAN,'
    ' CBT COV466)', 'src_original', 'CBT.COV466.FILE119.PDS', 'ADVTDATA.txt')

TEXT_SECTIONS = (1, 2, 5, 6, 10, 12)


def read(path):
    """{section: {number: [lines]}} for the text sections, and
    {section: [normalised numeric lines]} for the rest."""
    raw = open(path, 'rb').read().decode('latin-1')
    lines = [l.rstrip() for l in raw.replace('\r\n', '\n').split('\n')
             if not l.startswith('==p') and not l.startswith('   21MAR79')]
    out, sect, i = {}, None, 0
    while i < len(lines):
        l = lines[i]
        i += 1
        if not l.strip():
            continue
        if sect is None:
            sect = int(l.strip())
            out[sect] = {} if sect in TEXT_SECTIONS else []
            continue
        if l.strip() == '-1':
            sect = None
            continue
        if sect in TEXT_SECTIONS:
            n, text = int(l[:8]), l[8:].rstrip()
            out[sect].setdefault(n, []).append(text)
        elif sect == 4:
            out[sect].append((int(l[:8]), l[8:13].rstrip()))
        else:
            out[sect].append([int(x) for x in l.split()])
    return out


def digest(sect, rows):
    """The numeric sections mean the same thing whatever the record length -
    and the two databases use different ones (this one packs fourteen motions
    on a travel line, the MSU one eight) - so compare what they say, not how
    it is laid out.  Keys are what the program indexes by; values are lists,
    because order matters in the travel table."""
    out = {}
    for r in rows:
        if sect == 3:
            out.setdefault(r[0], []).extend((r[1], m) for m in r[2:] if m)
        elif sect == 4:
            out[r[1]] = r[0]                        # word -> number
        elif sect in (7, 8):
            out[r[0]] = r[1:]
        elif sect == 9:
            out.setdefault(r[0], []).extend(x for x in r[1:] if x)
        elif sect == 11:
            out[r[0]] = r[1:5]
        else:
            out.setdefault(r[0], []).append(r[1:])
    if sect == 9:
        for k in out:
            out[k] = sorted(out[k])
    return out


def main():
    a = read(MINE)
    b = read(sys.argv[1] if len(sys.argv) > 1 else OTHER)
    print('sections here %s' % sorted(a))
    print('sections there %s' % sorted(b))
    diffs = 0
    for s in sorted(set(a) | set(b)):
        if s not in a or s not in b:
            print('section %d: only in %s' % (s, 'here' if s in a else 'there'))
            continue
        if s in TEXT_SECTIONS:
            for n in sorted(set(a[s]) | set(b[s])):
                ta, tb = a[s].get(n), b[s].get(n)
                if ta == tb:
                    continue
                diffs += 1
                print('\nsection %d, %d:' % (s, n))
                for l in (ta or ['(absent)']):
                    print('  here  |%s|' % l)
                for l in (tb or ['(absent)']):
                    print('  there |%s|' % l)
        else:
            da, db = digest(s, a[s]), digest(s, b[s])
            for k in sorted(set(da) | set(db)):
                if da.get(k) == db.get(k):
                    continue
                diffs += 1
                print('\nsection %d, %s:\n  here  %s\n  there %s'
                      % (s, k, da.get(k, '(absent)'), db.get(k, '(absent)')))
    print('\n%d differences' % diffs)


if __name__ == '__main__':
    main()
