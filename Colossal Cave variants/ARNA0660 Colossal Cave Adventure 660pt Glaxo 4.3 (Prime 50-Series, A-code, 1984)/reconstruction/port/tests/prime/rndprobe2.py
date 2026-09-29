"""RND, black box: 30 values after RND(501) with their raw halfwords, and
whether the halfword after the argument matters (RND reading a REAL or an
INTEGER*4 would see it)."""
import sys
import time

from primesh import Prime
from edput import put, check

SRC = """C RND PROBE 2
      REAL RND,X
      INTEGER KK(2),IA(2),I
      EQUIVALENCE (X,KK(1))
      IA(1)=501
      IA(2)=0
      X=RND(IA(1))
      WRITE(1,10) X,KK
      DO 20 I=1,30
      X=RND(0)
20    WRITE(1,10) X,KK
      IA(2)=12345
      X=RND(IA(1))
      WRITE(1,10) X,KK
      DO 30 I=1,3
      X=RND(0)
30    WRITE(1,10) X,KK
      IA(1)=500
      IA(2)=0
      X=RND(IA(1))
      WRITE(1,10) X,KK
      DO 40 I=1,3
      X=RND(0)
40    WRITE(1,10) X,KK
10    FORMAT(F14.10,2I8)
      CALL EXIT
      END
"""


def main():
    p = Prime(log='rndprobe2.log')
    try:
        p.login()
        put(p, 'RNDR.FTN', SRC)
        if not check(p, 'RNDR.FTN', SRC):
            return 1
        print(p.cmd('FTN RNDR -64V', timeout=120))
        p.line('SEG')
        time.sleep(2)
        for c in ('LOAD #RNDR', 'LO RNDR.BIN', 'LI VAPPLB', 'LI', 'SAVE',
                  'RETURN', 'QUIT'):
            p.line(c)
            time.sleep(2)
            p.read()
        print(p.expect(r'\nOK, |\nER! ', 60))
        print(p.cmd('SEG #RNDR', timeout=60))
    finally:
        p.close()
    return 0


if __name__ == '__main__':
    sys.exit(main())
