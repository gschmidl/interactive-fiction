"""Name a stripped .PR's runtime routines by matching their code against the
AOS/VS utilities, which shipped with their .ST symbol tables."""
import glob, os, sys, struct
from st import read_st, load_pr, SRC

def index(M, n):
    """map an n-word tuple -> list of addresses"""
    d = {}
    for a in range(len(M) - n):
        d.setdefault(tuple(M[a:a+n]), []).append(a)
    return d

def main(target, donors, n=12):
    Mt, _ = load_pr(target)
    idx = {}
    for a in range(0x1000, len(Mt) - n):
        if any(Mt[a:a+n]):
            idx.setdefault(tuple(Mt[a:a+n]), []).append(a)
    found = {}
    for dn in donors:
        pr, st = os.path.join(SRC, dn + '.PR'), os.path.join(SRC, dn + '.ST')
        if not (os.path.exists(pr) and os.path.exists(st)): continue
        try: Md, _ = load_pr(pr)
        except Exception: continue
        syms = read_st(st)
        for name, vals in syms.items():
            for v in vals:
                a = v & 0x0FFFFFFF
                if a == 0x0FFFFFFF or a >= len(Md) - n: continue
                key = tuple(Md[a:a+n])
                if not any(key): continue
                hits = idx.get(key)
                if hits and len(hits) == 1:
                    found.setdefault(hits[0], set()).add(name)
    return found

if __name__ == '__main__':
    target = sys.argv[1]
    donors = sys.argv[2:] or ['SCOM','BRAN','DISPLAY','XBAT','XMNT','LFCOPY',
                              'DISCO','CPIO','BROWSE','EXEC','MSCOPY','TAR',
                              'HISTOREPORT','MIRRORINFO','SED','FED','LINK',
                              'MASM','PED','SPRED','QCMP','FILCOM','PATCH',
                              'STACKER','SMI','LOGCALLS','CONVERT','FCU','LFE']
    f = main(target, donors)
    for a in sorted(f):
        print('%05X  %s' % (a, ' '.join(sorted(f[a]))))
    print('%d routines named' % len(f), file=sys.stderr)
