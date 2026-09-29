"""Read the members of a Honeywell LADC SST tape (CP-V, Xerox Sigma).

The tapes are SIMH .tap images.  Each member is a ':BOF' label record, a
tape mark, the data blocks, a tape mark, an ':EOF' label and a tape mark.
A data block starts with two halfwords - the length of the previous block
and the number of record segments in this one - and then holds the
segments of a keyed file, each padded to a word:

    key length (1 byte), key, flags (1 byte), 0, data length (halfword), data

flags 01 = first segment of a record, 02 = the record goes on in the next
block.  A line that crosses a block boundary is two segments with the same
key - the ASCII conversions in the LADC zips drop every such line, which is
why the port reads the tape itself.

The text is EBCDIC as the Sigma used it (CP-V Time-Sharing Reference,
Appendix A): the IBM graphics, plus B1-B5 = backslash { } [ ].
"""

import gzip
import struct

TM = None


def tape_records(path):
    """Yield the records of a SIMH tape image (None for a tape mark)."""
    op = gzip.open if path.endswith('.gz') else open
    with op(path, 'rb') as f:
        d = f.read()
    p = 0
    while p + 4 <= len(d):
        (n,) = struct.unpack('<I', d[p:p + 4])
        p += 4
        if n == 0:
            yield TM
            continue
        if n == 0xFFFFFFFF:
            break
        n &= 0x7FFFFFFF
        yield d[p:p + n]
        p += n + (n & 1) + 4


def members(path):
    """{name: (label, [blocks])} for every labelled member."""
    rs = list(tape_records(path))
    out = {}
    i = 0
    bof = ':BOF'.encode('cp037')
    while i < len(rs):
        r = rs[i]
        if r is not TM and r[:4] == bof:
            name = r[9:9 + r[8]].decode('cp037')
            assert rs[i + 1] is TM
            i += 2
            blocks = []
            while i < len(rs) and rs[i] is not TM:
                blocks.append(rs[i])
                i += 1
            out[name] = (r, blocks)
            continue
        i += 1
    return out


def keyed_records(blocks):
    """[(key, bytes)] of a keyed member, segments joined."""
    lines = []
    cur = None
    for b in blocks:
        _prev, nseg = struct.unpack('>HH', b[:4])
        p = 4
        for _ in range(nseg):
            kl = b[p]
            key = int.from_bytes(b[p + 1:p + 1 + kl], 'big')
            q = p + 1 + kl
            flags = b[q]
            n = int.from_bytes(b[q + 2:q + 4], 'big')
            data = b[q + 4:q + 4 + n]
            assert len(data) == n, 'short segment'
            if flags & 1:
                assert cur is None, 'unfinished record before key %d' % key
                cur = [key, bytearray(data)]
            else:
                assert cur is not None and cur[0] == key, 'orphan segment'
                cur[1] += data
            if not flags & 2:
                lines.append((cur[0], bytes(cur[1])))
                cur = None
            p += (1 + kl + 4 + n + 3) & ~3
    assert cur is None, 'unterminated record'
    return lines


# --- Xerox EBCDIC ------------------------------------------------------

PRINTABLE = {}
for _b in range(0x40, 0x100):
    try:
        _c = bytes([_b]).decode('cp037')
    except UnicodeDecodeError:
        continue
    if _c.isprintable():
        PRINTABLE[_b] = _c
PRINTABLE.update({0xB1: '\\', 0xB2: '{', 0xB3: '}', 0xB4: '[', 0xB5: ']'})
for _b in (0x4A, 0x4F, 0x5F, 0x6A):      # cent, bar, not, broken bar
    PRINTABLE.pop(_b, None)

# The control codes that occur in the Adventure text, as CP-V's COC
# treats them on output (Table A-3), and the byte the port uses for each:
#   07 BEL, 08 BS, 0D CR (COC prints CR LF), 0A NAK, 20 'LF only'.
CONTROL = {0x07: 0x07, 0x08: 0x08, 0x0D: 0x0D, 0x0A: 0x15, 0x20: 0x0A}


def internal(b):
    """EBCDIC record -> the port's internal bytes (ASCII + controls)."""
    out = bytearray()
    for c in b:
        if c in CONTROL:
            out.append(CONTROL[c])
        elif c in PRINTABLE:
            out += PRINTABLE[c].encode('latin-1')
        else:
            raise ValueError('unexpected EBCDIC byte %02X' % c)
    return bytes(out)


PICTURES = {0x07: '␇', 0x08: '␈', 0x0D: '␍', 0x15: '␕',
            0x0A: '␊'}


def readable(b):
    """EBCDIC record -> text, the control codes as Unicode pictures."""
    return ''.join(PICTURES.get(c, chr(c)) for c in internal(b))
