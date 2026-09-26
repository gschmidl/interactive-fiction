#!/usr/bin/env python3
"""Embed a set of original .SHR tape files into a C file.

    python tools/mkimages.py 84      -> src/images_84.c
    python tools/mkimages.py 87      -> src/images_87.c

The tape files hold PDP-10 words as five 7-bit septets per five bytes, with
the word's 36th bit in the high bit of the fifth.  Nothing is decoded here:
the bytes go into the C file verbatim and load.c unpacks them, so what the
program runs is bit-for-bit what came off the tape.

The one thing this tool computes is the *hole*.  Every TBA-compiled .SHR
file on the Tymshare tape -- all 211 of them, not just this game's -- is
one word short at a fixed point inside the TBA runtime, at a place that
depends only on which build of the runtime was linked in.  Load the file
flat at 400000 and the code below that point resolves perfectly while
everything above it is displaced by one, which shows up as an exact,
mechanical test:

    a routine that ends
        SUB  17,X
        POPJ 17,
    pushed some fixed number of words, so X must hold an "XWD n,n"
    literal.

In ventur.shr 46 of the 71 such epilogues land on one, and the other 25
land one word past it.  Insert a single word at the right index and all 71
land -- and every image in both sets here reaches 100%.  The hole always
falls immediately after a POPJ that ends a routine, at a module boundary,
and no instruction refers to the address it occupies, so a zero word
restores a working image.

This tool finds that index from each file rather than trusting a constant,
and refuses to emit an image it could not make consistent.
"""
import os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, '..', '..')

# Each set is a directory of tape files, the banner the port prints, and
# the programs it offers.  The order is the order they appear in -h, and
# the first is the default.
SETS = {
    '84': dict(
        dump='dump_original',
        banner=('LANDS OF ZARAST, by "Sauron the Feared"\\n'
                '          Tymshare TYMCOM-X, December 1984'),
        progs=[
            ('dungeon',   'dungen', 'the dungeon crawl -- the game itself'),
            ('overworld', 'ventur', 'the older stand-alone adventure above ground'),
            ('newchar',   'charc',  'roll up a character (the quick version)'),
            ('character', 'charac', 'roll up a character (the long version)'),
            ('filer',     'filer',  '(re)build the world file, NEWADV.DAT'),
        ]),
    '87': dict(
        dump='dump_1987',
        banner=('LANDS OF ZARAST, the later version\\n'
                '          Tymshare TYMCOM-X, 1987-88'),
        progs=[
            ('dungeon',   'pub',    'the dungeon crawl -- the game itself'),
            ('older',     'b',      'the May 1987 build of the same game'),
            ('newchar',   'cr',     'roll up a character'),
            ('filer',     'filer',  '(re)build the world file, NEWADV.DAT'),
        ]),
}


def words(data):
    return [((data[i]   & 0x7f) << 29) | ((data[i+1] & 0x7f) << 22)
          | ((data[i+2] & 0x7f) << 15) | ((data[i+3] & 0x7f) <<  8)
          | ((data[i+4] & 0x7f) <<  1) | ((data[i+4] >> 7) & 1)
            for i in range(0, len(data), 5)]


def is_nn(v):
    """An 'XWD n,n' stack-adjustment literal."""
    lh, rh = (v >> 18) & 0o777777, v & 0o777777
    return lh == rh and 0 < lh < 0o40


def epilogues(w):
    """Every 'SUB 17,X / POPJ 17,' in the image, as (address, X)."""
    out = []
    for i in range(len(w) - 1):
        x, nx = w[i], w[i+1]
        if (x >> 27) != 0o274 or ((x >> 23) & 0o17) != 0o17: continue
        if (nx >> 27) != 0o263 or ((nx >> 23) & 0o17) != 0o17: continue
        out.append((0o400000 + i, x & 0o777777))
    return out


