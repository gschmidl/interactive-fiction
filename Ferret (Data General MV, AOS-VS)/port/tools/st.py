import struct, sys, os
# The AOS/VS :UTIL files that came with the Thissala export.  Defaults to the
# sibling Thissala project in this repository; set THISSALA_SRC to override.
SRC = os.environ.get(
    'THISSALA_SRC',
    os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..',
                 'Thissala (Data General Eclipse, AOS-VS)', 'src_original'))

def read_st(path):
    d = open(path, 'rb').read()
    syms = {}
    i = 0
    while i + 20 <= len(d):
        if d[i] in (0x20, 0x22, 0x21, 0x23, 0x24, 0x26) and d[i+1] and d[i+1] < 33 \
           and d[i+6:i+8] == b'\xff\xff':
            n = d[i+1]
            name = d[i+20:i+20+n]
            if all(33 <= c < 127 for c in name):
                val = struct.unpack('>I', d[i+2:i+6])[0]
                syms.setdefault(name.decode(), []).append(val)
                i += 20 + n + (n & 1)
                continue
        i += 2
    return syms

def load_pr(path):
    raw = open(path, 'rb').read()
    fw = struct.unpack('>%dH' % (len(raw)//2), raw[:len(raw)//2*2])
    PAGEW = 1024
    nblk = len(fw)//PAGEW
    bl, st, sz = fw[0x100+0x0C], fw[0x100+0x0F], fw[0x100+0x13]
    if fw[0x100+0x14] & 0x8000:
        raise ValueError('16-bit program')
    sblk = nblk - sz
    M = [0]*max(0x100000, (st + sz) * PAGEW)
    for i in range(sz*PAGEW):
        M[st*PAGEW+i] = fw[sblk*PAGEW+i]
    for i in range(min(bl*PAGEW, len(fw)-0x2000)):
        M[i] = fw[0x2000+i]
    return M, (bl, st, sz)

if __name__ == '__main__':
    name = sys.argv[1]
    syms = read_st(os.path.join(SRC, name + '.ST'))
    M, info = load_pr(os.path.join(SRC, name + '.PR'))
    print(name, 'blocks', info, 'symbols', len(syms))
    pat = [0xA739,0x0011,0x8699,0x0000,0x00FF,0x86A9,0x0000,0x00A0,0x8B79,
           0xF409,0x0004,0x95B9,0xF319,0x0002,0xDB79,0xE6B9]
    hit = None
    for a in range(0x1000, len(M)-len(pat)):
        if M[a:a+len(pat)] == pat:
            hit = a; break
    print('routine at %05X' % hit if hit is not None else 'not found')
    if hit is None: sys.exit()
    # nearest symbol at or below
    flat = sorted((v & 0x0FFFFFFF, k) for k, vs in syms.items() for v in vs)
    for addr, nm in flat:
        if addr == hit: print('EXACT  %05X  %s' % (addr, nm))
    below = [x for x in flat if x[0] <= hit]
    above = [x for x in flat if x[0] > hit]
    print('--- 6 symbols at or below ---')
    for addr, nm in below[-6:]: print('  %05X  %-30s  (-%d)' % (addr, nm, hit-addr))
    print('--- 6 symbols above ---')
    for addr, nm in above[:6]: print('  %05X  %-30s  (+%d)' % (addr, nm, addr-hit))
