"""basic\\LEGEND-LU.txt -> basic\\LEGEND-LU.ZYMB, the form SINTRAN keeps a
BASIC source in: CR LF line ends, even parity on every byte, ETB (027) at the end.

usage: python tools\\mkzymb.py [--check]
"""
import os
import sys

PORT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TXT = os.path.join(PORT, 'basic', 'LEGEND-LU.txt')
ZYMB = os.path.join(PORT, 'basic', 'LEGEND-LU.ZYMB')


def parity(b):
    n = bin(b & 0x7F).count('1')
    return (b & 0x7F) | (0 if n % 2 == 0 else 0x80)


def convert(text):
    out = bytearray()
    for line in text.split('\n'):
        if line == '' and len(out):
            continue
        for ch in line:
            out.append(parity(ord(ch)))
        out += bytes((parity(13), parity(10)))
    out.append(parity(0o27))
    return bytes(out)


def main():
    text = open(TXT, encoding='latin-1', newline='\n').read()
    new = convert(text)
    if '--check' in sys.argv:
        old = open(ZYMB, 'rb').read()
        print('same' if old == new else 'DIFFERS (%d vs %d bytes)' % (len(old), len(new)))
        if old != new:
            for i, (a, b) in enumerate(zip(old, new)):
                if a != b:
                    print('first difference at %d: %03o vs %03o' % (i, a, b))
                    break
        return 0 if old == new else 1
    open(ZYMB, 'wb').write(new)
    print('%s: %d bytes' % (ZYMB, len(new)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
