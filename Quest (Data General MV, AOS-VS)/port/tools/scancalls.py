"""List every .SYSTM call number reached via XJSR @6 in a 32-bit .PR."""
import sys, os, collections
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from st import load_pr, read_st
DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'data')
names = {}
for ln in open(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                            'sysid.txt')).read().splitlines():
    p = ln.split()
    if len(p) >= 2: names[int(p[0])] = p[1]
for prog in sys.argv[1:]:
    M, _ = load_pr(os.path.join(DATA, prog + '.PR'))
    syms = read_st(os.path.join(DATA, prog + '.ST'))
    flat = sorted((v & 0x0FFFFFFF, k) for k, vs in syms.items() for v in vs)
    def near(a):
        lo = [x for x in flat if x[0] <= a and x[0] > 0x1000]
        return lo[-1][1] if lo else '?'
    hits = collections.defaultdict(list)
    for a in range(len(M) - 2):
        if M[a] == 0xC619 and M[a+1] == 0x8006:
            hits[M[a+2]].append(a)
    print('===', prog, '===')
    for n in sorted(hits):
        print('  %3d (0%03o) %-10s x%-3d  e.g. %05X %s' %
              (n, n, names.get(n, '?'), len(hits[n]), hits[n][0], near(hits[n][0])))
