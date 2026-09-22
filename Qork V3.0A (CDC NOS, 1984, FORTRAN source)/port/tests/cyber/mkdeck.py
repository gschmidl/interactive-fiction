#!/usr/bin/env python3
"""Build a NOS 2.8.7 card deck that compiles Qork with FTN5 and plays a session.

  python mkdeck.py <commands file> <deck> [--listing]

Qork V3.0A is written for FTN5 (FORTRAN 77: O"..." octal constants, a block
IF), so the reference machine is NOS 2.8.7, not the NOS 1.3 (FTN 4.7) of the
collection's other CDC ports.  The deck is four records:

  1. the control statements;
  2. qork.src exactly as it is in src_original - FTN5 hands its two COMPASS
     subprograms (RIO, CONCAT) to the assembler itself;
  3. qork.txt, the database, which the program reads as DATBAS (TAPE1);
  4. the commands, which it reads from INPUT (TAPE5=INPUT).

RIO takes four NOS common decks with `COMCLIB XTEXT COMCRDS` (COMCWTS,
COMCRDW, COMCWTW): the text from a program library on the local file
COMCLIB.  The site had one; here the job attaches the system's own source
library, INSTALL's OPL871, under that name.

The program runs twice, as the site ran it.  With sense switch 1 off, PRS
reads DATBAS into the random file TAPE2 ("CREATING NEW 'DTEXT.DAT'"), saves
the whole initial state with SAVEGM, and exits.  With SWITCH,1 it restores
that state (RSTRGM) and plays.  Both files are local and last the job.

The session must end with the program's own exit: at the end of its input
it prints "I CANNOT HEAR YOU!" and reads again, for ever, so the time
limit is kept small.  With --listing the compiler listing is printed too.

Submit it from a DtCyber tree:  node job287.js <deck> <output>
"""

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ORIG = os.path.join(HERE, '..', '..', '..', 'src_original')

CONTROL = """QORKJOB.
USER,INSTALL,INSTALL.
NORERUN.
SETTL,30.
RFL,300000.
COPYBR,INPUT,SOURCE,1.
COPYBR,INPUT,DATBAS,1.
REWIND,SOURCE,DATBAS.
ATTACH,COMCLIB=OPL871/UN=INSTALL.
FTN5(I=SOURCE,L={listing})
LGO.
SWITCH,1.
LGO.
***
*** QORKJOB COMPLETE
***
EXIT.
***
*** QORKJOB FAILED
***
"""


def cards(path):
    text = open(path, 'rb').read().decode('latin-1').replace('\r\n', '\n')
    out = []
    for line in text.split('\n'):
        line = line.rstrip()
        if len(line) > 80:
            sys.exit('mkdeck: card longer than 80 columns in %s: %r' % (path, line))
        out.append(line)
    while out and not out[-1]:
        out.pop()
    return '\n'.join(out)


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    control = CONTROL.format(listing='OUTPUT' if '--listing' in sys.argv[3:] else '0')
    deck = '\n~eor\n'.join([control.rstrip('\n'),
                            cards(os.path.join(ORIG, 'qork.src')),
                            cards(os.path.join(ORIG, 'qork.txt')),
                            cards(sys.argv[1])]) + '\n'
    open(sys.argv[2], 'w', encoding='latin-1', newline='\n').write(deck)
    print('mkdeck: %d cards -> %s' % (deck.count('\n'), sys.argv[2]))


if __name__ == '__main__':
    main()
