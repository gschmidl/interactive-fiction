#!/usr/bin/env python3
"""List a tokenised NorthStar BASIC program from the 101DISK.NSI image.

    detok.py [file-name] > listing.bas

The disk is NorthStar DOS, double density: 512-byte blocks, the directory
in the first blocks (16-byte entries: name, block address, length in
blocks, type).  A BASIC program (type 2) is a run of lines

    <length byte> <line number, 2 bytes little-endian> <tokens> 0D

and ends with a line of length 1.  The token values are the ones in the
keyword table of HYBASIC on the same disk (each keyword follows its token
byte there); 9A is a line number constant, two bytes little-endian.
"""

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DISK = os.path.join(HERE, '..', 'archive_original', '101DISK.NSI')
BLOCK = 512


def directory(d):
    out = {}
    for k in range(0, 4 * BLOCK, 16):
        e = d[k:k + 16]
        name = e[:8].decode('latin-1').strip()
        if not name or e[0] in (0, 0x20):
            continue
        out[name] = (e[8] | e[9] << 8, e[10] | e[11] << 8, e[12])
    return out


def keywords(interp):
    """token -> keyword, from the interpreter's table: 80LET 81FOR ..."""
    i = interp.find(b'\x80LET\x81FOR')
    assert i >= 0, 'no keyword table'
    table = {}
    j = i
    while j < len(interp) and interp[j] != 0xFF:
        tok = interp[j]
        j += 1
        k = j
        while k < len(interp) and interp[k] < 0x80:
            k += 1
        table.setdefault(tok, interp[j:k].decode('latin-1'))
        j = k
    return table


def listing(prog, kw):
    lines = []
    p = 0
    while p < len(prog):
        n = prog[p]
        if n == 1:                      # the end of the program
            break
        body = prog[p + 3:p + n - 1]
        assert prog[p + n - 1] == 0x0D, p
        num = prog[p + 1] | prog[p + 2] << 8
        out = ''
        i = 0
        q = False
        while i < len(body):
            c = body[i]
            if c == 0x22:
                q = not q
            if q or c < 0x80:
                out += chr(c)
                i += 1
                continue
            if c == 0x9A:               # a line number
                out += str(body[i + 1] | body[i + 2] << 8)
                i += 3
                continue
            out += kw[c] if c in kw else '<%02X>' % c
            i += 1
        lines.append('%d%s' % (num, out))
        p += n
    return lines, p


def main():
    name = sys.argv[1] if len(sys.argv) > 1 else 'DBACK'
    d = open(DISK, 'rb').read()
    dirs = directory(d)
    a, n, t = dirs['HYBASIC']
    kw = keywords(d[a * BLOCK:(a + n) * BLOCK])
    a, n, t = dirs[name]
    lines, used = listing(d[a * BLOCK:(a + n) * BLOCK], kw)
    sys.stdout.write('\n'.join(lines) + '\n')
    sys.stderr.write('%s: %d lines, %d bytes of %d\n'
                     % (name, len(lines), used, n * BLOCK))


if __name__ == '__main__':
    main()
