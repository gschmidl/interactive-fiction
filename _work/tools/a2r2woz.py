"""a2r2woz.py - convert an Applesauce A2R 2.x flux image of a 5.25" disk to WOZ 2.

Each timing capture of a whole track is decoded into bits (flux intervals to
4-microsecond bit cells through a slowly tracking clock) and cut to exactly
one revolution that joins up without a seam:

  - with sectors on the track, the revolution is the distance between the
    same address field on the first and the second time round, and the seam
    goes in the gap just before that field;
  - without (a track of sync bytes, say), it is the length at which a
    stretch of bits repeats best, 98% at least.

The track still starts at the index.  Of the captures of a track, the one
with the most good sectors (16-sector 6-and-2 or 13-sector 5-and-3, checked
down to the data field checksums) is kept.  The quarter-tracks either side of
a whole track read it too, as a drive head does; the half-tracks are blank.

Limits: A2R 2.x only (not A2R 3), 5.25" disks only, and only whole tracks go
into the WOZ - `report` lists every quarter-track, so a disk that keeps data
of its own on half or quarter tracks shows up there and needs more than this.
Weak bits are not modelled.  Written for JACM0350 (Adventure, SoftWareHouse
1981, Disk 1 Side A): all 16 sectors decode on every track, the sectors agree
with adafruit/a2woz's conversion wherever that has them, and the disk boots in
AppleWin.

    python a2r2woz.py report IMAGE.a2r
    python a2r2woz.py convert IMAGE.a2r OUT.woz

Needs numpy.
"""
import argparse
import struct
import zlib

import numpy as np

NOMINAL_CELL = 32.0          # 4 microseconds in 125 ns ticks


def read_a2r(path):
    """INFO fields, META dict, and the STRM captures (location, type, loop, flux)."""
    data = open(path, 'rb').read()
    if data[:4] != b'A2R2' or data[4:8] != b'\xff\n\r\n':
        raise SystemExit('%s: not an A2R 2.x file' % path)
    info, meta, caps = None, {}, []
    pos = 8
    while pos + 8 <= len(data):
        cid, size = data[pos:pos + 4], struct.unpack('<I', data[pos + 4:pos + 8])[0]
        body = data[pos + 8:pos + 8 + size]
        if cid == b'INFO':
            info = {'creator': body[1:33].decode('latin-1').strip(), 'drive': body[33],
                    'wp': body[34], 'sync': body[35]}
        elif cid == b'META':
            for line in body.decode('utf-8', 'replace').split('\n'):
                if '\t' in line:
                    k, v = line.split('\t', 1)
                    meta[k] = v
        elif cid == b'STRM':
            q = 0
            while body[q] != 0xFF:
                loc, ctype = body[q], body[q + 1]
                n, loop = struct.unpack('<II', body[q + 2:q + 10])
                caps.append((loc, ctype, loop, body[q + 10:q + 10 + n]))
                q += 10 + n
        pos += 8 + size
    if info is None or info['drive'] != 1:
        raise SystemExit('%s: not a 5.25" capture' % path)
    return info, meta, caps


def intervals(flux):
    """Flux intervals in 125 ns ticks; a 255 byte carries on into the next."""
    acc = 0
    for b in flux:
        acc += b
        if b != 255:
            yield acc
            acc = 0


def odd_even(a, b):
    """A 4-and-4 encoded address field byte."""
    return ((a << 1) | 1) & b


def address_marks(bits):
    """(bit position, (volume, track, sector)) of every good address field."""
    out, reg, nib = [], 0, []
    for i, b in enumerate(bits):
        reg = ((reg << 1) | b) & 0xFF
        if reg & 0x80:
            nib.append((reg, i))
            reg = 0
    v = [x for x, _ in nib]
    for i in range(len(v) - 11):
        if v[i] == 0xD5 and v[i + 1] == 0xAA and v[i + 2] in (0x96, 0xB5):
            f = v[i + 3:i + 11]
            vol, trk, sec, chk = (odd_even(f[0], f[1]), odd_even(f[2], f[3]),
                                  odd_even(f[4], f[5]), odd_even(f[6], f[7]))
            if vol ^ trk ^ sec ^ chk == 0:
                out.append((nib[i][1], (vol, trk, sec)))
    return out