def score(w):
    def M(a):
        i = a - 0o400000
        return w[i] if 0 <= i < len(w) else 0
    ok = off = 0
    for a, y in epilogues(w):
        if is_nn(M(y)): ok += 1
        elif is_nn(M(y - 1)): off += 1
    return ok, off


def find_hole(w):
    """Return the index a single zero word has to go in, or None."""
    ok, off = score(w)
    if off == 0:
        return None
    def M(a):
        i = a - 0o400000
        return w[i] if 0 <= i < len(w) else 0
    first = None
    for a, y in epilogues(w):
        if not is_nn(M(y)) and is_nn(M(y - 1)):
            first = y
            break
    if first is None:
        return None
    # The hole sits at a module boundary: walk back from the misresolved
    # literal to the POPJ that ends the routine before it.
    g = first - 0o400000 - 1
    while g > 0 and not ((w[g] >> 27) == 0o263 and ((w[g] >> 23) & 0o17) == 0o17):
        g -= 1
    return g + 1


def carray(name, data):
    out = ['static const unsigned char %s[] = {' % name]
    for i in range(0, len(data), 20):
        out.append('    ' + ','.join(str(b) for b in data[i:i+20]) + ',')
    out.append('};')
    return '\n'.join(out)


def find_file(setname, img):
    """Tape files for a set, falling back to the 1984 dump.

    The later version has no FILER of its own on the tape -- only the game
    and its character generator were kept in novafield -- so it borrows the
    one from mpl.  Same author, same world file, and the later game reads
    what that FILER writes.
    """
    tried = []
    for d in (SETS[setname]['dump'], 'dump_original'):
        p = os.path.join(ROOT, d, img + '.shr')
        tried.append(p)
        if os.path.exists(p):
            return p, d
    sys.exit('%s: no %s.shr in %s' % (setname, img, ' or '.join(tried)))


def main():
    if len(sys.argv) != 2 or sys.argv[1] not in SETS:
        sys.exit('usage: mkimages.py {%s}' % '|'.join(sorted(SETS)))
    name = sys.argv[1]
    spec = SETS[name]
    out = os.path.join(HERE, '..', 'src', 'images_%s.c' % name)

    parts = ['/* images_%s.c -- generated by tools/mkimages.py %s; do not edit. */'
             % (name, name),
             '#include "pdp10.h"', '']
    tab = []
    for cmd, img, what in spec['progs']:
        path, where = find_file(name, img)
        raw = open(path, 'rb').read()
        if len(raw) % 5:
            sys.exit('%s: not a whole number of PDP-10 words' % path)
        w = words(raw)
        g = find_hole(w)
        if g is None:
            sys.exit('%s: could not locate the runtime hole' % path)
        ok, off = score(w[:g] + [0] + w[g:])
        if off:
            sys.exit('%s: hole at %o still leaves %d bad epilogues'
                     % (path, g, off))
        print('  %-8s %6d words, hole at %06o, %d/%d epilogues resolve  (%s)'
              % (img, len(w), g, ok, ok + off, where))
        parts.append(carray('shr_' + img, raw))
        tab.append('    { "%s", shr_%s, sizeof shr_%s, %d },' % (img, img, img, g))
    parts.append('const shrfile_t shrfiles[] = {')
    parts.extend(tab)
    parts.append('    { 0, 0, 0, 0 }')
    parts.append('};')
    parts.append('')
    parts.append('const progentry_t progs[] = {')
    for cmd, img, what in spec['progs']:
        parts.append('    { "%s", "%s", "%s" },' % (cmd, img, what))
    parts.append('    { 0, 0, 0 }')
    parts.append('};')
    parts.append('')
    parts.append('const char *port_banner = "%s";' % spec['banner'].replace(chr(34), chr(92) + chr(34)))
    parts.append('')
    open(out, 'w').write('\n'.join(parts))
    print('wrote %s (%d bytes)' % (out, os.path.getsize(out)))


main()
