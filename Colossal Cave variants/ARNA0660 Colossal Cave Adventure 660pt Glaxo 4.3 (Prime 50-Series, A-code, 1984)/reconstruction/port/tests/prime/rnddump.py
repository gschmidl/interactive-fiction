"""Dump RND's code: EXTERNAL RND passed to DUMP(F) gives F = RND's entry
control block, whose first two halfwords are the code's address; the code
is then read as F(K...) relative to it (same segment)."""
import sys
import time

from primesh import Prime
from edput import put, check

SRC = """C DUMP RND
      EXTERNAL RND
      CALL DUMP(RND)
      CALL EXIT
      END
      SUBROUTINE DUMP(F)
      INTEGER F(1),I,K,J,LH(2)
      INTEGER*4 L
      EQUIVALENCE (L,LH(1))
      L=LOC(F(1))
      WRITE(1,20) LH
      WRITE(1,20) (F(I),I=1,16)
      K=F(2)-LH(2)+1
      WRITE(1,20) K
      DO 30 J=0,19
      WRITE(1,20) (F(K+16*J+I),I=-64,-49)
30    CONTINUE
20    FORMAT(16I7)
      RETURN
      END
"""


def main():
    p = Prime(log='rnddump.log')
    try:
        p.login()
        put(p, 'RNDD.FTN', SRC)
        if not check(p, 'RNDD.FTN', SRC):
            return 1
        print(p.cmd('FTN RNDD -64V', timeout=120))
        p.line('SEG')
        time.sleep(2)
        for c in ('LOAD #RNDD', 'LO RNDD.BIN', 'LI VAPPLB', 'LI', 'MAP',
                  'SAVE', 'RETURN', 'QUIT'):
            p.line(c)
            time.sleep(2)
            p.read()
        print(p.expect(r'\nOK, |\nER! ', 60))
        print(p.cmd('SEG #RNDD', timeout=60))
    finally:
        p.close()
    return 0


if __name__ == '__main__':
    sys.exit(main())