def flux_to_bits(flux, loop):
    """One revolution of a capture as a list of bits, and how well it
    repeats (1.0 when cut at a sector); (None, 0.0) if it cannot be cut."""
    bits = []
    cell = NOMINAL_CELL
    t = 0
    est = None
    for iv in intervals(flux):
        t += iv
        n = max(1, int(iv / cell + 0.5))
        if n <= 3:
            cell += (iv / n - cell) * 0.02
        bits.extend([0] * (n - 1))
        bits.append(1)
        if est is None and t >= loop:
            est = len(bits)
    if est is None:
        return None, 0.0
    # With sectors: L is the distance between the same address field in the
    # first and the second revolution, and the seam goes in the gap just
    # before that field, so the sector lines up across it exactly.  The
    # track's first OFF bits come from the second time round, so it still
    # starts at the index.  (The gaps between sectors need not repeat bit for
    # bit - self-sync bytes can come back a bit longer or shorter - which is
    # why matching whole stretches of bits is only the fallback.)
    marks = address_marks(bits)
    for p1, key in marks:
        if p1 < 200 or p1 >= est:
            continue
        p2 = next((q for q, k in marks if k == key and est - 400 <= q - p1 <= est + 400), None)
        if p2 is not None:
            L, off = p2 - p1, p1 - 120
            if off + L <= len(bits):
                return bits[L:L + off] + bits[off:L], 1.0
    # Without sectors: the L that agrees best (98% at least) with a stretch
    # of the start, nearest the estimate on a tie.  The stretch may begin
    # further in when the first bits after the index do not come back the
    # same; the track's first OFF bits then come from the second time round.
    arr = np.frombuffer(bytes(bits), dtype=np.uint8)
    K = 4000
    for off in (0, 6000, 10000):
        ref = arr[off:off + K]
        best, best_score = None, -1
        for d in range(0, 400):
            for L in (est - d, est + d):
                if off + L + K <= len(arr):
                    s = int((arr[off + L:off + L + K] == ref).sum())
                    if s > best_score:
                        best, best_score = L, s
        if best is not None and best_score >= 0.98 * K:
            return bits[best:best + off] + bits[off:best], best_score / K
    return None, 0.0


def nibbles(bits):
    """The Disk II latch: shift bits in, a byte is done when bit 7 is set.
    The track is read twice round so that fields across the seam count."""
    out = []
    reg = 0
    for b in bits + bits:
        reg = ((reg << 1) | b) & 0xFF
        if reg & 0x80:
            out.append(reg)
            reg = 0
    return out


# the disk bytes of 6-and-2 (16-sector) and 5-and-3 (13-sector) encoding
T62 = {v: i for i, v in enumerate([
    0x96, 0x97, 0x9A, 0x9B, 0x9D, 0x9E, 0x9F, 0xA6, 0xA7, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
    0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xCB,
    0xCD, 0xCE, 0xCF, 0xD3, 0xD6, 0xD7, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 0xE5,
    0xE6, 0xE7, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6,
    0xF7, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF])}
T53 = {v: i for i, v in enumerate([
    0xAB, 0xAD, 0xAE, 0xAF, 0xB5, 0xB6, 0xB7, 0xBA, 0xBB, 0xBD, 0xBE, 0xBF, 0xD6, 0xD7,
    0xDA, 0xDB, 0xDD, 0xDE, 0xDF, 0xEA, 0xEB, 0xED, 0xEE, 0xEF, 0xF5, 0xF6, 0xF7, 0xFA,
    0xFB, 0xFD, 0xFE, 0xFF])}


def scan(nibs):
    """Sectors whose data field checks out, 16- and 13-sector, and the track
    numbers the address fields give."""
    res = {'16': {}, '13': {}, 'tracks': set()}
    n = len(nibs)
    i = 0
    while i < n - 20:
        if nibs[i] == 0xD5 and nibs[i + 1] == 0xAA and nibs[i + 2] in (0x96, 0xB5):
            kind = '16' if nibs[i + 2] == 0x96 else '13'
            f = nibs[i + 3:i + 11]
            vol, trk, sec, chk = (odd_even(f[0], f[1]), odd_even(f[2], f[3]),
                                  odd_even(f[4], f[5]), odd_even(f[6], f[7]))
            if vol ^ trk ^ sec ^ chk == 0:
                res['tracks'].add(trk)
                # the data field follows within a few dozen nibbles
                j = i + 11
                end = min(n - 3, j + 60)
                while j < end and not (nibs[j] == 0xD5 and nibs[j + 1] == 0xAA and nibs[j + 2] == 0xAD):
                    j += 1
                if j < end:
                    size, tab = (343, T62) if kind == '16' else (411, T53)
                    body = nibs[j + 3:j + 3 + size]
                    if len(body) == size and all(x in tab for x in body):
                        acc = 0
                        for x in body:
                            acc ^= tab[x]
                        if acc == 0:
                            res[kind].setdefault(sec, trk)
            i += 11
        else:
            i += 1
    return res


def analyse(caps, whole_only=True):
    """Best capture per location: most good sectors, then the cleanest
    repeat.  A track with no sectors at all is kept too."""
    by_loc = {}
    for loc, ctype, loop, flux in caps:
        if ctype != 1 or loop == 0 or (whole_only and loc % 4):
            continue
        bits, match = flux_to_bits(flux, loop)
        if bits is None:
            continue
        r = scan(nibbles(bits))
        score = max(len(r['16']), len(r['13']))
        best = by_loc.get(loc)
        if best is None or (score, match) > (best['score'], best['match']):
            by_loc[loc] = {'score': score, 'match': match, 'bits': bits, 'res': r}
    return by_loc


