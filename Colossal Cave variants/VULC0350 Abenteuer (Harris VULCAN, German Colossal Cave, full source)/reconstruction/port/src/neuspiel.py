#!/usr/bin/env python3
"""Turn the site's NEUSPIEL into this port's NEUSPIEL.DAT.

    neuspiel.py <fast.tap> <converted source dir> <NEUSPIEL.DAT>

NEUSPIEL is the file the main program loads first (LDCOMN(.TRUE.)): the
game as the site had it set up.  It is a game saved at its first command -
SICHR MEIN, on Friday 13 June 1980 at 14:34 (SAVED 1441, SAVET 874) - and
renamed: SETUP is -1, so the program goes on from there (8305) instead of
reading ADV.DATA.

On the tape (the FAST save fast.tap, a SIMH image whose records make one
stream) a file is a 25-word header - the name in 6-bit code (ASCII & 077)
in words 0 and 1, the size in 112-word sectors in word 14 - then the eight
bytes 4B 00 00 00 86 0A 00 00, then the data: segments of 2688 bytes (896
24-bit words, eight sectors), each followed by 14 bytes 00 00 00 s s s 86
0A 00 00 86 0A 00 00 where s is the sum of the segment's words, modulo
2**24 (the last segment is stored whole; its gap ends in zeros).

In NEUSPIEL, the eleven stretches of COMMON the main program lists
(CMADRS, CMSZES) follow one another, each from a sector boundary, as the
Harris IO routine wrote them.  Their layout is the main program's own
declarations (read from the converted main.f): every variable INTEGER*6,
two words, a value hi * 2**23 + (lo & 0x7FFFFF) with hi signed; INTEGER*3
and LOGICAL one word; no padding.  A LOGICAL is true when its sign bit is
set: the constant .TRUE. is all ones (BLKLIN), and WZDARK, which the
program sets every turn to DARK(0), is 000001 - false, since the road is
lit.  Here INTEGER*6 is
INTEGER*8 and INTEGER*3 INTEGER*4, as src\\convert.py makes them, LOGICAL
the default four bytes (.TRUE. 1), and the stretches follow one another
without gaps - which is what PCLOAD (pharrisc.c) reads.

Two things are not taken as they are:
  TTYI TTYO DBFI  the Harris's logical units, 0 3 10, are saved with the
                  game; here they are 5 6 10 (IOINIT's)
  HNAME           the name of a holiday, read 20A1: on the Harris each
                  character in the high byte of a double word; here its
                  code (all twenty are zero in NEUSPIEL - there was none)
"""

import re
import struct
import sys

START = b'\x4b\x00\x00\x00\x86\x0a\x00\x00'
SEGMENT = 2688
GAP = 14
SECTOR = 112


def stream(path):
    """the records of a SIMH tape image, one after another"""
    d = open(path, 'rb').read()
    out = bytearray()
    off = 0
    while off + 4 <= len(d):
        n = struct.unpack('<I', d[off:off + 4])[0]
        if n == 0:
            off += 4
            continue
        if n == 0xFFFFFFFF:
            break
        n &= 0xFFFFFF
        out += d[off + 4:off + 4 + n]
        off += 4 + n + (n & 1) + 4
    return bytes(out)


def word(d, p):
    return int.from_bytes(d[p:p + 3], 'big')


def sixbit(w):
    return ''.join(chr(v + 64) if v < 32 else chr(v)
                   for v in ((w >> s) & 63 for s in (18, 12, 6, 0)))


