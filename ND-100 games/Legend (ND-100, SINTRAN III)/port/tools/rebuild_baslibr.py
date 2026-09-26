"""Rebuild the lost sector of BASLIBR-H00:BRF, the ND BASIC runtime library.

usage: python tools\\rebuild_baslibr.py [OUT]   (default build\\BASLIBR-H00.BRF)

Both images of the HUMBUG floppy (ND-disk-00437 and -00437c) hold the only
known copy of BASLIBR-H00:BRF, and in both the 512 bytes at file offset
2048-2559 are unreadable (garbage, then zeros).  SINTRAN's loader stops
there with ILLEGAL BRF-CONTROL NO.  What those bytes held is recovered here:

  * BASIC:PROG, the ND BASIC compiler of January 1985 (LUNDIN-8, and the same
    file on the SINTRAN pack), has the runtime library linked into it in the
    library's own order, so the lost units' words are there;
  * NO-GA/BACKGAMMON:PROG on the SINTRAN pack is a BASIC program linked with
    the same library, at another address, which shows which of those words
    are relocated and which areas were only reserved;
  * the record layout follows the units around the hole (tools\\brf.py).

The lost sector held the end of unit 7INIT, the units 7EXCB, 7DBUG, 7PBAS
(one word each), 7GOP (the GOSUB stack) and 7FNP (the FN parameter stack),
and the start of the unit 7IOIN/7INB/7OUTB.  Two checks confirm it: the
records come to exactly 512 bytes, and the checksum of the 7IOIN unit, whose
END survived after the hole, comes out right.  The order of the three LBR
names in that unit cannot be recovered (the checksum does not depend on it);
they are written in address order.
"""
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from brf import checksum, code_of, encode, parse, units  # noqa: E402
from place_units import image  # noqa: E402

WORK = os.path.join(os.environ.get('ND100_WORK',
    os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', '_ND100_work')), 'files', 'basic')
HOLE = (2048, 2560)


def N(s):
    return list(code_of(s))


def rec(c, ws=()):
    return (None, c, list(ws))


def lnf(ws):
    return rec(0x14, [len(ws)] + list(ws))


def one_word(name, value):
    return [rec(0x0f), rec(0x0d, N(name)), rec(0x0e, N(name)), lnf([value]), rec(0x11, [0])]


def stack(name, size):
    return [rec(0x0f), rec(0x0d, N(name)), rec(0x0e, N(name)), rec(0x01, [1]), rec(0x0a, [size + 1]),
            rec(0x11, [0])]


def with_checksum(unit):
    unit[-1] = rec(0x11, [checksum(unit)])
    return unit


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(HERE), 'build', 'BASLIBR-H00.BRF')
    lib = open(os.path.join(WORK, 'BASLIBR-H00.BRF'), 'rb').read()
    bas, _ = image(os.path.join(WORK, 'BASIC.PROG'))

    # unit 7INIT: its records before the hole, the rest of its 61-word block,
    # a reference to 77ERR and 129 zero words (a buffer)
    before, stop = parse(lib, 1661, 1951)
    assert stop == 1951
    block = struct.unpack('>61H', lib[1954:2048] + b'\0\0' * 14)[:47] + tuple(bas[a] for a in range(0o43551, 0o43567))
    init = before + [lnf(block), rec(0x10, N('77ERR')), lnf([0] * 129), rec(0x11, [0])]
    init = with_checksum(init)
    units_lost = [init]
    for name in ('7EXCB', '7DBUG', '7PBAS'):
        units_lost.append(with_checksum(one_word(name, 0)))
    units_lost.append(with_checksum(stack('7GOP', 0o21)))
    units_lost.append(with_checksum(stack('7FNP', 0o202)))

    # unit 7IOIN: rebuilt head, then the records that survived after the hole
    head = [rec(0x0f), rec(0x0d, N('7IOIN')), rec(0x0d, N('7INB')), rec(0x0d, N('7OUTB')),
            rec(0x0e, N('7IOIN')), lnf([bas[a] for a in range(0o44214, 0o44254)]),
            rec(0x10, N('7CONO')), rec(0x10, N('7TABP')), rec(0x10, N('7RANT')),
            rec(0x0e, N('7INB')), lnf([bas[a] for a in range(0o44257, 0o44267)])]
    tail, stop = parse(lib, 2569, 2818)
    assert stop == 2818 and tail[-1][1] == 0x11
    ioin = head + tail
    survived = tail[-1][2][0]
    assert checksum(ioin) == survived, 'the 7IOIN unit does not add up: %04x, not %04x' % (checksum(ioin), survived)

    # the bytes: everything from the start of unit 7INIT to the end of 7IOIN
    new = b''.join(encode(u) for u in units_lost) + encode(ioin)
    old_span = lib[1661:2818]
    # the rebuilt bytes must agree with every byte that survived
    assert new[:2048 - 1661] == old_span[:2048 - 1661]
    assert new[-(2818 - 2560):] == old_span[-(2818 - 2560):]
    assert len(new) == len(old_span), (len(new), len(old_span))
    fixed = lib[:1661] + new + lib[2818:]
    os.makedirs(os.path.dirname(out), exist_ok=True)
    open(out, 'wb').write(fixed)

    r, stop = parse(fixed, 64)
    bad = [u for u in units(r) if checksum(u) != u[-1][2][0]]
    print('%s: %d bytes, %d units, all checksums %s; parsed to %d' % (
        out, len(fixed), len(units(r)), 'right' if not bad else 'WRONG', stop))
    changed = [i for i in range(len(lib)) if lib[i] != fixed[i]]
    print('bytes changed: %d, all within %d-%d' % (len(changed), min(changed), max(changed)))


if __name__ == '__main__':
    main()
