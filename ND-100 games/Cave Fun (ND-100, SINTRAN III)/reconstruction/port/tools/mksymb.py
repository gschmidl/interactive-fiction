"""basic\ADV-INTER-CB-MJ.txt -> basic\ADV-INTER-CB-MJ.SYMB, the form SINTRAN
keeps a BASIC source in: CR LF line ends, even parity on every byte, ETB (027)
at the end.  (As the Legend port's mkzymb.py.)

usage: python tools\mksymb.py [--check]
       python tools\mksymb.py --text FILE.SYMB FILE.txt   (the other way)
"""
import os
import sys

PORT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TXT = os.path.join(PORT, 'basic', 'ADV-INTER-CB-MJ.txt')
SYMB = os.path.join(PORT, 'basic', 'ADV-INTER-CB-MJ.SYMB')


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


def to_text(data):
    return bytes(b & 0x7F for b in data).split(b'\x17')[0].decode('latin-1').replace('\r\n', '\n')


def main():
    if sys.argv[1:2] == ['--text']:
        open(sys.argv[3], 'w', encoding='latin-1', newline='\n').write(to_text(open(sys.argv[2], 'rb').read()))
        return 0
    new = convert(open(TXT, encoding='latin-1', newline='\n').read())
    if '--check' in sys.argv:
        old = open(SYMB, 'rb').read()
        print('same' if old == new else 'DIFFERS (%d vs %d bytes)' % (len(old), len(new)))
        return 0 if old == new else 1
    open(SYMB, 'wb').write(new)
    print('%s: %d bytes' % (SYMB, len(new)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
