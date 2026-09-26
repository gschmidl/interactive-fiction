#!/usr/bin/env python3
"""
Pull ANKH1's and CHIP1's data files off the PRIMOS disk image.

The two Pascal programs open four files at run time -- LEXICON and ROADS for
ANKH1, LEX2 and ROAD2 for CHIP1 -- and none of them were in the directory of
recovered sources.  They were still on the emulator's disk pack.

How the image is laid out
-------------------------
The pack is 83632 records of 2080 bytes: a 16-halfword record header followed
by 1024 halfwords of data.  Everything on a Prime is big-endian.

Record header words used here:
    [1]  this record's own record address (RA)
    [4]  number of data halfwords in this record
    [7]  RA of the next record in the file, or 0 at the end

The directory (UFD) entry for a file is a run of halfwords beginning with an
entry control word (type << 8 | length in halfwords).  Word 2 of the entry is
the file's first record address; the file name sits in the last words of the
entry, one character per byte with the high bit set, which is how PRIMOS
stores text.  That high bit is why a plain grep for "ROADS" finds nothing.

Record addresses do not index the image directly.  On this pack the record
holding address RA sits at offset (RA + 19080) * 2080; the constant was found
by scanning every record for a header whose self-address matched a directory
entry, and it is the same for all files here, so the mapping is linear.

What the data files contain
---------------------------
LEXICON / LEX2 are text: one word per line, right padded, with an "X" in the
last column as a margin marker.  The Pascal reads only the first three
characters of each line, and a line beginning "XXX" ends the current list.

On disk that text is not plain ASCII.  Every character carries the high bit,
a run of blanks is compressed to DC1 (0x91) followed by a count byte, lines
end with a single LF, and each line is padded with a NUL to an even length.
Decoding all four of those gives back the ASCII the game sees.

ROADS / ROAD2 are PRIMOS Pascal FILE OF INTEGER, and a Prime INTEGER is 32
bits, so each value is two big-endian halfwords.  ROADS holds ROUTES[1..24]
[1..10] followed by OBJECT[1..17].  ROAD2 holds ROUTES[1..59][1..10] followed
by objects, which CHIP1 reads "UNTIL K > 150" -- 21 real placements, then
nineteen zeros, then a 200 that stops the loop.
"""

import struct
import sys
import os

# The p50em disk pack is not redistributed with this repository.
# Pass its path as the first argument, or set P50EM_IMAGE.
IMAGE = (sys.argv[1] if len(sys.argv) > 1
         else os.environ.get('P50EM_IMAGE', 'disk26u0.600m'))
RECORD = 2080
HEADER_HALFWORDS = 16
RA_TO_RECORD = 19080

# Record addresses read out of the UFD, in octal as PRIMOS prints them.
FILES = {
    'LEXICON': 0o157753,
    'ROADS':   0o157747,
    'LEX2':    0o157741,
    'ROAD2':   0o157752,
}


def read_record(fh, ra):
    fh.seek((ra + RA_TO_RECORD) * RECORD)
    return fh.read(RECORD)


def read_file(fh, ra):
    """Follow the record chain and return the file's data halfwords."""
    out = []
    seen = set()
    while ra and ra not in seen:
        seen.add(ra)
        data = read_record(fh, ra)
        head = struct.unpack('>%dH' % HEADER_HALFWORDS, data[:HEADER_HALFWORDS * 2])
        self_ra, count, nxt = head[1], head[4], head[7]
        if self_ra != ra:
            raise SystemExit('record %o claims to be %o' % (ra, self_ra))
        start = HEADER_HALFWORDS * 2
        out.extend(struct.unpack('>%dH' % count, data[start:start + count * 2]))
        ra = nxt
    return out


def as_int32(halfwords):
    values = []
    for i in range(0, len(halfwords) - 1, 2):
        v = (halfwords[i] << 16) | halfwords[i + 1]
        values.append(v - 0x100000000 if v >= 0x80000000 else v)
    return values


def decode_text(raw):
    """Undo PRIMOS text storage: high bit, blank compression, NUL padding."""
    out = bytearray()
    i = 0
    while i < len(raw):
        if raw[i] == 0x00:                      # halfword alignment pad
            i += 1
            continue
        ch = raw[i] & 0x7f
        if ch == 0x11:                          # DC1: next byte is a blank count
            out.extend(b' ' * (raw[i + 1] & 0x7f))
            i += 2
            continue
        out.append(ch)
        i += 1
    return bytes(out)


def main():
    with open(IMAGE, 'rb') as fh:
        for name, ra in FILES.items():
            halfwords = read_file(fh, ra)
            raw = struct.pack('>%dH' % len(halfwords), *halfwords)
            if name in ('ROADS', 'ROAD2'):
                with open(name, 'wb') as out:
                    out.write(raw)
                note = '  (%d 32-bit integers)' % len(as_int32(halfwords))
            else:
                lines = decode_text(raw).split(b'\n')
                if lines and lines[-1] == b'':
                    lines.pop()
                with open(name.lower(), 'wb') as out:
                    out.write(b'\r\n'.join(lines) + b'\r\n')
                note = '  (%d lines of text)' % len(lines)
            print('%-8s ra=%o  %d halfwords%s' % (name, ra, len(halfwords), note))


if __name__ == '__main__':
    sys.exit(main())
