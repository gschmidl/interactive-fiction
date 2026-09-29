#!/usr/bin/env python3
"""Can the game be won - 350 of 350 points?

    python tests\\win.py [-v]

Not a walkthrough played against the dwarves: --debug sets the state the
long middle of the game leaves -

    every treasure in the building, found (prop 0);
    the magazine at Witt's End (the point for visiting it);
    tally 0 (nothing left to find), so the closing clock can run;
    the lamp carried and lit, and the player in the Hall of Mists;
    clock1 at 1, and then clock2 at 1, to save thirty turns of waiting -

and then the program does the rest itself: the sepulchral voice warns
that the cave is closing, then closes it and carries the player to the
repository.  There the classic ending is played: take the black rod with
the rusty mark from the southwest end, leave it at the northeast end, go
back and BLAST.  dflag is set at the end (being "well inside" the cave is
worth 25 points; setting it earlier would let the dwarves loose during
the test).

Checked: the explosion that ends the game, 350 of 350 points and the top
rank, with the fixes and with --no-fixes.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import common as C  # noqa: E402

EXPECT = ['The sepulchral voice intones, "The cave is now closed."',
          'burying the dwarves in the rubble.',
          'carry the conquering adventurer off into the sunset.',
          'You scored 350 points out of a possible 350',
          'All of Adventuredom gives tribute to you, Adventure Grandmaster!',
          'would be a neat trick!']


def commands():
    cmds = ['no']
    for t in C.TREASURES:
        cmds += ['#put %d %d' % (C.OBJ[t], C.LOC['house']), '#prop %d 0' % C.OBJ[t]]
    cmds += ['#put %d %d' % (C.OBJ['MAG'], C.LOC['witt']),
             '#set tally 0', '#set lost 0',
             '#put %d -1' % C.OBJ['LAMP'], '#prop %d 1' % C.OBJ['LAMP'],
             '#loc %d' % C.LOC['emist'],
             '#set clock1 1', 'look',            # the closing warning
             '#set clock2 1', 'inventory',       # the cave closes
             'sw', 'take rod', 'ne', 'drop rod', 'sw',
             '#set dflag 1', '#show', 'blast']
    return cmds


def check(args, verbose):
    out, err, rc = C.run(commands(), ['--debug', '--seed', '7'] + args)
    at, miss = 0, None
    for e in EXPECT:
        i = out.find(e, at)
        if i < 0:
            miss = e
            break
        at = i + len(e)
    ok = miss is None and rc == 0
    if verbose or not ok:
        print(out + err)
        if miss:
            print('[missing: %s]' % miss)
    print('%-50s %s' % ('win %s: 350 of 350' % (' '.join(args) or '(fixes)'),
                        'ok' if ok else 'FAILED'))
    return ok


def main():
    verbose = '-v' in sys.argv[1:]
    ok = check([], verbose) & check(['--no-fixes'], verbose)
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
