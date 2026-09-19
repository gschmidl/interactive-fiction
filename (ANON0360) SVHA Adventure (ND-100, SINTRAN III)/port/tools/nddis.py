"""ND-100 disassembler (user-mode instruction set).

Written for the Skattejakt port from ND-06.029.1 (ND-110 Instruction Set) with
the nd100x decoder read as a cross-check.  Octal throughout.

usage: nddis.py FILE.PROG START [END]      (addresses in octal)
       as a module: dis(word, addr) -> (text, info)
"""
import struct, sys

REG = ['0', 'D', 'P', 'B', 'L', 'A', 'T', 'X']
MEMOP = {0o00: 'STZ', 0o01: 'STA', 0o02: 'STT', 0o03: 'STX', 0o04: 'STD', 0o05: 'LDD',
         0o06: 'STF', 0o07: 'LDF', 0o10: 'MIN', 0o11: 'LDA', 0o12: 'LDT', 0o13: 'LDX',
         0o14: 'ADD', 0o15: 'SUB', 0o16: 'AND', 0o17: 'ORA', 0o20: 'FAD', 0o21: 'FSB',
         0o22: 'FMU', 0o23: 'FDV', 0o24: 'MPY', 0o25: 'JMP', 0o27: 'JPL'}
CJP = ['JAP', 'JAN', 'JAZ', 'JAF', 'JPC', 'JNC', 'JXZ', 'JXN']
SKPC = ['EQL', 'GEQ', 'GRE', 'MGRE', 'UEQ', 'LSS', 'LST', 'MLST']
ARG = ['SAB', 'SAA', 'SAT', 'SAX', 'AAB', 'AAA', 'AAT', 'AAX']
BOP = ['BSET ZRO', 'BSET ONE', 'BSET BCM', 'BSET BAC', 'BSKP ZRO', 'BSKP ONE', 'BSKP BCM',
       'BSKP BAC', 'BSTC', 'BSTA', 'BLDC', 'BLDA', 'BANC', 'BAND', 'BORC', 'BORA']
STSBIT = {0: 'SSPTM', 1: 'SSTG', 2: 'SSK', 3: 'SSZ', 4: 'SSQ', 5: 'SSO', 6: 'SSC', 7: 'SSM'}
SINGLE = {0o140120: 'ADDD', 0o140121: 'SUBD', 0o140122: 'COMD', 0o140123: 'TSET',
          0o140124: 'PACK', 0o140125: 'UPACK', 0o140126: 'SHDE', 0o140127: 'RDUS',
          0o140130: 'BFILL', 0o140131: 'MOVB', 0o140132: 'MOVBF', 0o140133: 'VERSN',
          0o140134: 'INIT', 0o140135: 'ENTR', 0o140136: 'LEAVE', 0o140137: 'ELEAV',
          0o150400: 'OPCOM', 0o150401: 'IOF', 0o150402: 'ION', 0o150404: 'POF',
          0o150405: 'PIOF', 0o150406: 'SEX', 0o150407: 'REX', 0o150410: 'PON',
          0o150412: 'PION', 0o150415: 'IOXT', 0o150416: 'EXAM', 0o150417: 'DEPO',
          0o142700: 'GECO', 0o143200: 'MIX3', 0o146142: 'EXIT'}


def sx8(v):
    v &= 0o377
    return v - 256 if v & 0o200 else v


def ea_text(w, addr):
    """Operand text for a memory instruction, plus the P-relative location it names."""
    # mode bits: 4 = ,X  2 = I  1 = ,B
    #   0 P+d   1 B+d   2 (P+d)   3 (B+d)   4 X+d   5 B+d+X   6 (P+d)+X   7 (B+d)+X
    d = sx8(w)
    mode = (w >> 8) & 7
    if mode in (0, 2, 6):
        loc = (addr + d) & 0xFFFF
        txt = ('I ' if mode != 0 else '') + '%06o' % loc + (',X' if mode == 6 else '')
        return txt, loc
    if mode == 4:
        return '%d,X' % d, None
    txt = ('I ' if mode in (3, 7) else '') + '%d,B' % d + (',X' if mode in (5, 7) else '')
    return txt, None


