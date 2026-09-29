"""Read the INTEREX CSL/1000 startup tape (1986).

The tape is a SIMH image (4-byte little-endian record lengths) of 10240-byte
records written by RTE's TF ("tape filer"): a 512-byte volume header, then
for each file a 512-byte tar-like header - name at 0, octal mode at 100,
octal size at 124, checksum at 148 - and the file's bytes, padded to 512.

An RTE FMP file keeps its records on the tape as FMP stores them:
a length in 16-bit words, that many big-endian words, the length again;
a length of -1 ends the file.  Source (type 4) records are ASCII lines.
"""

import struct


def tape_records(path):
    with open(path, 'rb') as f:
        d = f.read()
    p = 0
    out = []
    while p + 4 <= len(d):
        (n,) = struct.unpack('<I', d[p:p + 4])
        p += 4
        if n == 0:
            out.append(None)
            continue
        if n == 0xFFFFFFFF:
            break
        n &= 0x7FFFFFFF
        out.append(d[p:p + n])
        p += n + (n & 1) + 4
    return out


def tf_files(path):
    """[(name, header, bytes)] of a TF tape."""
    stream = b''.join(r for r in tape_records(path) if r)
    p = 512                     # the volume header
    out = []
    while p + 512 <= len(stream):
        h = stream[p:p + 512]
        name = h[:100].split(b'\0')[0].decode('latin-1').strip()
        if not name:
            break
        size = int(h[124:136].strip() or b'0', 8)
        out.append((name, h, stream[p + 512:p + 512 + size]))
        p += 512 + ((size + 511) // 512) * 512
    return out


def fmp_records(data):
    """The records of an FMP variable-length file, and the bytes used."""
    p = 0
    out = []
    while p + 2 <= len(data):
        (n,) = struct.unpack('>H', data[p:p + 2])
        if n == 0xFFFF:
            break
        rec = data[p + 2:p + 2 + 2 * n]
        (m,) = struct.unpack('>H', data[p + 2 + 2 * n:p + 4 + 2 * n])
        if m != n:
            raise ValueError('record at %d: length %d, trailer %d' % (p, n, m))
        out.append(rec)
        p += 4 + 2 * n
    return out, p
