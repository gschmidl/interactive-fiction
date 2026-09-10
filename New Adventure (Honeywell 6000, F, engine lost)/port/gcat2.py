"""Full GCOS/TSS archive extractor for the ar073.252x tape members.

Fixes the two bugs in the original gcat.c:
  * the archive is stored as repeated 3855-word segments (15-word header +
    12 blocks of 320 words), each byte-padded -- so every segment after the
    first sits at a 4-bit offset from the previous one.  gcat.c read only
    segment 0 and then walked off into noise.
"""
import sys

SEG_WORDS  = 0o7417          # 3855 words per archive segment
SEG_BITS   = SEG_WORDS * 36  # 138780
SEG_STRIDE = ((SEG_BITS + 7) // 8) * 8   # padded to a byte boundary: 138784
HDR_WORDS  = 0o17            # 15-word member header
BLOCK      = 320

class Bits:
    def __init__(self, data):
        self.b = int.from_bytes(data, 'big')
        self.n = len(data) * 8
    def get(self, pos, width):
        if pos + width > self.n: raise EOFError
        return (self.b >> (self.n - pos - width)) & ((1 << width) - 1)

def bcd(w, out):
    for sh in (27, 18, 9, 0):
        c = (w >> sh) & 0o777
        if c == 0o177: return True      # record padding: rest of word is dead
        out.append(chr(c) if c < 0o200 else '?')
    return False

def extract(fn):
    data = open(fn, 'rb').read()
    bs = Bits(data)
    lines, name = [], None
    seg = 0
    while seg * SEG_STRIDE < bs.n:
        base = seg * SEG_STRIDE
        W = lambda i: bs.get(base + i * 36, 36)
        if seg == 0:
            hdr = []
            for i in range(2, HDR_WORDS): bcd(W(i), hdr)
            name = ''.join(hdr).split('/')[-1].strip('?').replace(chr(0),'').strip()
        p = HDR_WORDS
        while p < SEG_WORDS:
            try: bcw = W(p)
            except EOFError: return name, lines
            bsize = bcw & 0o777777
            q, end = p + 1, p + 1 + min(bsize, BLOCK - 1)
            while q < end:
                try: rcw = W(q)
                except EOFError: return name, lines
                q += 1
                size = (rcw >> 18) & 0o777777
                mark = (rcw >> 12) & 0o17
                code = (rcw >> 6) & 0o17
                if size == 0 and mark in (0o17, 0o23):
                    return name, lines            # true end-of-file marker
                if code in (5, 6):                # TSS / standard ASCII
                    out = []
                    for k in range(size):
                        try:
                            if bcd(W(q + k), out): break
                        except EOFError: break
                    lines.append(''.join(out))
                q += size
            p += BLOCK
        seg += 1
    return name, lines

for fn in sys.argv[1:]:
    name, lines = extract(fn)
    print("%-16s -> %-10s %4d lines, %6d bytes" % (fn.split('/')[-1], name, len(lines),
                                                   sum(len(l) + 1 for l in lines)))
    open(name, 'w', newline='\n').write('\n'.join(lines) + '\n')
