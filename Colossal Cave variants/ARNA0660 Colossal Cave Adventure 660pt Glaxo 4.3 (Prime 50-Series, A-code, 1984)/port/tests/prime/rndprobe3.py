"""RND with structured 32-bit seeds: for each seed (two halfwords), the
first three numbers after it, as raw halfwords."""
import sys
import time

from primesh import Prime
from edput import put, check

SEEDS = [(0, 1), (0, 2), (0, 128), (0, 256), (0, 512), (0, 1024), (0, 16384),
         (0, -32768), (1, 0), (2, 0), (4, 0), (256, 0), (16384, 0),
         (-32768, 0), (501, 0), (501, 1), (501, 128), (501, 256),
         (1, 1), (0, 129)]

SRC = """C RND PROBE 3
      REAL RND,X
      INTEGER KK(2),IA(2),I,J,S(40)
      EQUIVALENCE (X,KK(1))
      DATA S/%s/
      DO 50 J=1,%d
      IA(1)=S(2*J-1)
      IA(2)=S(2*J)
      X=RND(IA(1))
      WRITE(1,10) IA
      DO 20 I=1,3
      X=RND(0)
20    WRITE(1,10) KK
50    CONTINUE
10    FORMAT(2I8)
      CALL EXIT
      END
"""


def main():
    vals = []
    for a, b in SEEDS:
        vals += [a, b]
    data = ','.join(str(v) for v in vals)
    # DATA lines must stay within 72 columns: continue them
    src = SRC % ('@', len(SEEDS))
    first, rest = src.split('@')
    line = first
    out = []
    for tok in data.split(','):
        piece = tok + ','
        if len(line) + len(piece) > 70:
            out.append(line)
            line = '     +'
        line += piece
    line = line[:-1] + rest.split('\n', 1)[0]
    out.append(line)
    src = '\n'.join(out) + '\n' + rest.split('\n', 1)[1]
    p = Prime(log='rndprobe3.log')
    try:
        p.login()
        put(p, 'RNDS.FTN', src)
        if not check(p, 'RNDS.FTN', src):
            return 1
        print(p.cmd('FTN RNDS -64V', timeout=120))
        p.line('SEG')
        time.sleep(2)
        for c in ('LOAD #RNDS', 'LO RNDS.BIN', 'LI VAPPLB', 'LI', 'SAVE',
                  'RETURN', 'QUIT'):
            p.line(c)
            time.sleep(2)
            p.read()
        print(p.expect(r'\nOK, |\nER! ', 60))
        print(p.cmd('SEG #RNDS', timeout=60))
    finally:
        p.close()
    return 0


if __name__ == '__main__':
    sys.exit(main())
