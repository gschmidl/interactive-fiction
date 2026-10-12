"""Read an AOS (pre-AOS/VS, 16-bit) linker symbol table.

Records are [flags][name length][value][w1][w2][w3][name, padded to even].
"""
import struct, sys

def read_st16(path):
    d = open(path, 'rb').read()
    out = []
    i = 0
    while i + 10 <= len(d):
        fl, n = d[i], d[i+1]
        if n == 0:
            # the table is written in 512-byte blocks, each zero-filled at
            # the end: go on to the next block
            nxt = (i // 512 + 1) * 512
            if nxt >= len(d): break
            i = nxt
            continue
        if n > 32:
            break
        val, w1, w2, w3 = struct.unpack_from('>4H', d, i + 2)
        name = d[i+10:i+10+n]
        if not all(32 < c < 127 for c in name):
            nxt = (i // 512 + 1) * 512
            if nxt >= len(d): break
            i = nxt
            continue
        out.append((val, name.decode(), fl, w1, w2, w3))
        i += 10 + n + (n & 1)
    return out

if __name__ == '__main__':
    for val, name, fl, w1, w2, w3 in sorted(read_st16(sys.argv[1])):
        print('%04X %-10s fl=%02X %04X %04X %04X' % (val, name, fl, w1, w2, w3))
