"""RTE-6/VM (CI hierarchical FMP) extractor for SIMH HP 7920 disc images.

Layout notes (reverse-engineered from hp7920-advent.disc):
  * The image stores 16-bit words byte-swapped relative to host order.
  * The FMP LU starts at image sector 0xC4E0; a block is 128 words = 256 bytes,
    so  byte_offset = (0xC4E0 + block) * 256.
  * The root directory is at block 0x24.  A directory is an array of 64-byte
    entries; entry 0 is the directory's own header record.
  * Entry layout:
       +0  u16 flags        +4  u32 first block
       +16 char[16] name    +32 char[4] extension
       +36/+40/+44 u32 timestamps (Unix seconds)
       +48 u32 blocks allocated   +52 u32 words used   +56 u32 record count
  * Record-structured (type 3/4) files store each record as
       <u16 len> <data> <u16 len>
    where len < 0x8000 means len words (2*len characters) and len >= 0x8000
    means 2*(len & 0x7FFF)+1 characters occupying (len & 0x7FFF)+1 words.
  * Type 1/2 files (RUN images, some libraries) have no record framing;
    they are recognised by words == recs*128 and dumped raw.
"""
import os, sys, struct, re

BASE = 0xC4E0
SEC  = 256
NAMERE = re.compile(r'[A-Za-z0-9_.+#^*&%$@!~-]+$')
MAXSEC = 197520
NUL = chr(0).encode('latin1')

def load(path):
    d = bytearray(open(path,'rb').read())
    d[0::2], d[1::2] = d[1::2], d[0::2]
    return bytes(d)

def off(blk):  return (BASE + blk) * SEC
def u16(d,o): return struct.unpack_from('>H', d, o)[0]
def u32(d,o): return struct.unpack_from('>I', d, o)[0]

class Ent:
    __slots__=('name','ext','start','blocks','words','recs','t1','t2','t3','raw')
    def __init__(self,d,o):
        self.raw    = d[o:o+64]
        self.start  = u32(d,o+4)
        self.name   = d[o+16:o+32].decode('latin1').rstrip()
        self.ext    = d[o+32:o+36].decode('latin1').rstrip()
        self.t1,self.t2,self.t3 = u32(d,o+36),u32(d,o+40),u32(d,o+44)
        self.blocks = u32(d,o+48)
        self.words  = u32(d,o+52)
        self.recs   = u32(d,o+56)
    def isdir(self): return self.ext == 'DIR'
    def empty(self): return self.raw[16:32] == NUL*16
    def bad(self):
        if self.raw[32:34] == bytes([0x0f,0xff]): return True
        if self.name == 'VOLUME HEADER': return True
        if not NAMERE.match(self.name): return True
        if self.ext and not NAMERE.match(self.ext): return True
        if self.start == 0 or self.start + self.blocks > MAXSEC: return True
        return False

def read_dir(d, blk, nblocks=48):
    o = off(blk)
    for i in range(nblocks * SEC // 64):
        e = Ent(d, o + i*64)
        if i == 0 or e.empty(): continue
        yield e

def read_file(d, e):
    """Return (records, ok).  Records are the exact record bytes."""
    o = off(e.start); end = o + e.words*2
    recs=[]; p=o; ok=True
    while p < end - 1 and len(recs) < 500000:
        L = u16(d,p)
        if L & 0x8000:
            nch = 2*(L & 0x7FFF) + 1
            nw  = (L & 0x7FFF) + 1
        else:
            nch, nw = 2*L, L
        if nw > 4096: ok=False; break
        recs.append(d[p+2 : p+2+nch])
        tr = u16(d, p+2+nw*2) if p+2+nw*2+2 <= len(d) else -1
        p += 2 + nw*2 + 2
        if tr != L: ok=False; break
    return recs, ok

def is_binary(e):
    return e.recs and e.words == e.recs*128

def walk(d, blk, blocks, path, out, depth=0, seen=None, log=print):
    seen = seen if seen is not None else set()
    if blk in seen: return
    seen.add(blk)
    for e in read_dir(d, blk, blocks):
        if e.bad(): continue
        if e.isdir():
            sub = os.path.join(path, e.name)
            log('%s[%s]  blk=%06x' % ('  '*depth, e.name, e.start))
            os.makedirs(os.path.join(out,sub), exist_ok=True)
            walk(d, e.start, e.blocks or 48, sub, out, depth+1, seen, log)
            continue
        fn  = (e.name + ('.'+e.ext if e.ext else '')).replace('*','@STAR@')
        dst = os.path.join(out, path, fn)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        raw = d[off(e.start): off(e.start) + max(e.words*2, e.blocks*SEC)]
        if is_binary(e):
            open(dst,'wb').write(raw)
            note = 'binary'
        else:
            recs, ok = read_file(d, e)
            eol = chr(10).encode('latin1')
            body = eol.join(r.replace(NUL, b' ') for r in recs) + eol
            open(dst,'wb').write(body)
            open(dst+'.raw','wb').write(raw)
            note = '' if ok and len(recs)==e.recs else 'IRREG got=%d' % len(recs)
        log('%s%-16s %-4s start=%06x blk=%-5d words=%-7d recs=%-6d %s'
            % ('  '*depth, e.name, e.ext, e.start, e.blocks, e.words, e.recs, note))

if __name__ == '__main__':
    img, out = sys.argv[1], sys.argv[2]
    d = load(img)
    os.makedirs(out, exist_ok=True)
    lines=[]
    def log(s): lines.append(s); print(s)
    walk(d, 0x24, 48, '', out, log=log)
    open(os.path.join(out,'_LISTING.txt'),'w').write(chr(10).join(lines)+chr(10))
