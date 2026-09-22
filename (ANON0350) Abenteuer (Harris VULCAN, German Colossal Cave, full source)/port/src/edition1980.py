#!/usr/bin/env python3
"""ABENTEUER's database as it was in 1980: ADV1980.DATA.

    python src/edition1980.py DATABASE_DIR OUTPUT

The section files on the tape (TAPE1 ... TAPE1012) are a later edition
than the one the site's NEUSPIEL was set up from on 13 June 1980.  Five
edits were made since, all in text:

  TAPE1  "FEE FIE FOE FOO" [SIC]. and [WITTS ... GESELLSCHAFT]" became
         \\SIC!. and \\WITTS ...!" - while TAPE1's other exclamation marks
         stayed, so this is an edit and not a character code
  TAPE4  a line 0 after the section number: a vocabulary entry 0, blank
  TAPE5  every ! became [ (30 of them)
  TAPE6  "... NICHT WERTVOLL GENUG.", MEINT ER. gained its comma

This undoes them - each must be found exactly as often as listed - and
writes the sections in the order build.sh puts them into ADV.DATA.  The
game set up from the result (abenteuer --fresh=1980) is the site's
NEUSPIEL, all 13,552 variables of it: tests\\crosscheck.py.
"""
import os
import sys

# the section files, in the order build.sh puts them into ADV.DATA
SECTIONS = ['TAPE1.txt', 'TAPE2.txt', 'TAPE3.txt', 'TAPE4.txt', 'TAPE5.txt',
            'TAPE6.txt', 'TAPE7-9.txt', 'TAPE1012.txt']

# (file, the tape's text, the 1980 text, how often)
EDITS = [
    ('TAPE1.txt', '"FEE FIE FOE FOO" \\SIC!.', '"FEE FIE FOE FOO" [SIC].', 1),
    ('TAPE1.txt', '\\WITTS KONSTRUKTIONSGESELLSCHAFT!"',
     '[WITTS KONSTRUKTIONSGESELLSCHAFT]"', 1),
    ('TAPE4.txt', '       4\n       0\n', '       4\n', 1),
    ('TAPE5.txt', '[', '!', 30),
    ('TAPE6.txt', 'GENUG.", MEINT', 'GENUG." MEINT', 1),
]


def database(db):
    """the 1980 edition, as bytes, from the section files in DB"""
    out = ''
    for f in SECTIONS:
        text = open(os.path.join(db, f), 'rb').read().decode('latin-1')
        for g, old, new, n in EDITS:
            if g == f:
                if text.count(old) != n:
                    sys.exit('edition1980: %s has %r %d times, not %d'
                             % (f, old, text.count(old), n))
                text = text.replace(old, new)
        out += text
    return out.encode('latin-1')


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    data = database(sys.argv[1])
    with open(sys.argv[2], 'wb') as f:
        f.write(data)
    return 0


if __name__ == '__main__':
    sys.exit(main())
