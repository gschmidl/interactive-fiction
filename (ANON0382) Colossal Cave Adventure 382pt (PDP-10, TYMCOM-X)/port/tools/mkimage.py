#!/usr/bin/env python3
r"""Build src/image.c: the TOPS-10 core image of Tymshare's 382-point ADVENTURE.

Source
------
    raw\advent-upl17.sav     = tapes\upl\advent.sav.17        1978-11-22
    raw\advent-upl20.sav     = tapes\upl\advent.sav.20        1979-10-17
    raw\advent-games.sav     = tapes\games\advent.sav         1981-02-25
    raw\advent-pointerc.sav  = tapes\pointerc\madven.sav      1985-02-20
    raw\advent-carl.sav      = tapes\carl\advent.sav          1988-01-27

all five from the Tymshare (TYMCOM-X) tape collection.  They hold two
distinct compilations of one game -- Don Woods' FORTRAN ADVENTURE as
Tymshare extended it to 382 points:

    1978   upl17                                  one copy
    1979   upl20, games, pointerc, carl           four copies of one build

Within the 1979 group the four copies are the same program, and the only
deliberate difference anywhere is two instructions in `games` (see "The
KI-10 patch" below).

Word format
-----------
Each 36-bit PDP-10 word occupies 5 bytes.  Byte k (k=0..4) holds one
7-bit septet right-justified; the word's spare 36th bit is the high bit
of byte 4.  This is the "5 septets per word" ANSI-ASCII tape mode -- the
same encoding as the EXPLOR and CRYSTAL CAVE tapes, and the reason a
naive 8-bit dump of these files looks like noise.

File format
-----------
TOPS-10 .SAV: a chain of IOWD blocks <-count,,addr-1>, each followed by
<count> data words, ending with a JRST to the start address, then one
trailing pad word.  (The pad word is uninitialised: it differs in every
one of the five copies and is discarded.)

The repair
----------
Every one of the five files is exactly one 36-bit word short, and the
word is always missing from the same two places -- file word 1850 or
file word 3674.  That is the identical defect the EXPLOR and CRYSTAL
CAVE tapes carry, at the identical offsets, so it is an artifact of how
the collection was transferred and not something about any one game.

    lost file word 1850:   advent-games.sav, advent-pointerc.sav
    lost file word 3674:   advent-upl20.sav, advent-carl.sav,
                           advent-upl17.sav

Which file lost which is not assumed.  Two copies that lost the *same*
word agree everywhere; two that lost *different* words agree outside
[1850, 3674) and, inside it, the early-loser reads exactly one word
ahead of the late-loser.  That shift window is checked word for word
below, and it is what fixes the two offsets and tells the two groups
apart.

1979: reconciled
    pointerc lost 1850, upl20 lost 3674, and each supplies what the
    other is missing.  Rebuilding from either direction gives the same
    26793-word sequence, and deleting the restored word gives each
    original back.  That round trip is the check.

    games and carl reconstruct to the same thing, so the restored words
    are confirmed by a second, independent donor each:

        word at 1850 = 325000003621   from upl20 and again from carl
        word at 3674 = 321100007276   from pointerc and again from games

1978: derived
    upl17 is the only copy of its build, so there is no donor.  It lost
    the word at 3674 -- inserting a placeholder at 1850 instead produces
    an image that prints the first room and then stops dead, while 3674
    plays.  The hole lands at address 007256, in the middle of this:

        007244  MOVEI 3,107        007250  MOVEI 4,101
        007245  CAME  3,014627     007251  CAME  4,014627
        007246  TDZA  3,3          007252  TDZA  4,4
        007247  SETO  3,0          007253  SETO  4,0

        007254  MOVEI 5,76
        007255  CAME  5,014627
        007256  <missing>
        007257  SETO  5,0

    so the missing word is TDZA 5,5 = 634240000005.  The 1979 build has
    the same routine five words lower, and its 007251 -- a word proven
    by donor reconciliation, not inferred -- is exactly 634240000005.
    Two independent lines of evidence, no borrowing of anything the
    file did not already imply.

The KI-10 patch
---------------
FOROTS opens with the standard processor test:

        SETO  0,            ; AC0 := -1
        AOBJN 0,.+1         ; KA-10 carries between halves, KI-10 does not
        JUMPE 0,ok          ; zero => KI-10
        OUTSTR "?KI-10 CODE WILL NOT RUN ON A KA-10"
        EXIT

and stores the same answer as a flag for the floating-point routines.
In advent-games.sav -- and in none of the other four -- both AOBJN
instructions are replaced by SETZ 0,, which hard-wires the answer to
"KI-10" and skips the test.  Someone patched that copy by hand.

It makes no difference here: the emulator increments the two halves
independently, as a KI-10 does, so all five images take the same path.
BUILD=1981 is offered because the patch is real history, not because it
plays differently.

The entry point is 014002 for the 1979 build, 014007 for 1978.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
RAW = os.path.join(HERE, '..', 'raw')
OUT = os.path.join(HERE, '..', 'src', 'image.c')
WMASK = (1 << 36) - 1

# The two places a word ever went missing.  Established from the 1979
# copies, where the shift window pins them exactly, and the same two
# offsets the EXPLOR and CRYSTAL CAVE tapes lost words at.
DROP_EARLY, DROP_LATE = 1850, 3674

# upl17 is the only copy of its build.  Its hole falls at address 007256
# inside a MOVEI/CAME/TDZA/SETO quartet whose AC-3 and AC-4 copies sit
# right above it, and the 1979 build carries the identical instruction at
# the matching spot.  See "The repair" above.
UPL17_LOST = 0o634240000005

#   name: (kind, early-loser, late-loser, start address)
# 'pair'    -- two copies of one build; the splice reconciles word for word
# 'derived' -- one copy; the missing word is supplied from UPL17_LOST
BUILDS = {
    '1979': ('pair', 'advent-pointerc.sav', 'advent-upl20.sav', 0o14002),
    '1981': ('pair', 'advent-games.sav',    'advent-upl20.sav', 0o14002),
    '1978': ('derived', None, 'advent-upl17.sav', 0o14007),
}

# carl is not a build.  It lost the same word upl20 did and is otherwise
# the same program, but one 512-word tape record inside it was destroyed
# in transfer, so it can only corroborate -- which --verify makes it do.
CROSSCHECK = [
    ('advent-pointerc.sav', 'advent-upl20.sav'),
    ('advent-pointerc.sav', 'advent-carl.sav'),
    ('advent-games.sav',    'advent-upl20.sav'),
    ('advent-games.sav',    'advent-carl.sav'),
]

# Read back out of the assembled image as a check.  A one-word
# misalignment shifts every packed character by 7 bits and turns the
# prose to noise, so this is the sharpest check available on a program
# whose database is compiled into the image.
ROOM1 = ('YOU ARE STANDING AT THE END OF A ROAD BEFORE A SMALL BRICK '
         'BUILDING.')
# The three things that make this the Tymshare 382 and not Woods' 350.
EXTRAS = ('THERE IS A 50 FOOT COIL OF ROPE HERE',
          'A RING OF ADAMANT',
          'NEAR YOU IS A SMALL MAIL COAT MADE OF MITHRIL')
# The class-rating ladder, verbatim from the 350-point game: Tymshare
# added treasures without touching it.
CVAL = [35, 100, 130, 200, 250, 300, 330, 349, 9999]


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


def splice(early, late):
    """Restore both dropped words.  `early` lost file word p1, `late` lost
    file word p2 (p1 < p2); each supplies what the other is missing.

    Write T for the intact image.  Then

        early[i] = T[i]    for i < p1,   T[i+1] for i >= p1
        late[i]  = T[i]    for i < p2,   T[i+1] for i >= p2

    so the two agree outside [p1, p2), and inside it early[i] == late[i+1]
    -- one file reading exactly one word ahead of the other.  That shift
    is the whole signature of the defect, and checking it word for word is
    what tells an early-loser from a late-loser.

    Each file's missing word is therefore sitting in the other:
    T[p1] = late[p1], and T[p2] = early[p2-1].

    Reconstructing from either direction must give the same sequence, and
    deleting p1 or p2 from it must give back `early` and `late`
    respectively.  That round trip is the real check.

    Two of the 1979 copies carry genuine content differences besides the
    defect -- the KI-10 patch in games, a destroyed tape record in carl --
    so the reconstructions are allowed to disagree outside the repair, and
    the caller is told where.  Inside the shift window they may not.
    """
    p1, p2 = DROP_EARLY, DROP_LATE
    if len(early) != len(late):
        sys.exit('copies differ in length: %d vs %d' % (len(early), len(late)))
    bad = [i for i in range(p1, p2 - 1) if early[i] != late[i + 1]]
    if bad:
        sys.exit('the shift window breaks at file word %d -- these two did '
                 'not lose words at %d and %d' % (bad[0], p1, p2))
    from_early = early[:p1] + [late[p1]] + early[p1:]
    from_late = late[:p2] + [early[p2 - 1]] + late[p2:]
    if from_early[:p1] + from_early[p1 + 1:] != early:
        sys.exit('deleting word %d does not give back the early copy' % p1)
    if from_late[:p2] + from_late[p2 + 1:] != late:
        sys.exit('deleting word %d does not give back the late copy' % p2)
    # The trailing pad word sits after the JRST and is uninitialised, so
    # it is expected to differ and is not counted.
    elsewhere = [i for i in range(len(from_early) - 1)
                 if from_early[i] != from_late[i]]
    return from_early, elsewhere


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


def check(mem):
    """Room 1, the three Tymshare treasures, and the rating ladder must all
    read correctly out of the assembled image."""
    lo, hi = min(mem), max(mem)
    flat = ''.join(a5(mem.get(a, 0)) for a in range(lo, hi + 1))
    if ROOM1 not in flat:
        sys.exit('room 1 does not decode -- the image is misaligned')
    for s in EXTRAS:
        if s not in flat:
            sys.exit('%r is missing -- this is not the 382-point game' % s)
    run = [i for i in range(lo, hi - len(CVAL))
           if all(mem.get(i + k) == CVAL[k] for k in range(len(CVAL)))]
    if not run:
        sys.exit('the class-rating table is not where it should be')
    return run[0]


def runs(idx):
    """Collapse a sorted index list into printable ranges."""
    if not idx:
        return 'nowhere'
    out, s, p = [], idx[0], idx[0]
    for i in idx[1:]:
        if i != p + 1:
            out.append((s, p))
            s = i
        p = i
    out.append((s, p))
    return ', '.join('%d' % a if a == b else '%d..%d' % (a, b) for a, b in out)


def verify():
    """Cross-check every 1979 copy against every other one, and say what
    each pair proves.  Nothing here feeds the build; it is the evidence."""
    W = {f: words(os.path.join(RAW, f))
         for f in ('advent-upl20.sav', 'advent-games.sav',
                   'advent-pointerc.sav', 'advent-carl.sav')}
    print('the 1979 build -- four copies, %d words each\n'
          % len(W['advent-upl20.sav']))
    seen = {}
    for f_early, f_late in CROSSCHECK:
        img, elsewhere = splice(W[f_early], W[f_late])
        e, l = img[DROP_EARLY], img[DROP_LATE]
        seen.setdefault(DROP_EARLY, {})[f_late] = e
        seen.setdefault(DROP_LATE, {})[f_early] = l
        print('  %-21s (lost 1850) + %-19s (lost 3674)'
              % (f_early, f_late))
        print('      shift window [1850,3674) holds word for word; '
              'both round trips pass')
        print('      %012o back into the first, %012o back into the second'
              % (e, l))
        print('      the two reconstructions then differ at: %s'
              % runs(elsewhere))
    print()
    for at in (DROP_EARLY, DROP_LATE):
        vals = set(seen[at].values())
        print('  word %d restored as %s from %s -- %s'
              % (at, ', '.join('%012o' % w for w in sorted(vals)),
                 ' and '.join(sorted(seen[at])),
                 'the two donors agree' if len(vals) == 1
                 else 'THE DONORS DISAGREE'))
        if len(vals) != 1:
            sys.exit(1)
    print('\n  (games differs by two instructions -- the KI-10 patch; carl by\n'
          '   one destroyed 512-word tape record.  Neither is the defect.)')


def main():
    which = '1979'
    for a in sys.argv[1:]:
        if a.startswith('--build='):
            which = a.split('=', 1)[1]
        elif a == '--verify':
            verify()
            return
        else:
            sys.exit('usage: mkimage.py [--build=%s] [--verify]'
                     % '|'.join(sorted(BUILDS)))
    if which not in BUILDS:
        sys.exit('unknown build %s (want one of %s)'
                 % (which, ', '.join(sorted(BUILDS))))
    kind, f_early, f_late, want_start = BUILDS[which]
    late = words(os.path.join(RAW, f_late))

    if kind == 'pair':
        early = words(os.path.join(RAW, f_early))
        img, elsewhere = splice(early, late)
        restored = [(DROP_EARLY, img[DROP_EARLY], f_late),
                    (DROP_LATE, img[DROP_LATE], f_early)]
    else:
        img = late[:DROP_LATE] + [UPL17_LOST] + late[DROP_LATE:]
        if img[:DROP_LATE] + img[DROP_LATE + 1:] != late:
            sys.exit('the insertion does not round-trip')
        elsewhere = []
        restored = [(DROP_LATE, UPL17_LOST, 'derived, see tools/mkimage.py')]

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
    cval = check(mem)

    print('build %s (%s)' % (which, kind))
    for at, w, src in restored:
        print('  restored %012o at file word %-5d from %s' % (w, at, src))
    if elsewhere:
        print('  the two copies also differ at %s -- content, not the defect'
              % runs(elsewhere))
    print('image: %d blocks, %d words, %06o..%06o, start %06o, 1 pad word'
          % (len(blocks), total, lo, hi, start))
    print('checked: room 1 decodes; rope, ring and mail coat all present;')
    print('         rating ladder %s at %06o' % (CVAL[:4] + ['...'], cval))

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
