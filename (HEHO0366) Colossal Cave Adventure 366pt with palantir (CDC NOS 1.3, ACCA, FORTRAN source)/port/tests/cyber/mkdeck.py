#!/usr/bin/env python3
"""Build a NOS 1.3 card deck that compiles the game and plays a session.

  python mkdeck.py <commands file> <deck> [--seed]

The deck is four records: the control statements, `adventure.src` exactly as
it is in src_original, `adventure.txt` likewise, and the commands - which the
program reads from INPUT, because its own PROGRAM statement says TAPE5=INPUT.

With --seed, one line of the source is changed on the way through:

    CALL RANSET(SECOND(1.))   ->   CALL RANSET(0.5)

so that the generator starts at a known seed (0.5 sets it to
04000000000000001 octal = 140737488355329).  Without that the game seeds
itself from the CPU clock and no two runs are alike; with it the run can be
compared against the port started with --seed 140737488355329.  Nothing else
in the deck differs.

Submit it with DtCyber's automation:  node job.js <deck> <output>
"""

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ORIG = os.path.join(HERE, '..', '..', '..', 'src_original')

CONTROL = """ADVJOB.
$USER,INSTALL,INSTALL.
$NORERUN.
$SETTL,7777.
$SETJSL,7777.
$SETASL,7777.
$RFL,150000.
$COPYBR,INPUT,SOURCE,1.
$COPYBR,INPUT,TAPE1,1.
$REWIND,SOURCE.
$FTN(I=SOURCE,L=0)
LOAD(LGO)
NOGO(ADVENT)
ADVENT.
***
*** ADVJOB COMPLETE
***
EXIT.
***
*** ADVJOB FAILED
***
"""

SEEDOLD = '      CALL RANSET(SECOND(1.))'
SEEDNEW = '      CALL RANSET(0.5)'


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    cmds = open(sys.argv[1], encoding='latin-1').read()
    deckfile = sys.argv[2]
    fixseed = '--seed' in sys.argv[3:]

    src = open(os.path.join(ORIG, 'adventure.src'),
               encoding='latin-1').read().replace('\r\n', '\n')
    if fixseed:
        n = sum(1 for l in src.split('\n') if l[:72].rstrip() == SEEDOLD)
        if n != 1:
            sys.exit('mkdeck: found the RANSET line %d times' % n)
        src = '\n'.join(SEEDNEW if l[:72].rstrip() == SEEDOLD else l
                        for l in src.split('\n'))
    data = open(os.path.join(ORIG, 'adventure.txt'),
                encoding='latin-1').read().replace('\r\n', '\n')

    def rec(text):
        out = []
        for line in text.split('\n'):
            line = line.rstrip()
            if len(line) > 80:
                sys.exit('mkdeck: card longer than 80 columns: %r' % line)
            out.append(line)
        while out and not out[-1]:
            out.pop()
        return '\n'.join(out) + '\n'

    deck = (CONTROL + '~eor\n' + rec(src) + '~eor\n' + rec(data)
            + '~eor\n' + rec(cmds))
    with open(deckfile, 'w', newline='\n', encoding='latin-1') as f:
        f.write(deck)
    print('%s: %d cards%s' % (deckfile, deck.count('\n'),
                              ', seed fixed at 140737488355329' if fixseed
                              else ''))


if __name__ == '__main__':
    main()
