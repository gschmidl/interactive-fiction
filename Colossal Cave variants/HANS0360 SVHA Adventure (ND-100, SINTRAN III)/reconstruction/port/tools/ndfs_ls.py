"""Minimal read-only NDFS (SINTRAN III) directory lister/extractor.
Written from the NDFS-FORMAT.md spec; does not modify the image."""
import sys, struct, os

PAGE = 2048

class Img:
    def __init__(self, path):
        self.d = open(path, 'rb').read()
    def page(self, n):
        return self.d[n*PAGE:(n+1)*PAGE]

def bp(v):
    return (v >> 30) & 3, v & 0x3FFFFFFF

def name(b):
    s = bytearray()
    for c in b:
        c &= 0x7F
        if c == 0x27: break
        s.append(c)
    return s.decode('ascii', 'replace')

def ndtime(v):
    if v == 0: return '-'
    y = (v >> 26) + 1950; m = (v >> 22) & 15; d = (v >> 17) & 31
    h = (v >> 12) & 31; mi = (v >> 6) & 63; s = v & 63
    return f'{y:04d}-{m:02d}-{d:02d} {h:02d}:{mi:02d}'

def data_pages(img, ptr, npages=None):
    t, blk = bp(ptr)
    if blk == 0: return []
    if t == 0:
        return list(range(blk, blk + (npages or 1)))
    if t == 1:
        ib = img.page(blk)
        return [bp(struct.unpack_from('>I', ib, i*4)[0])[1] for i in range(512)]
    if t == 2:
        sb = img.page(blk); out = []
        for i in range(512):
            p = struct.unpack_from('>I', sb, i*4)[0]
            if bp(p)[1] == 0:
                out += [0]*512; continue
            ib = img.page(bp(p)[1])
            out += [bp(struct.unpack_from('>I', ib, j*4)[0])[1] for j in range(512)]
        return out
    raise ValueError('bad ptr type')

def read_file(img, ptr, npages, nbytes):
    t, blk = bp(ptr)
    pages = data_pages(img, ptr, npages)
    if t != 0:
        pages = pages[:npages] if npages else pages
    buf = bytearray()
    for p in pages[:npages]:
        buf += img.page(p) if p else bytes(PAGE)
    return bytes(buf[:nbytes])

def main():
    img = Img(sys.argv[1])
    p0 = img.page(0)
    mb = p0[0x7E0:0x800]
    dname = name(mb[:16])
    objp, userp, bitp, unres = struct.unpack_from('>IIII', mb, 16)
    print('directory', dname, 'obj', bp(objp), 'user', bp(userp), 'bit', bp(bitp))
    users = {}
    upages = data_pages(img, userp)
    for pi, pg in enumerate(upages[:8]):
        if pg == 0: continue
        page = img.page(pg)
        for e in range(32):
            ent = page[e*64:(e+1)*64]
            if (ent[0] & 0x81) == 0x81:
                users[ent[37]] = name(ent[2:18])
    opages = data_pages(img, objp)
    want = [a.upper() for a in sys.argv[2:]]
    files = []
    for pi, pg in enumerate(opages):
        if pg == 0: continue
        page = img.page(pg)
        for e in range(32):
            ent = page[e*64:(e+1)*64]
            hdr = struct.unpack_from('>H', ent, 0)[0]
            if not (hdr & 0x8000): continue
            oname = name(ent[2:18]); otype = name(ent[18:22])
            acc, ftf = struct.unpack_from('>HH', ent, 26)
            oidx = struct.unpack_from('>H', ent, 34)[0]
            created, rd, wr, npg, nb1, fptr = struct.unpack_from('>IIIIII', ent, 40)
            owner = users.get(oidx >> 8, '?%d' % (oidx >> 8))
            files.append((owner, oname, otype, npg, nb1 + 1, fptr, created, wr, ftf, acc))
    for f in files:
        if want and f[0].upper() not in want: continue
        print(f'{f[0]:>16}/{f[1]}:{f[2]:<4} pages={f[3]:<5} bytes={f[4]:<8} ptr={bp(f[5])} flags={f[8]:#06x} acc={f[9]:#06o} created={ndtime(f[6])} written={ndtime(f[7])}')
    return img, files

if __name__ == '__main__':
    main()

def extract(image, owner, fname, ftype, out):
    img = Img(image)
    mb = img.page(0)[0x7E0:0x800]
    objp, userp, bitp, unres = struct.unpack_from('>IIII', mb, 16)
    users = {}
    for pg in data_pages(img, userp)[:8]:
        if pg == 0: continue
        page = img.page(pg)
        for e in range(32):
            ent = page[e*64:(e+1)*64]
            if (ent[0] & 0x81) == 0x81: users[ent[37]] = name(ent[2:18])
    for pg in data_pages(img, objp):
        if pg == 0: continue
        page = img.page(pg)
        for e in range(32):
            ent = page[e*64:(e+1)*64]
            if not (struct.unpack_from('>H', ent, 0)[0] & 0x8000): continue
            oidx = struct.unpack_from('>H', ent, 34)[0]
            if users.get(oidx >> 8) == owner and name(ent[2:18]) == fname and name(ent[18:22]) == ftype:
                created, rd, wr, npg, nb1, fptr = struct.unpack_from('>IIIIII', ent, 40)
                data = read_file(img, fptr, npg, nb1 + 1)
                open(out, 'wb').write(data)
                return data
    raise KeyError(fname)
