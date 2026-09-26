"""Measure PRIMOS's RND (as the A-code executive calls it) on the machine.

Uploads RNDP.FTN with ED, compiles it with FTN, loads it with SEG against
VAPPLB and the default libraries (as EXECUTIVE.BUILD.CPL does), runs it
twice and prints what it wrote.
"""
import sys
import time

from primesh import Prime
from edput import put, check

SRC = """C RND PROBE
      REAL RND,X
      INTEGER I,J
      X=RND(501)
      WRITE(1,10) X
      DO 20 I=1,4
      X=RND(0)
20    WRITE(1,10) X
      X=RND(502)
      WRITE(1,10) X
      X=RND(0)
      WRITE(1,10) X
      X=RND(501)
      WRITE(1,10) X
      X=RND(0)
      WRITE(1,10) X
      X=RND(1)
      WRITE(1,10) X
      X=RND(-1)
      WRITE(1,10) X
10    FORMAT(F14.10)
      CALL EXIT
      END
"""


def main():
    p = Prime(log='rndprobe.log')
    try:
        p.login()
        put(p, 'RNDP.FTN', SRC)
        if not check(p, 'RNDP.FTN', SRC):
            return 1
        print(p.cmd('FTN RNDP -64V', timeout=120))
        p.line('SEG')
        time.sleep(2)
        for c in ('LOAD #RNDP', 'LO RNDP.BIN', 'LI VAPPLB', 'LI', 'SAVE',
                  'RETURN', 'QUIT'):
            p.line(c)
            time.sleep(2)
            p.read()
        print(p.expect(r'\nOK, |\nER! ', 60))
        for k in range(2):
            print('--- run %d' % (k + 1))
            print(p.cmd('SEG #RNDP', timeout=60))
            time.sleep(3)
    finally:
        p.close()
    return 0


if __name__ == '__main__':
    sys.exit(main())
