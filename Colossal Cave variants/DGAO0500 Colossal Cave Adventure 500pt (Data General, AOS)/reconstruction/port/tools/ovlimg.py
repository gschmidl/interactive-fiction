"""Build one memory image per overlay of ADVENTURE.PR for dis.exe.

The original AOS .PR is the 32K-word address space itself.  Its 13 overlays
are 1024 words each in ADVENTURE.OL, overlay n at file block 4n, and all of
them run at 6800 (see NOTES.md).  Each image written is the .PR with one
overlay laid over 6800..6BFF, so

    python tools/ovlimg.py data notes
    ./dis.exe notes/ovl00.img 0 68F3 6934

disassembles SPEAK in place.  The images are not kept: rebuild them.
"""
import sys

src = sys.argv[1] if len(sys.argv) > 1 else 'data'
dst = sys.argv[2] if len(sys.argv) > 2 else 'notes'
pr = open(src + '/ADVENTURE.PR', 'rb').read()
ol = open(src + '/ADVENTURE.OL', 'rb').read()
for n in range(len(ol) // 2048):          # 1024 words = four 512-byte blocks
    img = bytearray(pr)
    img[0x6800 * 2:0x6800 * 2 + 2048] = ol[n * 2048:(n + 1) * 2048]
    open('%s/ovl%02d.img' % (dst, n), 'wb').write(img)
    print('%s/ovl%02d.img' % (dst, n))