def write_woz(path, info, meta, by_loc, tmap):
    tracks = sorted(set(v for v in tmap if v is not None))
    index = {loc: k for k, loc in enumerate(tracks)}
    tmap_bytes = bytes(index[v] if v is not None else 0xFF for v in tmap)
    trk_entries = b''
    trk_data = b''
    block = 3                   # track data starts at byte 1536
    largest = 0
    for loc in tracks:
        bits = by_loc[loc]['bits']
        raw = bytearray((len(bits) + 7) // 8)
        for k, b in enumerate(bits):
            if b:
                raw[k >> 3] |= 0x80 >> (k & 7)
        nblocks = (len(raw) + 511) // 512
        largest = max(largest, nblocks)
        trk_entries += struct.pack('<HHI', block, nblocks, len(bits))
        trk_data += bytes(raw) + b'\0' * (nblocks * 512 - len(raw))
        block += nblocks
    trk_entries += b'\0' * (160 * 8 - len(trk_entries))
    any16 = any(len(by_loc[l]['res']['16']) for l in tracks)
    any13 = any(len(by_loc[l]['res']['13']) for l in tracks)
    fmt = 3 if any16 and any13 else 1 if any16 else 2 if any13 else 0
    creator = ('a2r2woz.py from ' + info['creator'])[:32].ljust(32).encode('utf-8')
    ram = int(meta.get('requires_ram', '0').rstrip('K') or 0)
    info_body = struct.pack('<BBBBB', 2, 1, info['wp'], info['sync'], 0) + creator + \
        struct.pack('<BBBHHH', 1, fmt, 32, 0, ram, largest)
    info_body += b'\0' * (60 - len(info_body))
    chunks = b'INFO' + struct.pack('<I', 60) + info_body
    chunks += b'TMAP' + struct.pack('<I', 160) + tmap_bytes
    chunks += b'TRKS' + struct.pack('<I', len(trk_entries) + len(trk_data)) + trk_entries + trk_data
    keys = ['title', 'subtitle', 'publisher', 'developer', 'copyright', 'version', 'language',
            'requires_ram', 'requires_machine', 'notes', 'side', 'side_name', 'contributor',
            'image_date']
    meta_text = '\n'.join('%s\t%s' % (k, meta.get(k, '')) for k in keys if k in meta).encode('utf-8')
    chunks += b'META' + struct.pack('<I', len(meta_text)) + meta_text
    assert chunks.index(b'TRKS') + 8 + 1280 == 1536 - 12
    head = b'WOZ2\xff\n\r\n' + struct.pack('<I', zlib.crc32(chunks) & 0xFFFFFFFF)
    open(path, 'wb').write(head + chunks)
    return tracks


def main():
    ap = argparse.ArgumentParser(description='Applesauce A2R 2.x (5.25") to WOZ 2.')
    sub = ap.add_subparsers(dest='cmd', required=True)
    rp = sub.add_parser('report', help='list every captured quarter-track and what decodes on it')
    rp.add_argument('a2r')
    cp = sub.add_parser('convert', help='write the whole tracks to a WOZ 2 file')
    cp.add_argument('a2r')
    cp.add_argument('woz')
    args = ap.parse_args()
    info, meta, caps = read_a2r(args.a2r)
    if args.cmd == 'report':
        print('%s; %s, %s' % (info['creator'], meta.get('title', ''), meta.get('side', '')))
        by_loc = analyse(caps, whole_only=False)
        for loc in sorted(by_loc):
            b = by_loc[loc]
            r = b['res']
            print('qt %3d (track %5.2f)  bits %6d  repeat %.3f  16-sector good %2d  '
                  '13-sector good %2d  address tracks %s'
                  % (loc, loc / 4, len(b['bits']), b['match'], len(r['16']), len(r['13']),
                     sorted(r['tracks'])[:6]))
        return
    by_loc = analyse(caps)
    tmap = [None] * 160
    for loc in by_loc:
        for q in (loc - 1, loc, loc + 1):
            if 0 <= q < 160:
                tmap[q] = loc
    tracks = write_woz(args.woz, info, meta, by_loc, tmap)
    print('wrote %s: %d tracks' % (args.woz, len(tracks)))
    for loc in tracks:
        r = by_loc[loc]['res']
        if max(len(r['16']), len(r['13'])) < 16 and len(r['13']) < 13:
            print('  track %d: %d good 16-sector, %d good 13-sector' % (loc // 4, len(r['16']), len(r['13'])))


if __name__ == '__main__':
    main()
