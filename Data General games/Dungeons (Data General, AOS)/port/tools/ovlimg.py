"""Build one memory image per overlay of DG.PR for dis.exe.

DG.PR is an original AOS program: the file is the 32K-word address space
itself.  DG.OL holds 4 overlays of 2048 words (eight 512-byte blocks), all
running at 3000 (see NOTES.md).  Each image written is the .PR with one
overlay laid over 3000..37FF:

    python tools/ovlimg.py data notes
    ./dis.exe notes/ovl1.img 0 33E0 3445

The images are not kept: rebuild them.
"""
import sys

src = sys.argv[1] if len(sys.argv) > 1 else 'data'
dst = sys.argv[2] if len(sys.argv) > 2 else 'notes'
pr = open(src + '/DG.PR', 'rb').read()
ol = open(src + '/DG.OL', 'rb').read()
for n in range(len(ol) // 4096):            # 2048 words = eight 512-byte blocks
    img = bytearray(pr)
    img[0x3000 * 2:0x3000 * 2 + 4096] = ol[n * 4096:(n + 1) * 4096]
    open('%s/ovl%d.img' % (dst, n), 'wb').write(img)
    print('%s/ovl%d.img' % (dst, n))
