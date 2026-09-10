# -*- coding: utf-8 -*-
"""Pull the documents out of the Wang OIS disk image.

The disk is 616 sectors of 512 bytes, addressed here as 1232 blocks of 256.
Every allocated block carries a **7-byte header** and 249 bytes of payload:

    byte 0   major block number
    byte 1   sub-block number (0..15)   -> logical order is major*16 + sub
    byte 2   check byte
    byte 3   type/flags
    byte 4   0
    byte 5   document number
    byte 6   0x20

Free blocks are filled with 0xAA, which is what makes them easy to skip.

**Order the blocks by where they sit on the disk, not by their numbers.**  Every
document carries one or two blocks numbered 0.0, and those are *not* its first
blocks -- they come physically after a run and continue it.  Sorting by number
files them at the front, which silently moves a chunk of text from the end of
the document to the beginning.  Document 0x01 shows what that costs: its 0.0
block holds `" up the sign).  He takes your treasures..."`, the continuation of
the last glossary entry, and by number it lands 60 kB earlier, where it looks
like a damaged opening.  In physical order every one of the 89 glossary entry
markers still falls at offset 64 of a block -- the numbering is otherwise
contiguous and agrees with it -- and the sentence joins up.

Three documents matter:

    0x10  "MISTY MARSH GAME / OIS VERSION"  the playing document
    0x59  "MISTY MARSH GAME / WP VERSION"   document + glossary, compact form
    0x01  the glossary as a readable listing, keystrokes spelled (-LIKE-THIS-)

Text is stored with **bit 7 set** on every character, so the payload has to be
masked with 0x7F before it reads as ASCII.  Bytes below 0x20 are structure and
are left alone: 0x02 is a tab stop in a format line, 0x03 a carriage return, and
0x03 '(' key ')' 0x03 marks the start of a glossary entry.

    python extract.py ../../src_original/mistymarsh.img ../data
"""
import os
import sys
import collections

BLOCK = 256
HDRLEN = 7
PAYLOAD = BLOCK - HDRLEN
FREE = 0xAA


def read_blocks(path):
    """Every allocated block, in disk order, as (document, major, sub, payload)."""
    raw = open(path, "rb").read()
    out = []
    for off in range(0, len(raw), BLOCK):
        h = raw[off:off + HDRLEN]
        if len(h) < HDRLEN or (h[0] == FREE and h[1] == FREE):
            continue
        out.append((h[5], h[0], h[1], raw[off + HDRLEN:off + BLOCK]))
    return out


def documents(path):
    """Document number -> payload, blocks in disk order (see the note above)."""
    docs = collections.defaultdict(list)
    for doc, major, sub, payload in read_blocks(path):
        docs[doc].append((major, sub, payload))
    return {d: b"".join(p for _, _, p in blocks) for d, blocks in docs.items()}


def main():
    img = sys.argv[1] if len(sys.argv) > 1 else "../../src_original/mistymarsh.img"
    outdir = sys.argv[2] if len(sys.argv) > 2 else "../data"
    os.makedirs(outdir, exist_ok=True)
    docs = documents(img)
    for doc in sorted(docs):
        body = docs[doc]
        if len(body) < 4096:            # catalogue fragments, not documents
            continue
        name = os.path.join(outdir, "doc%02X.bin" % doc)
        open(name, "wb").write(body)
        print("%s  %6d bytes  (%d blocks)" % (name, len(body), len(body) // PAYLOAD))


if __name__ == "__main__":
    main()
