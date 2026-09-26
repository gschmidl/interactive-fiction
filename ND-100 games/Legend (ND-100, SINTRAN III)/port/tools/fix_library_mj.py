"""LIBRARY-MJ:BRF with GUSNA fixed.

usage: python tools\\fix_library_mj.py [OUT]   (default build\\LIBRARY-MJ.BRF)

The club library that survives (NILSSON-3, 1985-10-19) has GUSNA load both
MON 214 registers from its first argument:

    SWAP SA DB; STA SAVE; LDA I 0,B; LDX I 0,B; MON 214 ...

MON 214 (GetUserName) wants A -> the 16-byte name buffer and X = the user
index.  LEGEND calls CALL GUSNA(USER,WHERE(UR(0))): the index from RSIO
first, the buffer second.  With the library as it is, SINTRAN writes the name
at the address USER, LEGEND reads an empty name, and every signature on its
list of members "insl{ppta p} DNF" -- the author's own, LU, among them -- is
thrown out ("[r du insl{ppt p} DNF kan inte k|ra Legend fr}n !").  The copy
the club linked LEGEND with in 1987 must have taken the buffer from the second
argument; that one word, LDA I 1,B, is what this changes.  The unit's END
checksum is recomputed.
"""
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from brf import checksum, encode, layout, parse, units  # noqa: E402

# the club's own library, from the working tree this repository does not
# publish; set ND100_WORK to point at it
WORK = os.environ.get('ND100_WORK',
    os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', '_ND100_work'))
SRC = os.path.join(WORK, 'files', 'basic', 'LIBRARY-MJ.BRF')


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(HERE), 'build', 'LIBRARY-MJ.BRF')
    lib = bytearray(open(SRC, 'rb').read())
    recs, stop = parse(lib, 0)
    assert stop == len(lib)
    unit = [u for u in units(recs) if 'GUSNA' in layout(u)[2]][0]
    lnf = [r for r in unit if r[1] == 0x14][-1]           # the GUSNA code block
    words = lnf[2]
    assert words[1:5] == [0o144053, 0o004364, 0o045400, 0o055400], [oct(w) for w in words[1:5]]
    words[3] = 0o045401                                   # LDA I 1,B
    new_unit = [r if r is not lnf else (r[0], r[1], words) for r in unit]
    new_unit[-1] = (new_unit[-1][0], 0x11, [checksum(new_unit)])
    start, end = unit[0][0], unit[-1][0] + 3
    fixed = bytes(lib[:start]) + encode(new_unit) + bytes(lib[end:])
    assert len(fixed) == len(lib)
    r2, stop2 = parse(fixed, 0)
    assert stop2 == len(fixed) and all(checksum(u) == u[-1][2][0] for u in units(r2))
    os.makedirs(os.path.dirname(out), exist_ok=True)
    open(out, 'wb').write(fixed)
    diff = [i for i in range(len(lib)) if lib[i] != fixed[i]]
    print('%s: %d bytes, bytes changed at %s' % (out, len(fixed), diff))


if __name__ == '__main__':
    main()
