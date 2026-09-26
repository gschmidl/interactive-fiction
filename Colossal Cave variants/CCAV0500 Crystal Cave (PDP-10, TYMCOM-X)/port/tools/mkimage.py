#!/usr/bin/env python3
r"""Build src/image.c: the TOPS-10 core image of CRYSTAL CAVE.

Source
------
    raw\cave-upl20.sav   = tapes\upl\cave.sav.20     1979-10-04
    raw\cave-upl22.sav   = tapes\upl\cave.sav.22     1979-10-17
    raw\cave-games.sav   = tapes\games\cave.sav      1983-03-29
    raw\cave-mpl.sav     = tapes\mpl\cave.sav        1984-12-27

all four from the Tymshare (TYMCOM-X) tape collection.  They hold three
distinct builds of one game:

    1979   upl20 and upl22 are the same build, byte for byte
    1983   games -- a rebuild; addresses shift throughout, and one
           message ("YOU ARE DEAD!") moves four lines earlier
    1984   mpl   -- a rebuild of the 1983 content, 433 words larger

The game text is otherwise identical across all three, so what the
player sees is the 1979 game whichever you build.  1979 is the default
because it is the earliest and the only one whose repair is exact.

Word format
-----------
Each 36-bit PDP-10 word occupies 5 bytes.  Byte k (k=0..4) holds one
7-bit septet right-justified; the word's spare 36th bit is the high bit
of byte 4.  This is the "5 septets per word" ANSI-ASCII tape mode -- the
same encoding as the EXPLOR tapes, and the reason a naive 8-bit dump of
these files looks like noise.

File format
-----------
TOPS-10 .SAV: a chain of IOWD blocks <-count,,addr-1>, each followed by
<count> data words, ending with a JRST to the start address, then one
trailing pad word.

The repair
----------
Every one of the four files is exactly one 36-bit word short, and the
word is always missing from the same two places -- file word 1850 or
file word 3674.  That is the identical defect the EXPLOR tapes carry, at
the identical offsets, so it is an artifact of how the collection was
transferred and not something about either game.

    cave-upl22.sav  lost the word at file offset 1850
    cave-upl20.sav  lost the word at file offset 3674
    cave-games.sav  lost the word at file offset 1850
    cave-mpl.sav    lost the word at file offset 3674

Each pair therefore repairs the other.

Evidence, in the order it was established:

  * No file parses as a .SAV on its own.  Each loses sync inside the big
    block and never reaches a terminator.

  * upl20 and upl22 differ in exactly one window, file words 1850..3673,
    and inside it every word of upl22 equals the *next* word of upl20,
    all 1823 of them.  That is the signature of one dropped word on each
    side, at the two ends of the window -- nothing else produces it.

  * Splicing each file's missing word back in from the other yields the
    same 29575-word sequence from either direction, and that sequence
    parses cleanly: 399 blocks landing on a "JRST 014300" terminator
    with exactly one pad word to spare.

  * The repaired image reads correctly where the unrepaired one does
    not: the message text decodes to running prose from the first word,
    and the object mnemonic table reads KEYS, LAMP, SEARS, ... in object
    order.  Both are checked below rather than merely asserted.

games and mpl are two different builds, so their cross-splice cannot be
validated by reconciliation the way the 1979 pair can -- only by the
fact that each result parses as a clean .SAV chain terminating exactly
on its JRST.  That is strong, but it is not proof, and the tool says so.

The entry point is 014300 for the 1979 build (014271 for the other two).
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
RAW = os.path.join(HERE, '..', 'raw')
OUT = os.path.join(HERE, '..', 'src', 'image.c')
WMASK = (1 << 36) - 1

# The two places a word ever went missing.  Established from the 1979 pair,
# where the shifted window pins them exactly, and confirmed by aligning
# games against mpl: the same two offsets, in a different pair of files, of
# a different game -- which is why this is a property of the transfer and
# not of any one image.
DROP_EARLY, DROP_LATE = 1850, 3674

# 1979: two copies of one build, so the splice reconciles word for word and
#       the offsets are re-derived from the files rather than assumed.
#       ('exact')
# 1983 and 1984: two *different* builds, each repaired from the other.  They
#       differ in length, so there is no window to derive offsets from; the
#       offsets above are used, and the only validation is that the result
#       parses as a clean .SAV chain.  ('cross')
#
#   (kind, target file, donor file, offset the target lost, start address)
# The 'exact' entry re-derives its offsets from the files and only uses the
# constants above as an assertion, so its offset field is None.
BUILDS = {
    '1979': ('exact', 'cave-upl22.sav', 'cave-upl20.sav', None,       0o14300),
    '1983': ('cross', 'cave-games.sav', 'cave-mpl.sav',   DROP_EARLY, 0o14271),
    '1984': ('cross', 'cave-mpl.sav',   'cave-games.sav', DROP_LATE,  0o14271),
}

# The first long-form room description, and the object mnemonics in object
# order.  Both are read back out of the assembled image as a check.
ROOM1 = 'YOU ARE STANDING AT THE END OF A ROAD BEFORE A BARN.'
OBJECTS = ('KEYS LAMP SEARS RICK WALLE BRIDG BOAT DAM DOOR GATE KEG KNIFE '
           'FOOD BOTTL WATER WINE AXE SPICE COLUM COLA SHOWE VENDI CRAP '
           'BATTE ROPE TOMB TOAD SAND MIRRO ORCS DWARF BEAR SKELE SPIDE '
           'DRAGO DJIN COBOL GIANT ARARI').split()


def words(path):
    """Unpack a 5-septets-per-word tape file into 36-bit words."""
    data = open(path, 'rb').read()
    if len(data) % 5:
        sys.exit('%s: length %d is not a multiple of 5' % (path, len(data)))
    out = []
    for i in range(0, len(data), 5):
        b = data[i:i + 5]
        w = 0
        for j in range(5):
            w = (w << 7) | (b[j] & 0x7f)
        out.append(((w << 1) | ((b[4] >> 7) & 1)) & WMASK)
    return out


def a5(x):
    """The five 7-bit characters packed in one word, left justified."""
    return ''.join(chr((x >> (29 - 7 * k)) & 0x7f) for k in range(5))


def splice_cross(target, donor, lost):
    """Repair `target` from a donor that is a *different build*.

    Only usable because the defect always strikes at DROP_EARLY or
    DROP_LATE, and the two builds still agree word for word that early in
    the file.  Nothing here can be reconciled the way the 1979 pair can;
    the check is that the result parses as a clean .SAV chain, which the
    caller does.
    """
    if lost == DROP_EARLY:
        return target[:lost] + [donor[lost]] + target[lost:], donor[lost]
    return target[:lost] + [donor[lost - 1]] + target[lost:], donor[lost - 1]


def splice(early, late):
    """Restore both dropped words.  `early` lost file word p1, `late` lost
    file word p2 (p1 < p2); each supplies what the other is missing.

    Write T for the intact image.  Then

        early[i] = T[i]    for i < p1,   T[i+1] for i >= p1
        late[i]  = T[i]    for i < p2,   T[i+1] for i >= p2

    so the two agree outside [p1, p2), and inside it early[i] == late[i+1]
    -- one file reading exactly one word ahead of the other.  That shift
    is the whole signature of the defect, and it fixes p1 and p2 as the
    first and last differing offsets.

    Each file's missing word is therefore sitting in the other:
    T[p1] = late[p1], and T[p2] = early[p2-1].

    Reconstructing from either direction must give the same sequence, and
    deleting p1 or p2 from it must give back `early` and `late`
    respectively.  That round trip is the real check -- checking only
    that the window differs would prove nothing, since a few words inside
    it match by coincidence.
    """
    if len(early) != len(late):
        sys.exit('copies differ in length: %d vs %d' % (len(early), len(late)))
    diff = [i for i in range(len(early)) if early[i] != late[i]]
    if not diff:
        sys.exit('the two copies are identical -- nothing to repair')
    p1, p2 = diff[0], diff[-1] + 1
    if (p1, p2) != (DROP_EARLY, DROP_LATE):
        sys.exit('dropped words are at %d and %d, not the expected %d and %d'
                 % (p1, p2, DROP_EARLY, DROP_LATE))
    from_early = early[:p1] + [late[p1]] + early[p1:]
    from_late = late[:p2] + [early[p2 - 1]] + late[p2:]
    if from_early != from_late:
        sys.exit('the two reconstructions disagree at word %d -- these are '
                 'not copies of one build'
                 % next(i for i in range(len(from_early))
                        if from_early[i] != from_late[i]))
    T = from_early
    if T[:p1] + T[p1 + 1:] != early:
        sys.exit('deleting word %d does not give back the first copy' % p1)
    if T[:p2] + T[p2 + 1:] != late:
        sys.exit('deleting word %d does not give back the second copy' % p2)
    return T, (p1, late[p1]), (p2, early[p2 - 1])


def loadsav(w):
    """Walk the IOWD block chain.  Returns (blocks, start, trailing)."""
    blocks, i = [], 0
    while i < len(w):
        x = w[i]
        lh = (x >> 18) & 0o777777
        if lh < 0o400000:                          # positive left half
            if (x >> 27) != 0o254:
                sys.exit('terminator at file word %d is not a JRST: %012o'
                         % (i, x))
            return blocks, x & 0o777777, len(w) - i - 1
        n = 0o1000000 - lh
        a = (x & 0o777777) + 1
        if i + 1 + n > len(w):
            sys.exit('block at file word %d runs off the end' % i)
        blocks.append((a, w[i + 1:i + 1 + n]))
        i += 1 + n
    sys.exit('block chain has no terminator')


def check_text(mem):
    """The room text and the object table must both read correctly.  A
    one-word misalignment shifts every packed character by 7 bits and
    turns the prose to noise, so this is the sharpest check available on
    a program whose database is compiled into the image."""
    lo, hi = min(mem), max(mem)
    flat = ''.join(a5(mem.get(a, 0)) for a in range(lo, hi + 1))
    if ROOM1 not in flat:
        sys.exit('room 1 does not decode -- the image is misaligned')
    # The table stores each object as one A5 name word followed by one
    # pointer word, so the names sit ten characters apart.  Require the
    # whole run in order, not just that the names appear somewhere.
    pad = [o.ljust(5) for o in OBJECTS]
    start = -1
    while True:
        start = flat.find(pad[0], start + 1)
        if start < 0:
            sys.exit('object table not found -- the image is misaligned')
        if all(flat[start + 10 * k:start + 10 * k + 5] == pad[k]
               for k in range(len(pad))):
            return len(OBJECTS)


def main():
    which = '1979'
    for a in sys.argv[1:]:
        if a.startswith('--build='):
            which = a.split('=', 1)[1]
        else:
            sys.exit('usage: mkimage.py [--build=1979|1983|1984]')
    if which not in BUILDS:
        sys.exit('unknown build %s (want one of %s)'
                 % (which, ', '.join(sorted(BUILDS))))
    kind, f_target, f_donor, lost_at, want_start = BUILDS[which]
    target = words(os.path.join(RAW, f_target))
    donor = words(os.path.join(RAW, f_donor))

    if kind == 'exact':
        img, lost_target, lost_donor = splice(target, donor)
        note = None
    else:
        img, w = splice_cross(target, donor, lost_at)
        lost_target, lost_donor = (lost_at, w), None
        note = ('cross-build splice from %s: validated only by the clean '
                '.SAV chain below' % f_donor)

    blocks, start, trailing = loadsav(img)
    total = sum(len(d) for _, d in blocks)
    lo = min(a for a, _ in blocks)
    hi = max(a + len(d) - 1 for a, d in blocks)
    if trailing != 1:
        sys.exit('expected 1 pad word after the terminator, got %d' % trailing)
    if start != want_start:
        sys.exit('unexpected start address %06o (wanted %06o)'
                 % (start, want_start))

    mem = {}
    for a, d in blocks:
        for k, w in enumerate(d):
            mem[a + k] = w
    nobj = check_text(mem)

    print('build %s' % which)
    print('  %-16s lost %012o at file word %d'
          % (f_target, lost_target[1], lost_target[0]))
    if lost_donor:
        print('  %-16s lost %012o at file word %d'
              % (f_donor, lost_donor[1], lost_donor[0]))
    if note:
        print('  (%s)' % note)
    print('image: %d blocks, %d words, %06o..%06o, start %06o, 1 pad word'
          % (len(blocks), total, lo, hi, start))
    print('checked: room 1 decodes, all %d object mnemonics in order' % nobj)

    with open(OUT, 'w') as f:
        f.write('/* Generated by tools/mkimage.py --build=%s -- do not edit. */\n'
                % which)
        f.write('#include "pdp10.h"\n\n')
        f.write('const int image_start = 0%o;\n' % start)
        f.write('const int image_hisegtop = 0;   /* single segment */\n\n')
        f.write('/* Each entry: address, count, then <count> words. '
                'Terminated by count 0. */\n')
        f.write('const w36 image_data[] = {\n')
        for a, data in blocks:
            f.write('/*%06o*/ 0%o, 0%o,\n' % (a, a, len(data)))
            for i in range(0, len(data), 6):
                f.write('  ' + ', '.join('0%o' % x for x in data[i:i + 6]) + ',\n')
        f.write('  0, 0\n};\n')
    print('wrote %s' % os.path.normpath(OUT))


if __name__ == '__main__':
    main()
