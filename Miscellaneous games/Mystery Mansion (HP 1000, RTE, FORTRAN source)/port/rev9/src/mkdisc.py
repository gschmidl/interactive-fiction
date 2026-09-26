"""Build the SIMH disc image for the revision 9 run: file 3 of the DSKUP tape
(archive_original/f1_dskup_f2_rte2250sys.tap.gz) is a save of the removable
platter of an HP 7906 holding the site's RTE-IVB system ("RTE IV B  7906 DISC
SYS. REV E; LEN ROSE 14 APR 83 REMOVEABLE PLATER").  It is one 280-byte label
record and 1233 records of 4100 bytes: a 2-word prefix (flag, track number)
and 2048 words, three records to a 6144-word track, tracks 0-410 in order.

SIMH's 7906 image keeps the removable platter first, track after track, with
16-bit words little-endian; the fixed platter after it stays empty.

    mkdisc.py OUTFILE"""
import gzip
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TAPE = os.path.join(HERE, '..', '..', '..', 'archive_original', 'f1_dskup_f2_rte2250sys.tap.gz')
SIZE = 411 * 4 * 48 * 256          # 7906: 411 cylinders, 4 heads, 48 sectors of 128 words


def main():
    if len(sys.argv) != 2:
        sys.exit('usage: mkdisc.py OUTFILE')
    if not os.path.exists(TAPE):
        sys.exit('mkdisc: no %s\n(a whole site\'s disc save, kept out of the repository: it is bitsavers.org '
                 'bits/HP/Crisis_Computer_Tapes/ccc_9trkTapes_20050826/f1_dskup_f2_rte2250sys.tap.gz)'
                 % os.path.normpath(TAPE))
    d = gzip.open(TAPE, 'rb').read()
    p, f, label, recs = 0, 0, None, []
    while p + 4 <= len(d):
        (n,) = struct.unpack('<I', d[p:p + 4])
        p += 4
        if n == 0:
            f += 1
            continue
        if n == 0xFFFFFFFF:
            break
        ln = n & 0x7FFFFFFF
        if f == 3:
            if ln == 280:
                label = d[p:p + 80]
            elif ln == 4100:
                recs.append(d[p:p + ln])
        p += ln + (ln & 1) + 4
    assert label and label.startswith(b'RTE IV B  7906 DISC SYS. REV E'), label
    assert len(recs) == 1233, len(recs)
    out = bytearray()
    for i, r in enumerate(recs):
        flag, trk = struct.unpack('>2H', r[:4])
        assert trk == i // 3, (i, trk)
        body = r[4:]
        out += b''.join(body[j + 1:j + 2] + body[j:j + 1] for j in range(0, len(body), 2))
    out += bytes(SIZE - len(out))
    with open(sys.argv[1], 'wb') as fp:
        fp.write(out)
    print('%s: %s' % (sys.argv[1], label[:72].decode('latin-1').strip()))


if __name__ == '__main__':
    main()
