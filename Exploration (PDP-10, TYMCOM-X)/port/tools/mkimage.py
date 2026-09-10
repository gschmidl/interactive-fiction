#!/usr/bin/env python3
r"""Build src/image.c: the TOPS-10 core image of EXPLOR.

Source
------
    raw\explor-games.sav    = tapes\games\explor.sav    1983-03-29
    raw\explor-upl17.sav    = tapes\upl\explor.sav.17   1981-07-15

both from the Tymshare (TYMCOM-X) tape collection.  A third copy,
tapes\carl\explor.sav.1 / .4 (1983-09-18), is a later build with two
regions zero-filled -- 4095 words at 000124..010122 and 1024 words at
012123..014122 -- so it cannot be run.  It is still useful as an
independent witness to the layout, and it corroborates what follows.

Word format
-----------
Each 36-bit PDP-10 word occupies 5 bytes.  Byte k (k=0..4) holds one
7-bit septet right-justified; the word's spare 36th bit is the high bit
of byte 4.  This is the "5 septets per word" ANSI-ASCII tape mode.

File format
-----------
TOPS-10 .SAV: a chain of IOWD blocks <-count,,addr-1>, each followed by
<count> data words, ending with a JRST to the start address, then one
trailing pad word.

The repair
----------
Both copies are the same build, and each lost exactly one 36-bit word in
transfer -- at different places, so they repair each other:

    explor-games.sav  lost  200100026110  MOVE 2,26110  at address 003616
    explor-upl17.sav  lost  474100000000  SETO 2,0      at address 007256

Evidence, in the order it was established:

  * Neither file parses as a .SAV on its own.  Block 8 -- the big one,
    000642..016323 -- has a header claiming 6962 data words but only
    6961 follow it, so the chain loses sync and never reaches a
    terminator.  Each file is one word short, and short inside block 8.

  * Outside one window the two files are word-identical.  Inside it --
    file words 1850..3673, which is block 8 addresses 003616..007255 --
    every word of the games copy equals the *next* word of the upl copy,
    all 1823 of them.  That is the signature of one dropped word on each
    side, at the two ends of the window.

  * Splicing each file's missing word back in yields the same 40320-word
    sequence from either direction, and that sequence parses cleanly:
    473 blocks, 39845 words into 000120..124213, landing exactly on the
    "JRST 016314" terminator with one pad word to spare, and block 8
    now holds the 6962 words its header always claimed.

  * The repaired code is self-consistent where the unrepaired code is
    not.  F40 compiles a FORMAT into a literal preceded by a JRST that
    jumps over it, and loads the literal's address with MOVEI 1,<the
    JRST>.  In the repaired image every one of the 38 format loads
    points at a JRST.  In the games copy alone, the 15 formats that live
    above 003616 point one word past their JRST, at the text itself,
    because the text sits one word low.

  * Likewise the constant pool at the end of block 8.  Repaired, it
    reads 8, 2, 1, 100, "EXPLOR    " and the reference counts are 3, 12,
    96, 2, 1 -- the constant 1 being the most-used constant in the
    program, as you would expect.  Unrepaired, everything shifts by one
    and the program appears to reference the constant 100 ninety-six
    times and never reference 8 at all.

The entry point is 016314, which in the repaired image is an 015 LUUO
followed by JRST 140.  That LUUO is genuine: F40 calls its object time
system through LUUOs, and cpu_reset installs the trap for them out of
JOBDAT .JBS41 the way the TOPS-10 loader does.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
RAW = os.path.join(HERE, '..', 'raw')
OUT = os.path.join(HERE, '..', 'src', 'image.c')
WMASK = (1 << 36) - 1


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


def repair(a, b):
    """Restore the word `a` lost, taking it from `b`, and check the result.

    `a` is the truth with one word deleted at p1, `b` the truth with one
    word deleted at p2 (p1 < p2).  Splicing b's word back into a gives
    the truth; b must then be exactly that with its own word removed,
    which is what the check below insists on -- every remaining word of
    b, to the end of the image, has to line up.  (A handful of words
    inside the window match by coincidence, so "the whole window
    differs" is not a usable test; this one is exact.)
    """
    if len(a) != len(b):
        sys.exit('copies differ in length: %d vs %d' % (len(a), len(b)))
    diff = [i for i in range(len(a) - 1) if a[i] != b[i]]
    if not diff:
        sys.exit('the two copies are identical -- nothing to repair')
    p1 = diff[0]
    fixed = a[:p1] + [b[p1]] + a[p1:]
    p2 = next((i for i in range(len(b) - 1) if b[i] != fixed[i]), None)
    if p2 is None:
        sys.exit('second copy lost nothing -- unexpected')
    bad = [i for i in range(p2, len(b) - 1) if b[i] != fixed[i + 1]]
    if bad:
        sys.exit('repair does not reconcile the copies: %d mismatches from %d'
                 % (len(bad), bad[0]))
    return fixed, (p1, b[p1]), (p2, fixed[p2])


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


def check_formats(mem):
    """Every F40 FORMAT load should point at the JRST that jumps the
    literal.  This is the sharpest check that the splice went in at the
    right place, so it is enforced rather than merely reported."""
    def a5(x):
        return ''.join(chr((x >> (29 - 7 * k)) & 0x7f) for k in range(5))
    good = bad = 0
    for a in sorted(mem):
        x = mem[a]
        if ((x >> 27) & 0o777) != 0o201 or ((x >> 23) & 0o17) != 1:
            continue                                # not MOVEI 1,X
        if ((mem.get(a + 1, 0) >> 27) & 0o777) != 0o17:
            continue                                # not followed by LUUO 017
        w = mem.get(x & 0o777777, 0)
        if ((w >> 27) & 0o777) == 0o254:
            good += 1
        elif all(32 <= ord(c) < 127 for c in a5(w)):
            bad += 1
    if bad:
        sys.exit('%d FORMAT loads point at literal text instead of its JRST '
                 '-- the image is misaligned' % bad)
    return good


def main():
    g = words(os.path.join(RAW, 'explor-games.sav'))
    u = words(os.path.join(RAW, 'explor-upl17.sav'))
    img, lost_g, lost_u = repair(g, u)

    blocks, start, trailing = loadsav(img)
    total = sum(len(d) for _, d in blocks)
    lo = min(a for a, _ in blocks)
    hi = max(a + len(d) - 1 for a, d in blocks)
    if trailing != 1:
        sys.exit('expected 1 pad word after the terminator, got %d' % trailing)
    if start != 0o16314:
        sys.exit('unexpected start address %o' % start)

    mem = {}
    for a, d in blocks:
        for k, w in enumerate(d):
            mem[a + k] = w
    nfmt = check_formats(mem)

    b8 = blocks[8]
    print('games/explor.sav  lost %012o at address %06o'
          % (lost_g[1], b8[0] + lost_g[0] - 334))
    print('upl/explor.sav.17 lost %012o at address %06o'
          % (lost_u[1], b8[0] + lost_u[0] - 334))
    print('image: %d blocks, %d words, %06o..%06o, start %06o'
          % (len(blocks), total, lo, hi, start))
    print('checked: %d FORMAT loads all point at their JRST' % nfmt)

    with open(OUT, 'w') as f:
        f.write('/* Generated by tools/mkimage.py -- do not edit. */\n')
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
