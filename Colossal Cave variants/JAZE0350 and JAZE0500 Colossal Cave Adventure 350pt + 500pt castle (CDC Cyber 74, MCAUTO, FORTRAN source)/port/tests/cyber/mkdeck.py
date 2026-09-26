#!/usr/bin/env python3
"""Build a NOS 1.3 card deck that compiles the MCAUTO source and plays a
session, for comparing the port against a real Cyber.

  python mkdeck.py <commands file> <deck> beg|adv

The deck is five records: the control statements, a little program that
writes the wizard's parameter file, `ADVENT.txt` itself, the database for the
game being played, and the commands - which the program reads from INPUT,
because its own PROGRAM card says so.

Three lines of the source are changed on the way through, and only three:

    CALL PFGET(1,"=DATABS1",...)      ->   CONTINUE
    CALL PFGET(1,"=DATABS2",...)      ->   CONTINUE
    CALL PFGET(3,"=AMAINT",...)       ->   CONTINUE

PFGET attached MCAUTO's permanent files, which are not on this machine; the
deck supplies TAPE1 and TAPE3 as local files instead.  Because of that the
BEG/ADV answer no longer chooses the database - the deck does - so the answer
in the commands file and the third argument here have to agree.

The game is loaded with LDSET(PRESET=ZERO).  The program needs memory that
starts at zero: the section 9 loader reads ten locations into TK but walks all
twenty (DO 1071 I=1,20), relying on TK(11)-TK(20) being 0 - which they were on
MCAUTO's Cyber, or the game would never have started, and which they are in
the port.  NOS 1.3's loader presets nothing by default, and without the option
the original dies with CPU ERROR EXIT 01 in INIT.

The wizard's parameter file is written by the little program from the values
POOF carries as comments, the same ones src/convert.py puts in amaint.dat; if
the two disagree the game behaves differently and the transcripts diverge.

Submit it with DtCyber's automation:  node job.js <deck> <output>
"""

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ORIG = os.path.join(HERE, '..', '..', '..', 'src_original')

CONTROL = """ADVJAZE.
$USER,INSTALL,INSTALL.
$NORERUN.
$SETTL,7777.
$SETJSL,7777.
$SETASL,7777.
$RFL,150000.
$COPYBR,INPUT,MAKER,1.
$COPYBR,INPUT,SOURCE,1.
$COPYBR,INPUT,TAPE1,1.
$REWIND,MAKER.
$FTN(I=MAKER,L=0,B=LGOM)
LGOM.
$REWIND,SOURCE.
$REWIND,TAPE1.
$FTN(I=SOURCE,L=0,B=LGOG)
$LDSET(PRESET=ZERO)
$LOAD(LGOG)
$EXECUTE.
***
*** ADVJAZE COMPLETE
***
EXIT.
***
*** ADVJAZE FAILED
***
"""

#  The values POOF used to read from =AMAINT, which it still carries as
#  comments: prime time 08.00-17.59 on weekdays, no weekend or holiday prime
#  time, a 30 turn short game, magic word DWAR, magic number 11111, and a 90
#  minute latency before a suspended game may be resumed.
MAKER = '''      PROGRAM MK(OUTPUT,TAPE3)
      IMPLICIT INTEGER(A-Z)
      DIMENSION JJ(11)
      JJ(1)=00777400B
      JJ(2)=0
      JJ(3)=0
      JJ(4)=0
      JJ(5)=-1
      JJ(6)=30
      JJ(7)="DWAR"
      JJ(8)=11111
      JJ(9)=90
      JJ(10)="          "
      JJ(11)="          "
      REWIND 3
      BUFFER OUT(3,1)(JJ(1),JJ(11))
      IF(UNIT(3))1,2,2
1     PRINT 10
10    FORMAT(" WROTE THE WIZARD PARAMETERS")
      STOP
2     PRINT 20
20    FORMAT(" COULD NOT WRITE THE WIZARD PARAMETERS")
      STOP
      END
'''

PFGET = [
    (' 2000 CALL PFGET(1,"=DATABS1","UN=XSY913","CT=PU","M=R")',
     ' 2000 CONTINUE'),
    (' 3000 CALL PFGET(1,"=DATABS2","UN=XSY913","CT=PU","M=R")',
     ' 3000 CONTINUE'),
    (' 5000 CALL PFGET(3,"=AMAINT","UN=XSY913","CT=PU","M=W")',
     ' 5000 CONTINUE'),
]


def rec(text):
    out = []
    for line in text.split('\n'):
        line = line.rstrip()
        #  A card is 80 columns.  A few comment lines in this listing are a
        #  character or two longer; FTN would not have seen those either.
        if len(line) > 80:
            if line[:1] not in ('C', '*'):
                sys.exit('mkdeck: card too long: %r' % line)
            line = line[:80].rstrip()
        out.append(line)
    while out and not out[-1]:
        out.pop()
    return '\n'.join(out) + '\n'


def main():
    if len(sys.argv) < 4 or sys.argv[3] not in ('beg', 'adv'):
        sys.exit(__doc__)
    cmds = open(sys.argv[1], encoding='latin-1').read()
    deckfile, which = sys.argv[2], sys.argv[3]

    src = open(os.path.join(ORIG, 'ADVENT.txt'),
               encoding='latin-1').read().replace('\r\n', '\n')
    lines = [l.rstrip() for l in src.split('\n')]
    while lines and lines[0].strip() == 'ADVENT':
        lines.pop(0)
    for old, new in PFGET:
        n = lines.count(old)
        if n != 1:
            sys.exit('mkdeck: %r appears %d times' % (old, n))
        lines[lines.index(old)] = new
    src = '\n'.join(lines)

    data = open(os.path.join(ORIG,
                             '001.2.txt' if which == 'beg' else '001.1.txt'),
                encoding='latin-1').read().replace('\r\n', '\n')

    deck = (CONTROL + '~eor\n' + rec(MAKER) + '~eor\n' + rec(src)
            + '~eor\n' + rec(data) + '~eor\n' + rec(cmds))
    with open(deckfile, 'w', newline='\n', encoding='latin-1') as f:
        f.write(deck)
    print('%s: %d cards, %s database' % (deckfile, deck.count('\n'), which))


if __name__ == '__main__':
    main()
