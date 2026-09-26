#!/usr/bin/env python3
"""Build a NOS 2.8.7 deck that measures Qork's RIO (the COMPASS random file
module) exactly as qork.src has it.

  python mkprobe.py <deck>

The probe writes one record per test line, as PRS does - WRR(2,ADDR(K))
then WRI(2,LINE,170), the line blank-filled to 170 characters - with
texts of every length from 1 to 22 characters, plus a record of three
lines; then reads each back with RDR (and RNL for the second and third
lines) and prints, in octal, the random address, the EOF status and the
first 26 words of the buffer, which READS fills one character per word.
It also prints what READS leaves in a buffer word it did not fill.

RIO is taken from qork.src (lines IDENT RIO .. END) untouched.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, '..', '..', '..', 'src_original', 'qork.src')

CONTROL = """PROBEJ.
USER,INSTALL,INSTALL.
NORERUN.
SETTL,10.
COPYBR,INPUT,SOURCE,1.
REWIND,SOURCE.
ATTACH,COMCLIB=OPL871/UN=INSTALL.
FTN5(I=SOURCE,L=0)
LGO.
***
*** PROBEJ COMPLETE
***
EXIT.
***
*** PROBEJ FAILED
***"""

PROGRAM = """      PROGRAM PROBE(OUTPUT,TAPE6=OUTPUT)
      INTEGER LINE(170),B(170),ADDR(30),ALPHA(22)
      DATA ALPHA/R"A",R"B",R"C",R"D",R"E",R"F",R"G",R"H",R"I",R"J",
     +R"K",R"L",R"M",R"N",R"O",R"P",R"Q",R"R",R"S",R"T",R"U",R"V"/
      CALL OPEN(2)
      DO 20 K=1,22
      DO 10 I=1,170
   10 LINE(I)=R" "
      DO 15 I=1,K
   15 LINE(I)=ALPHA(I)
      CALL WRR(2,ADDR(K))
      CALL WRI(2,LINE,170)
   20 CONTINUE
      CALL WRR(2,ADDR(23))
      DO 30 K=1,3
      DO 25 I=1,170
   25 LINE(I)=R" "
      DO 27 I=1,K+4
   27 LINE(I)=ALPHA(I)
      CALL WRI(2,LINE,170)
   30 CONTINUE
      CALL CLOSE(2)
      DO 50 K=1,23
      DO 40 I=1,170
   40 B(I)=O"12345"
      CALL RDR(2,B,170,ADDR(K),IEOF)
      WRITE(6,100) K,ADDR(K),IEOF,(B(I),I=1,26)
   50 CONTINUE
      DO 60 J=1,3
      DO 55 I=1,170
   55 B(I)=O"12345"
      CALL RNL(2,B,170,IEOF)
      WRITE(6,100) J,0,IEOF,(B(I),I=1,26)
   60 CONTINUE
  100 FORMAT(1X,I2,1X,O20,1X,O20/(1X,13O3))
      END"""


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    src = open(SRC, 'rb').read().decode('latin-1').replace('\r\n', '\n').split('\n')
    a = next(i for i, l in enumerate(src) if 'IDENT  RIO' in l)
    b = next(i for i in range(a, len(src)) if src[i][:72].split() == ['END'])
    rio = [l.rstrip() for l in src[a:b + 1]]
    deck = CONTROL + '\n~eor\n' + PROGRAM + '\n' + '\n'.join(rio) + '\n'
    open(sys.argv[1], 'w', encoding='latin-1', newline='\n').write(deck)
    print('mkprobe: RIO lines %d-%d -> %s' % (a + 1, b + 1, sys.argv[1]))


if __name__ == '__main__':
    main()
