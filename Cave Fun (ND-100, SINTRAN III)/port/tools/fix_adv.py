"""..\\src_original\\CAVE-FUN-MJ.ADV -> data\\CAVE-FUN-MJ.ADV with the port's two
fixes to the game's rules (NOTES.md, "The port's fixes"); everything else is
left byte for byte.

usage: python tools\\fix_adv.py

Rule 18 cleared flag 189 where flag 188 was meant (rule 16 already clears
189), so once the plant had grown twice rule 17 ran every turn and swapped
the empty bottle and the bottle of water on every move.

Rule 64 (LOCK at the iron door with the small key) had action 210 (swap two
rooms' descriptions) where 202 (set flag 192) was meant: it swapped rooms 192
and 210, which do not exist, and the door stayed open.  Now it sets flag 192,
which rule 7 answers by shutting the door, as rule 63 sets 193 to open it; the
describe action (213) goes too, since rule 7 describes the room itself.
"""
import os
import sys

PORT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(PORT, '..', 'src_original', 'CAVE-FUN-MJ.ADV')
OUT = os.path.join(PORT, 'data', 'CAVE-FUN-MJ.ADV')

FIXES = {
    # rule number: (as it is, as it should be)
    18: ('  0,100,  3,189,  0,189,  0,  0,  0,  0,  0,  0,203,  0,  0,  0',
         '  0,100,  3,188,  0,188,  0,  0,  0,  0,  0,  0,203,  0,  0,  0'),
    64: (' 16,  0,  1, 62,  3,  4,  2, 12,  0,  4,  0,192,203,210, 11,213',
         ' 16,  0,  1, 62,  3,  4,  2, 12,  0,  4,  0,192,203,202, 11,  0'),
}


def parity(b):
    return (b & 0x7F) | (0x80 if bin(b & 0x7F).count('1') % 2 else 0)


def main():
    data = open(SRC, 'rb').read()
    lines = data.split(bytes((parity(13), parity(10))))
    text = [bytes(b & 0x7F for b in l).decode('ascii') for l in lines]
    header = [int(x) for x in text[0].replace(' ', '').split(',')]
    rules = header[5]
    first = len(text) - 1 - rules            # the last element is what follows the last CR LF
    for n, (old, new) in FIXES.items():
        i = first + n - 1
        if text[i] != old:
            sys.exit('rule %d is not what was expected: %r' % (n, text[i]))
        lines[i] = bytes(parity(ord(c)) for c in new)
    out = bytes((parity(13), parity(10))).join(lines)
    open(OUT, 'wb').write(out)
    print('%s: %d bytes, rules %s fixed' % (OUT, len(out), ', '.join(map(str, FIXES))))


if __name__ == '__main__':
    main()
