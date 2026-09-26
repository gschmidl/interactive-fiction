"""RND as the A-code executive gets it: F77 -INTS -LOGS -BIG, RND(I) with I
left at 501 by a DO loop, then RND(0); also RND of REAL arguments.  Prints
the raw halfwords of every result in octal.  Run twice."""
import sys
import time

from primesh import Prime
from edput import put, check

SRC = """C RND PROBE, F77 AS THE EXECUTIVE
      REAL*4 RND, X, Y
      INTEGER*2 KK(2)
      EQUIVALENCE (X, KK)
      DO 5 I = 0, 500
5     CONTINUE
      X = RND(I)
      WRITE (1, 10) I, X, KK
      DO 20 J = 1, 8
      X = RND(0)
      WRITE (1, 10) J, X, KK
20    CONTINUE
      Y = 2.4
      X = RND(Y)
      WRITE (1, 10) 24, X, KK
      Y = 2.6
      X = RND(Y)
      WRITE (1, 10) 26, X, KK
      Y = -1.5
      X = RND(Y)
      WRITE (1, 10) -15, X, KK
      X = RND(0)
      WRITE (1, 10) 0, X, KK
10    FORMAT (I6, F16.10, 2O8)
      CALL EXIT
      END
"""


def main():
    p = Prime(log='rndprobe77.log')
    try:
        p.login()
        put(p, 'RNDQ.F77', SRC)
        if not check(p, 'RNDQ.F77', SRC):
            return 1
        print(p.cmd('F77 RNDQ -INTS -LOGS -BIG', timeout=120))
        p.line('SEG')
        time.sleep(2)
        for c in ('LOAD #RNDQ', 'LO RNDQ.BIN', 'LI VAPPLB', 'LI', 'MAP 3',
                  'SAVE', 'RETURN', 'QUIT'):
            p.line(c)
            time.sleep(2)
            p.read()
        print(p.expect(r'\nOK, |\nER! ', 60))
        for k in range(2):
            print('--- run %d' % (k + 1))
            print(p.cmd('SEG #RNDQ', timeout=60))
            time.sleep(3)
    finally:
        p.close()
    return 0


if __name__ == '__main__':
    sys.exit(main())
