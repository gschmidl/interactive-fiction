#!/usr/bin/env python3
"""Repair the world file that came off the tape, and write it to data/.

NEWADV.DAT is the world: 13,529 PDP-10 single-precision floats laid end to
end, built by FILER and then written back by the game as people play.  The
copy in mpl/ has the same one-word wound as the .SHR images -- the tape
extraction dropped a word and padded the end to keep the byte count -- and
DUNGEN will not start with it, because everything past the wound is shifted
by one and the last record runs off the end of the file: TYMBASIC run-phase
error 142, "End of file found".

This is the one file where the damage can be proved rather than argued.
FILER is deterministic and takes no input, so running the port's own FILER
produces a reference copy of a fresh world.  Compare the two:

  * words 0..3673 are identical;
  * from 3674 on, tape[k] == fresh[k+1] for all but five words;
  * the tape file's last word is not in the fresh one at all.

which is exactly one word missing at 3674 and one word of padding at the
end.  The five that still differ are the world as people left it -- five
values that read 15 on the tape where a new world has 3.

So the missing word is taken from a fresh world file built by this game's
own FILER, nothing else is touched, and the padding word is dropped.  The
result is the world as it stood in the mpl directory in December 1984.
"""
import os, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
DUMP = os.path.join(HERE, '..', '..', 'dump_original')
OUT  = os.path.join(HERE, '..', 'data', 'newadv-1984.dat')
EXE  = os.path.join(HERE, '..', 'bin', 'zarast.exe')


def words(data):
    return [((data[i]   & 0x7f) << 29) | ((data[i+1] & 0x7f) << 22)
          | ((data[i+2] & 0x7f) << 15) | ((data[i+3] & 0x7f) <<  8)
          | ((data[i+4] & 0x7f) <<  1) | ((data[i+4] >> 7) & 1)
            for i in range(0, len(data) - 4, 5)]


def unwords(ws):
    out = bytearray()
    for w in ws:
        for k in range(5):
            b = (w >> (29 - 7 * k)) & 0x7f
            if k == 4: b |= (w & 1) << 7
            out.append(b)
    # Every word goes out, zeros included: the file's length is
    # its record count, and the game reads it back that way.
    return bytes(out)


def main():
    tape = words(open(os.path.join(DUMP, 'newadv.dat'), 'rb').read())
    tmp = tempfile.mkdtemp(prefix='zarast-')
    subprocess.run([EXE, '-d', tmp, '-q', 'filer'],
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                   check=True)
    fresh = words(open(os.path.join(tmp, 'newadv.dat'), 'rb').read())

    g = 0
    while g < len(tape) and tape[g] == fresh[g]:
        g += 1
    off = sum(1 for k in range(g, len(tape) - 1) if tape[k] != fresh[k + 1])
    print('identical prefix        : %d words' % g)
    print('tape[k] == fresh[k+1]   : %d of %d words after that'
          % (len(tape) - 1 - g - off, len(tape) - 1 - g))
    if off > 20:
        sys.exit('the two files do not line up; not writing anything')

    fixed = tape[:g] + [fresh[g]] + tape[g:len(tape) - 1]
    assert len(fixed) == len(tape)
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    open(OUT, 'wb').write(unwords(fixed))
    print('restored word %012o at %d, dropped the padding word' % (fresh[g], g))
    print('wrote %s (%d bytes)' % (OUT, os.path.getsize(OUT)))


main()
