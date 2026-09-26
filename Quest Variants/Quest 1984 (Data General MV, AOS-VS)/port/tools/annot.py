"""Annotate an instruction trace with the nearest QUEST symbol."""
import sys, os, bisect
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from st import read_st
DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'data')
syms = read_st(os.path.join(DATA, (sys.argv[1] if len(sys.argv) > 1 else 'QUEST') + '.ST'))
flat = sorted((v & 0x0FFFFFFF, k) for k, vs in syms.items() for v in vs
              if 0x1000 < (v & 0x0FFFFFFF) < 0x200000)
addrs = [a for a, _ in flat]
last = None
for ln in sys.stdin:
    p = ln.split()
    if not p:
        print(ln.rstrip()); continue
    try: a = int(p[0], 16)
    except ValueError:
        print(ln.rstrip()); continue
    i = bisect.bisect_right(addrs, a) - 1
    nm = flat[i][1] if i >= 0 else '?'
    if nm != last: print('--- %s' % nm); last = nm
    print(ln.rstrip())
