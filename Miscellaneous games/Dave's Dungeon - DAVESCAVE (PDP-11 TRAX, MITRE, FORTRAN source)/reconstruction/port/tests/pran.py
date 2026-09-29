"""Check the port's RAN against a model of FOR$IRAN's VAX instructions."""
import os, struct, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))

def cvtlf(v):                       # CVTLF, rounding half away from zero
    v &= 0xFFFFFFFF
    if v & 0x80000000: v -= 1 << 32
    s, a = (-1 if v < 0 else 1), abs(v)
    nb = a.bit_length()
    if nb > 24:
        sh = nb - 24
        a = (a + (1 << (sh - 1))) >> sh
        a <<= sh
    return float(s * a)

def iran(i1, i2):
    r0 = ((i1 & 0xFFFF) << 16) | (i2 & 0xFFFF)
    if i2 & 0xFFFF == 0:
        r0 = (r0 + 0x10000) & 0xFFFFFFFF
        r1 = (r0 & 0xFFFF0000) | 3
    else:
        r1 = (r0 * 65539) & 0xFFFFFFFF
        r1 &= 0x7FFFFFFF
    x = cvtlf(r1) * 2.0 ** -31
    lo, hi = r1 & 0xFFFF, (r1 >> 16) & 0xFFFF
    s16 = lambda w: w - 0x10000 if w & 0x8000 else w
    return x, s16(hi), s16(lo)

def main():
    out = subprocess.run([os.path.join(HERE, '..', '.build', 'pran.exe')],
                         stdout=subprocess.PIPE).stdout.decode().split('\n')
    seeds = [(0, 1), (1234, 5678), (-1, 0), (32767, -32768)]
    bad, i = 0, 0
    for k, (a, b) in enumerate(seeds, 1):
        for n in range(1, 7):
            x, a, b = iran(a, b)
            want = '%2d%3d %08X%8d%8d' % (k, n, struct.unpack('<I', struct.pack('<f', x))[0], a, b)
            got = out[i].rstrip()
            if got != want:
                print('DIFF', got, '|', want); bad += 1
            i += 1
    print('pran: %d of 24 draws match the VAX model' % (24 - bad))
    return 1 if bad else 0

sys.exit(main())
