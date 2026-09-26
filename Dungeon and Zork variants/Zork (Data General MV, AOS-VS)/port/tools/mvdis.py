import io, re, struct, sys, os
from st import read_st, load_pr, SRC

ops = []
MVOPS = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                     '..', 'src32', 'mvops.h')
for L in io.open(MVOPS, encoding='utf-8'):
    m = re.match(r'\s*\{ 0x([0-9A-F]{4}), "([A-Z0-9]+)", 0x([0-9A-F]{2}), (\d) \}', L)
    if m: ops.append((int(m.group(1), 16), m.group(2), int(m.group(3), 16), int(m.group(4))))
MASKS = [0xFFFF, 0xE7FF, 0x9FFF, 0x87FF, 0xFFCF, 0xE7CF, 0x9FCF, 0x87CF]
byop = {}
for o in ops: byop.setdefault(o[0], o)

def find(ir):
    for mk in MASKS:
        b = ir & mk
        if b in byop: return byop[b]
    return None

IDXHI = {0x13,0x14,0x15,0x1A,0x26,0x27,0x28,0x2E}
def operands(ir, pc, M, o):
    op, nm, ty, ln = o
    acs, acd = (ir >> 13) & 3, (ir >> 11) & 3
    idx = acs if ty in IDXHI else acd
    ixn = ['', ',PC', ',2', ',3'][idx]
    if ty in (0x0F, 0x10): return '%d,%d' % (acs, acd)
    if ty == 0x09:         return '%d,%d' % (acs + 1, acd)
    if ty == 0x08:         return '%d' % acd
    if ty in (0x05,):      return '%d' % M[pc+1]
    if ty in (0x2E,0x26,0x27,0x28,0x13,0x14,0x15,0x1A,0x2B,0x2C,0x18,0x19,0x06,0x07,0x04,0x03,0x02):
        if ln == 3 and ty in (0x13,0x14,0x15,0x1A,0x18,0x19,0x17):
            d = (M[pc+1] << 16) | M[pc+2]
            ind = '@' if d & 0x80000000 else ''
            d &= 0x7FFFFFFF
            if d & 0x40000000: d -= 0x80000000
        else:
            d = M[pc+1]; ind = '@' if d & 0x8000 else ''
            d &= 0x7FFF
            if d & 0x4000: d -= 0x8000
        tgt = ''
        if idx == 1: tgt = '   ;%05X' % ((pc + 1 + d) & 0xFFFFF)
        elif idx == 0: tgt = '   ;%05X' % (d & 0xFFFFF)
        if ty in (0x0F,): pass
        acstr = '%d,' % acd if ty in (0x2E,0x26,0x27,0x28,0x13,0x14,0x15,0x1A,0x06,0x07,0x04) else ''
        return '%s%s%d%s%s' % (acstr, ind, d, ixn, tgt)
    if ty in (0x0B,0x0C): return '%d,%d' % (acd, M[pc+1])
    if ty in (0x11,0x12): return '%d,%d' % (acd, (M[pc+1] << 16) | M[pc+2])
    if ty == 0x16:
        t = (M[pc+1] << 16) | M[pc+2]
        return '%05X,%d' % (t & 0xFFFFF, M[pc+3])
    if ty == 0x29:
        d = M[pc+1]; d = d - 0x10000 if d & 0x8000 else d
        return '%05X,%d' % ((pc + 1 + d) & 0xFFFFF, M[pc+2])
    return ''

def dis(M, lo, hi, symbols=None):
    rev = {}
    if symbols:
        for k, vs in symbols.items():
            for v in vs: rev.setdefault(v & 0x0FFFFFFF, []).append(k)
    pc = lo
    out = []
    while pc < hi:
        if pc in rev: out.append('%s:' % ' '.join(rev[pc]))
        ir = M[pc]
        o = find(ir) if (ir & 0x8000 and (ir & 0xF) in (8, 9)) else None
        if o:
            ln = o[3]
            words = ' '.join('%04X' % M[pc+k] for k in range(ln))
            out.append('%05X  %-15s %-7s %s' % (pc, words, o[1], operands(ir, pc, M, o)))
            pc += ln
        else:
            out.append('%05X  %-15s %s' % (pc, '%04X' % ir, narrow(ir)))
            pc += 1
    return '\n'.join(out)

NOVA_ALC = ['COM','NEG','MOV','INC','ADC','SUB','ADD','AND']
SKIPS = ['','SKP','SZC','SNC','SZR','SNR','SEZ','SBN']
def narrow(ir):
    if ir & 0x8000:
        acs, acd = (ir >> 13) & 3, (ir >> 11) & 3
        f = NOVA_ALC[(ir >> 8) & 7]
        sh = ['', 'L', 'R', 'S'][(ir >> 6) & 3]
        cy = ['', 'Z', 'O', 'C'][(ir >> 4) & 3]
        nl = '#' if ir & 8 else ''
        sk = SKIPS[ir & 7]
        return '%s%s%s%s %d,%d%s' % (f, cy, sh, nl, acs, acd, ',' + sk if sk else '')
    op = (ir >> 11) & 7
    nm = ['JMP','JSR','ISZ','DSZ','LDA','STA',None,None][op]
    if nm is None: return '.word %04X' % ir
    ind = '@' if ir & 0x400 else ''
    mode = (ir >> 8) & 3
    d = ir & 0xFF
    if d & 0x80: d -= 0x100
    ac = (ir >> 11) & 3
    if op >= 4: return '%s %d,%s%d%s' % (nm, ac, ind, d, ['', ',PC', ',2', ',3'][mode])
    return '%s %s%d%s' % (nm, ind, d, ['', ',PC', ',2', ',3'][mode])

if __name__ == '__main__':
    prog = sys.argv[1]
    lo, hi = int(sys.argv[2], 16), int(sys.argv[3], 16)
    if os.path.exists(prog):
        M, _ = load_pr(prog); syms = None
    else:
        M, _ = load_pr(os.path.join(SRC, prog + '.PR'))
        syms = read_st(os.path.join(SRC, prog + '.ST'))
    print(dis(M, lo, hi, syms))
