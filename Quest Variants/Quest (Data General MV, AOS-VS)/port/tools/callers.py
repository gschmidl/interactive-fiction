"""Find every LCALL/LJSR to an address, named by the nearest symbol."""
import sys, os, bisect
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from st import load_pr, read_st
DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'data')
prog, tgt = sys.argv[1], int(sys.argv[2], 16)
M, _ = load_pr(os.path.join(DATA, prog + '.PR'))
syms = read_st(os.path.join(DATA, prog + '.ST'))
flat = sorted((v & 0x0FFFFFFF, k) for k, vs in syms.items() for v in vs
              if 0x1000 < (v & 0x0FFFFFFF) < 0x200000)
addrs = [a for a, _ in flat]
def near(a):
    i = bisect.bisect_right(addrs, a) - 1
    return flat[i][1] if i >= 0 else '?'
hi, lo = (0x7000 | (tgt >> 16)) & 0xFFFF, tgt & 0xFFFF
for a in range(len(M) - 3):
    if M[a] in (0xA6C9, 0xA6E9) and M[a+1] == hi and M[a+2] == lo:
        print('%06X  %-28s %s' % (a, near(a), 'LCALL' if M[a] == 0xA6C9 else 'LJSR'))
