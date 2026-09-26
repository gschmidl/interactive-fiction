"""What IOA$ does with the executive's texts: no new line of its own?  the
%! in Ralph's "GET OUT!$!!!%!" (text 4386015)?  a string cut by its
length?  Each line is fenced with TNOU so the output shows the joins."""
import sys
import time

from primesh import Prime
from edput import put, check

SRC = """C IOA$ PROBE
      CALL TNOU('<1>',3)
      CALL IOA$('AB',2)
      CALL IOA$('CD',2)
      CALL TNOU('<2>',3)
      CALL IOA$('OUT! GET OUT!$!!!%!" X',22)
      CALL TNOU('<3>',3)
      CALL IOA$('? %$',100)
      CALL TNOU('<4>',3)
      CALL IOA$('ABCDEFGH',4)
      CALL TNOU('<5>',3)
      CALL IOA$('A%B',3)
      CALL TNOU('<6>',3)
      CALL EXIT
      END
"""


def main():
    p = Prime(log='ioaprobe.log')
    try:
        p.login()
        put(p, 'IOAP.FTN', SRC)
        if not check(p, 'IOAP.FTN', SRC):
            return 1
        print(p.cmd('FTN IOAP -64V', timeout=120))
        p.line('SEG')
        time.sleep(2)
        for c in ('LOAD #IOAP', 'LO IOAP.BIN', 'LI VAPPLB', 'LI', 'SAVE',
                  'RETURN', 'QUIT'):
            p.line(c)
            time.sleep(2)
            p.read()
        print(p.expect(r'\nOK, |\nER! ', 60))
        out = p.cmd('SEG #IOAP', timeout=60)
        print(repr(out))
        print(out)
    finally:
        p.close()
    return 0


if __name__ == '__main__':
    sys.exit(main())
