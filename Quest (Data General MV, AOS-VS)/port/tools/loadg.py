#!/usr/bin/env python3
"""Extract an AOS/VS DUMP_II stream carried in a SIMH .tap image.

Record header is one 16-bit big-endian word: 6-bit type, 10-bit length.
"""
import os, struct, sys

SOD, FSB, NB, UDA, ACL, LINK, START, DATA, END, EOD = range(10)
TYPES = {SOD:'SOD',FSB:'FSB',NB:'NB',UDA:'UDA',ACL:'ACL',LINK:'LINK',
         START:'START',DATA:'DATA',END:'END',EOD:'EOD'}
FSTAT = {0:'FLNK',1:'FDSF',2:'FMTF',3:'FGFN',10:'FDIR',11:'FLDU',12:'FCPD',
         64:'FUDF',66:'FUPD',67:'FSTF',68:'FTXT',69:'FLOG',74:'FPRV',87:'FPRG'}
ISDIR = {10,11,12}

def tape_files(path):
    """Yield the concatenated payload of each tape file (filemark separated)."""
    d = open(path,'rb').read()
    off = 0; cur = []
    while off < len(d):
        (ln,) = struct.unpack_from('<I', d, off)
        if ln in (0xffffffff, 0xfffffffe):
            break
        if ln == 0:                       # filemark
            off += 4
            if cur: yield b''.join(cur); cur = []
            continue
        pad = ln + (ln & 1)
        cur.append(d[off+4:off+4+ln])
        off += 8 + pad
    if cur: yield b''.join(cur)

class Stream:
    def __init__(self, blob): self.b = blob; self.p = 0
    def take(self, n):
        if self.p + n > len(self.b): raise EOFError
        r = self.b[self.p:self.p+n]; self.p += n; return r
    def header(self):
        two = self.take(2)
        return (two[0] >> 2, (two[0] & 3) << 8 | two[1])
    def eof(self): return self.p >= len(self.b)

def clean(name):
    """AOS/VS names are legal-ish already; make them safe for Windows."""
    return ''.join(c if c not in '<>:"/\|?*' else '_' for c in name)

def extract(blob, outroot, log):
    s = Stream(blob)
    dirs = []           # directory stack
    fname = None; ftype = None; payload = None; fpath = None
    listing = []
    while not s.eof():
        try: rtype, rlen = s.header()
        except EOFError: break
        if rtype == SOD:
            sod = struct.unpack('>7H', s.take(rlen))
            log(f"SOD rev {sod[0]} {sod[4]:02d}-{sod[5]:02d}-{sod[6]:02d} "
                f"{sod[3]:02d}:{sod[2]:02d}:{sod[1]:02d}")
        elif rtype == FSB:
            fsb = s.take(rlen)
            ftype = fsb[1]
        elif rtype == NB:
            fname = s.take(rlen).rstrip(b'\0').decode('latin-1')
        elif rtype in (UDA, ACL):
            s.take(rlen)
        elif rtype == LINK:
            target = s.take(rlen).rstrip(b'\0').decode('latin-1')
            rel = '/'.join(dirs + [fname])
            listing.append(('LINK', rel, target, 0))
            log(f"  LINK {rel} => {target}")
            fname = None
        elif rtype == START:
            s.take(rlen)
            if ftype in ISDIR:
                dirs.append(clean(fname))
                os.makedirs(os.path.join(outroot, *dirs), exist_ok=True)
                log(f"DIR  {'/'.join(dirs)}")
                fname = None
            else:
                payload = bytearray()
                fpath = '/'.join(dirs + [fname])
        elif rtype == DATA:
            hdr = s.take(rlen)          # 10 bytes
            addr, blen = struct.unpack_from('>II', hdr, 0)
            align, = struct.unpack_from('>H', hdr, 8)
            s.take(align)
            data = s.take(blen)
            if payload is not None:
                if len(payload) < addr: payload.extend(b'\0' * (addr - len(payload)))
                payload[addr:addr+blen] = data
        elif rtype == END:
            s.take(rlen)
            if payload is not None:
                dest = os.path.join(outroot, *(clean(p) for p in fpath.split('/')))
                os.makedirs(os.path.dirname(dest), exist_ok=True)
                open(dest,'wb').write(payload)
                listing.append((FSTAT.get(ftype, str(ftype)), fpath, '', len(payload)))
                log(f"  {FSTAT.get(ftype,str(ftype)):5s} {len(payload):9d}  {fpath}")
                payload = None; fname = None; fpath = None
            elif dirs:
                dirs.pop()
        elif rtype == EOD:
            s.take(rlen); log("EOD"); break
        else:
            log(f"?? type {rtype} len {rlen} at {s.p}"); s.take(rlen)
    return listing

if __name__ == '__main__':
    tap, out = sys.argv[1], sys.argv[2]
    os.makedirs(out, exist_ok=True)
    logf = open(os.path.join(out, '_extract.log'), 'w')
    def log(m): logf.write(m + '\n')
    allrows = []
    for i, blob in enumerate(tape_files(tap)):
        log(f"===== tape file {i}  ({len(blob)} bytes) =====")
        sub = os.path.join(out, f'tf{i:02d}')
        try:
            allrows += extract(blob, sub, log)
        except EOFError:
            log("  *** truncated ***")
    logf.close()
    print(f"{len(allrows)} entries")