def cut(d, name):
    """the words of the file NAME"""
    found = []
    i = d.find(START)
    while i >= 0:
        h = i - 75
        if h >= 0 and (sixbit(word(d, h)) + sixbit(word(d, h + 3))).rstrip() \
                == name:
            found.append(h)
        i = d.find(START, i + 1)
    if len(found) != 1:
        sys.exit('neuspiel: %d files named %s on the tape' % (len(found), name))
    h = found[0]
    words = word(d, h + 42) * SECTOR
    p = h + 75 + len(START)
    data = bytearray()
    while len(data) < 3 * words:
        seg = d[p:p + SEGMENT]
        gap = d[p + SEGMENT:p + SEGMENT + GAP]
        s = sum(word(seg, k) for k in range(0, SEGMENT, 3)) & 0xFFFFFF
        if gap[:3] != b'\0\0\0' or word(gap, 3) != s or \
                gap[6:10] != b'\x86\x0a\0\0':
            sys.exit('neuspiel: %s: segment %d does not add up'
                     % (name, len(data) // SEGMENT))
        data += seg
        p += SEGMENT + GAP
    return [word(data, k) for k in range(0, 3 * words, 3)]


# ------------------------------------------------------------ layout
def statements(path):
    """the main program's statements up to its statement functions"""
    out = []
    for line in open(path, encoding='latin-1').read().split('\n'):
        if line[:1] in ('C', 'c', '*') or not line.strip():
            continue
        if len(line) > 5 and line[5] not in ' 0' and out:
            out[-1] += line[6:72]
        else:
            out.append(line[6:72].strip())
    return out


def layout(stmts):
    """{name: (block, harris word offset, port byte offset, kind, count)}
    and the eleven stretches [(first, i, last, j)]"""
    commons, dims, kind = {}, {}, {}
    ranges = {}
    for s in stmts:
        m = re.match(r'COMMON\s*(/.*)', s)
        if m:
            for blk, names in re.findall(r'/(\w+)/\s*([^/]*)', m.group(1)):
                commons[blk] = [n for n in names.replace(' ', '').split(',')
                                if n]
            continue
        m = re.match(r'(DIMENSION|LOGICAL|INTEGER\*4)\s+(.*)', s)
        if m:
            for name, d in re.findall(r'(\w+)(?:\(([^)]*)\))?', m.group(2)):
                if m.group(1) != 'DIMENSION':
                    kind[name] = m.group(1)
                if d:
                    n = 1
                    for x in d.split(','):
                        n *= int(x)
                    dims[name] = n
            continue
        m = re.match(r'CMSZES\((\d+)\)=PADDR\((\w+)(?:\((\d+)\))?\)'
                     r'-PADDR\((\w+)(?:\((\d+)\))?\)', s)
        if m:
            k, last, j, first, i = m.groups()
            ranges[int(k)] = (first, int(i or 1), last, int(j or 1))
    if sorted(ranges) != list(range(1, 12)):
        sys.exit('neuspiel: main.f does not list eleven stretches')
    lay = {}
    for blk, names in commons.items():
        hw = pb = 0
        for n in names:
            k = kind.get(n, 'INTEGER*6')
            cnt = dims.get(n, 1)
            lay[n] = (blk, hw, pb, k, cnt)
            hw += (2 if k == 'INTEGER*6' else 1) * cnt
            pb += (8 if k == 'INTEGER*6' else 4) * cnt
    return lay, [ranges[k] for k in range(1, 12)]


def signed24(w):
    return w - (1 << 24) if w & 0x800000 else w


PORT_UNITS = {'TTYI': (0, 5), 'TTYO': (3, 6), 'DBFI': (10, 10)}


def convert(words, lay, ranges):
    out = bytearray()
    at = 0                                  # the stretch's first sector
    odd = 0
    other = []
    for first, i, last, j in ranges:
        blk, hw0, pb0, k0, _ = lay[first]
        blk2, hw1, pb1, k1, _ = lay[last]
        if blk != blk2:
            sys.exit('neuspiel: %s and %s are not in one COMMON' % (first, last))
        hsize = {'INTEGER*6': 2}.get(k0, 1)
        start = hw0 + hsize * (i - 1)
        end = hw1 + {'INTEGER*6': 2}.get(k1, 1) * j
        members = sorted((v[1], n) for n, v in lay.items() if v[0] == blk)
        for _, n in members:
            _, hw, pb, k, cnt = lay[n]
            e = 2 if k == 'INTEGER*6' else 1
            for x in range(cnt):
                w = hw + e * x
                if w < start or w >= end:
                    continue
                p = at * SECTOR + w - start
                if k == 'INTEGER*6':
                    hi, lo = words[p], words[p + 1]
                    if n == 'HNAME':
                        v = (hi >> 16) & 0x7F if hi else 0
                    else:
                        v = signed24(hi) * (1 << 23) + (lo & 0x7FFFFF)
                        if (lo >> 23) != (hi >> 23):
                            odd += 1
                    out += struct.pack('<q', v)
                elif k == 'LOGICAL':
                    if words[p] not in (0, 0xFFFFFF):
                        other.append('%s=%06X' % (n, words[p]))
                    out += struct.pack('<i', 1 if words[p] & 0x800000 else 0)
                else:
                    v = signed24(words[p])
                    if n in PORT_UNITS:
                        if v != PORT_UNITS[n][0]:
                            sys.exit('neuspiel: %s is %d' % (n, v))
                        v = PORT_UNITS[n][1]
                    out += struct.pack('<i', v)
        at += -(-(end - start) // SECTOR)
    return bytes(out), at, odd, other


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__.split('\n\n')[1])
    tap, src, dat = sys.argv[1:]
    words = cut(stream(tap), 'NEUSPIEL')
    lay, ranges = layout(statements(src.rstrip('/\\') + '/main.f'))
    data, sectors, odd, other = convert(words, lay, ranges)
    if sectors * SECTOR > len(words):
        sys.exit('neuspiel: NEUSPIEL is %d sectors, the stretches need %d'
                 % (len(words) // SECTOR, sectors))
    open(dat, 'wb').write(data)
    print('neuspiel: %d of NEUSPIEL\'s %d sectors -> %s, %d bytes%s'
          % (sectors, len(words) // SECTOR, dat, len(data),
             ', %d double words not in the usual form' % odd if odd else ''))
    if other:
        print('neuspiel: LOGICALs neither 0 nor all ones (false: sign bit '
              'clear): ' + ' '.join(other))


if __name__ == '__main__':
    main()