def dis(w, addr):
    info = {}
    op5 = w >> 11
    if op5 in MEMOP:
        txt, loc = ea_text(w, addr)
        info.update(mem=MEMOP[op5], mode=(w >> 8) & 7, loc=loc)
        return '%s %s' % (MEMOP[op5], txt), info
    if op5 == 0o26:
        tgt = (addr + sx8(w)) & 0xFFFF
        info['cjp'] = tgt
        return '%s %06o' % (CJP[(w >> 8) & 7], tgt), info
    if w in SINGLE:
        return SINGLE[w], info
    top = w & 0o174000
    if top == 0o140000:
        if (w & 0o300) == 0:
            info['skip'] = True
            return 'SKP IF D%s %s S%s' % (REG[w & 7], SKPC[(w >> 8) & 7], REG[(w >> 3) & 7]), info
        sub = w & 0o177700
        if sub == 0o140600:
            return 'EXR S%s' % REG[(w >> 3) & 7], info
        if sub == 0o141200:
            return 'RMPY S%s D%s' % (REG[(w >> 3) & 7], REG[w & 7]), info
        if sub == 0o141600:
            return 'RDIV S%s' % REG[(w >> 3) & 7], info
        if sub == 0o142200:
            return 'LBYT', info
        if sub == 0o142600:
            return 'SBYT', info
        if sub == 0o143100:
            return 'MOVEW %o' % (w & 0o77), info
        return 'DATA %06o' % w, info
    if top == 0o144000:
        rad = (w >> 10) & 1
        sr, dr = (w >> 3) & 7, w & 7
        info['regop'] = (rad, sr, dr)
        if rad:
            f = (w >> 6) & 0o17        # bit9 ADC? bit8 AD1 bit7 CM1 bit6 CLD
            mods = []
            if w & 0o1000: mods.append('ADC')
            if w & 0o400: mods.append('AD1')
            if w & 0o200: mods.append('CM1')
            if w & 0o100: mods.append('CLD')
            name = 'RADD'
            if mods == ['CLD']:
                name, mods = 'COPY', []
            txt = ' '.join([name] + mods + ['S' + REG[sr], 'D' + REG[dr]])
        else:
            name = ['SWAP', 'RAND', 'REXO', 'RORA'][(w >> 8) & 3]
            mods = []
            if w & 0o200: mods.append('CM1')
            if w & 0o100: mods.append('CLD')
            txt = ' '.join([name] + mods + ['S' + REG[sr], 'D' + REG[dr]])
        return txt, info
    if (w & 0o177760) in (0o150000, 0o150100, 0o150200, 0o150300):
        return ['TRA', 'TRR', 'MCL', 'MST'][(w >> 6) & 3] + ' %o' % (w & 0o17), info
    hi8 = w & 0o177400
    if hi8 == 0o151000:
        return 'WAIT %o' % (w & 0o377), info
    if hi8 == 0o151400:
        return 'NLZ %d' % sx8(w), info
    if hi8 == 0o152000:
        return 'DNZ %d' % sx8(w), info
    if hi8 == 0o153000:
        info['mon'] = w & 0o377
        return 'MON %o' % (w & 0o377), info
    if hi8 in (0o152400, 0o152600):
        return ('SRB' if hi8 == 0o152400 else 'LRB') + ' %o' % (w & 0o377), info
    if hi8 in (0o153400, 0o153600):
        return ('IRW' if hi8 == 0o153400 else 'IRR') + ' %o' % (w & 0o377), info
    if top == 0o154000:
        reg = ['SHT', 'SHD', 'SHA', 'SAD'][(w >> 7) & 3]
        typ = ['', 'ROT ', 'ZIN ', 'LIN '][(w >> 9) & 3]
        cnt = w & 0o77
        if cnt & 0o40:
            cnt -= 0o100
        return '%s %s%d' % (reg, typ, cnt), info
    if top in (0o160000, 0o164000):
        return ('IOT' if top == 0o160000 else 'IOX') + ' %o' % (w & 0o3777), info
    if top == 0o170000:
        return '%s %d' % (ARG[(w >> 8) & 7], sx8(w)), info
    if top == 0o174000:
        f = (w >> 7) & 0o17
        bn = (w >> 3) & 0o17
        dr = w & 7
        loc = STSBIT.get(bn, 'STS%o' % bn) if dr == 0 else '%o D%s' % (bn, REG[dr])
        if f in (4, 5, 6, 7):
            info['skip'] = True
        return '%s %s' % (BOP[f], loc), info
    return 'DATA %06o' % w, info


def load_prog(path):
    d = open(path, 'rb').read()
    hdr = struct.unpack_from('>7H', d, 0)
    mem = [0] * 65536
    first, last = hdr[2], hdr[3]
    n = min(last - first + 1, (len(d) - 0x200) // 2)
    for i, w in enumerate(struct.unpack_from('>%dH' % n, d, 0x200)):
        mem[first + i] = w
    return hdr, mem


def chars(w):
    return ''.join(chr(c & 0x7f) if 32 <= (c & 0x7f) < 127 else '.' for c in (w >> 8, w & 0xff))


def dump(mem, start, end):
    for a in range(start, end + 1):
        w = mem[a]
        txt, info = dis(w, a)
        print('%06o  %06o  %s  %s' % (a, w, chars(w), txt))


if __name__ == '__main__':
    hdr, mem = load_prog(sys.argv[1])
    s = int(sys.argv[2], 8)
    e = int(sys.argv[3], 8) if len(sys.argv) > 3 else s + 0o40
    dump(mem, s, e)
