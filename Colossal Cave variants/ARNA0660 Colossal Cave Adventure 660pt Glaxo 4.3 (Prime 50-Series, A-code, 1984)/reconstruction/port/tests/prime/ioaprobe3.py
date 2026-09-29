"""What IOA$ does with the executive's texts: no new line of its own?  the
%! in Ralph's "GET OUT!$!!!%!" (text 4386015)?  a string cut by its
length?  Each line is fenced with TNOU so the output shows the joins."""
import sys
import time

from primesh import Prime
from edput import put, check

SRC = """C IOA$ PROBE 3
      CALL IOA$('X%AY',4)
      CALL TNOU(' <A>',4)
      CALL IOA$('X%BY',4)
      CALL TNOU(' <B>',4)
      CALL IOA$('X%CY',4)
      CALL TNOU(' <C>',4)
      CALL IOA$('X%DY',4)
      CALL TNOU(' <D>',4)
      CALL IOA$('X%EY',4)
      CALL TNOU(' <E>',4)
      CALL IOA$('X%FY',4)
      CALL TNOU(' <F>',4)
      CALL IOA$('X%GY',4)
      CALL TNOU(' <G>',4)
      CALL IOA$('X%HY',4)
      CALL TNOU(' <H>',4)
      CALL IOA$('X%IY',4)
      CALL TNOU(' <I>',4)
      CALL IOA$('X%JY',4)
      CALL TNOU(' <J>',4)
      CALL IOA$('X%KY',4)
      CALL TNOU(' <K>',4)
      CALL IOA$('X%LY',4)
      CALL TNOU(' <L>',4)
      CALL IOA$('X%MY',4)
      CALL TNOU(' <M>',4)
      CALL IOA$('X%NY',4)
      CALL TNOU(' <N>',4)
      CALL IOA$('X%OY',4)
      CALL TNOU(' <O>',4)
      CALL IOA$('X%PY',4)
      CALL TNOU(' <P>',4)
      CALL IOA$('X%QY',4)
      CALL TNOU(' <Q>',4)
      CALL IOA$('X%RY',4)
      CALL TNOU(' <R>',4)
      CALL IOA$('X%SY',4)
      CALL TNOU(' <S>',4)
      CALL IOA$('X%TY',4)
      CALL TNOU(' <T>',4)
      CALL IOA$('X%UY',4)
      CALL TNOU(' <U>',4)
      CALL IOA$('X%VY',4)
      CALL TNOU(' <V>',4)
      CALL IOA$('X%WY',4)
      CALL TNOU(' <W>',4)
      CALL IOA$('X%XY',4)
      CALL TNOU(' <X>',4)
      CALL IOA$('X%YY',4)
      CALL TNOU(' <Y>',4)
      CALL IOA$('X%ZY',4)
      CALL TNOU(' <Z>',4)
      CALL EXIT
      END
"""


def main():
    p = Prime(log='ioaprobe3.log')
    try:
        p.login()
        put(p, 'IOAR.FTN', SRC)
        if not check(p, 'IOAR.FTN', SRC):
            return 1
        print(p.cmd('FTN IOAR -64V', timeout=120))
        p.line('SEG')
        time.sleep(2)
        for c in ('LOAD #IOAR', 'LO IOAR.BIN', 'LI VAPPLB', 'LI', 'SAVE',
                  'RETURN', 'QUIT'):
            p.line(c)
            time.sleep(2)
            p.read()
        print(p.expect(r'\nOK, |\nER! ', 60))
        out = p.cmd('SEG #IOAR', timeout=60)
        print(repr(out))
        print(out)
    finally:
        p.close()
    return 0


if __name__ == '__main__':
    sys.exit(main())
