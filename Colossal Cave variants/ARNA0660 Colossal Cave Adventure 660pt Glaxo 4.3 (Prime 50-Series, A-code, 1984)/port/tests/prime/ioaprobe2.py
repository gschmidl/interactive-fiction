"""What IOA$ does with the executive's texts: no new line of its own?  the
%! in Ralph's "GET OUT!$!!!%!" (text 4386015)?  a string cut by its
length?  Each line is fenced with TNOU so the output shows the joins."""
import sys
import time

from primesh import Prime
from edput import put, check

SRC = """C IOA$ PROBE 2
      CALL IOA$('X%%Y',4)
      CALL TNOU('<1>',3)
      CALL IOA$('X%/Y',4)
      CALL TNOU('<2>',3)
      CALL IOA$('X%DY',4)
      CALL TNOU('<3>',3)
      CALL IOA$('X%AY',4)
      CALL TNOU('<4>',3)
      CALL IOA$('X%1Y',4)
      CALL TNOU('<5>',3)
      CALL IOA$('X%XY',4)
      CALL TNOU('<6>',3)
      CALL IOA$('X% Y',4)
      CALL TNOU('<7>',3)
      CALL IOA$('X%',2)
      CALL TNOU('<8>',3)
      CALL IOA$('X%$Y',4)
      CALL TNOU('<9>',3)
      CALL IOA$('X%!%!Y',6)
      CALL TNOU('<10>',3)
      CALL EXIT
      END
"""


def main():
    p = Prime(log='ioaprobe2.log')
    try:
        p.login()
        put(p, 'IOAQ.FTN', SRC)
        if not check(p, 'IOAQ.FTN', SRC):
            return 1
        print(p.cmd('FTN IOAQ -64V', timeout=120))
        p.line('SEG')
        time.sleep(2)
        for c in ('LOAD #IOAQ', 'LO IOAQ.BIN', 'LI VAPPLB', 'LI', 'SAVE',
                  'RETURN', 'QUIT'):
            p.line(c)
            time.sleep(2)
            p.read()
        print(p.expect(r'\nOK, |\nER! ', 60))
        out = p.cmd('SEG #IOAQ', timeout=60)
        print(repr(out))
        print(out)
    finally:
        p.close()
    return 0


if __name__ == '__main__':
    sys.exit(main())
