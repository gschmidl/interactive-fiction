"""Build port/data from the VMS copy of the QUEST area in ../quest/.

That copy holds every file exactly as RMS stored it on disk, so nothing has
to be repaired, only decoded (see vmsfile.py).  The game's direct-access
OPENs then read plain fixed-length records, and its sequential ones read
LF-terminated text.

  dungeon.dta    relative, RECL=4     FORMAT(I4)          10203 cells
  magic.dta      relative, RECL=54    FORMAT(A10,A36,4I2)
  moral.dta      relative, RECL=80    FORMAT(A80)
  mon.dta        sequential           FORMAT(A20,I8,I6)   text
  dunnam.dta     sequential           FORMAT(A37)         text
  access.fil     sequential           FORMAT(I1,2A5)      text
  character.dta  indexed, 252 bytes, keys name + user name

DUNGEON.DTA keeps its record numbers.  Records 520-549 were never written
on the VAX, and the author's pointer table skips over them; their cells
hold four NUL bytes, which are copied as they are.  Nothing reads them --
and if anything did, gfortran rejects NULs in an I4 field just as VMS
refused a read of a record that does not exist.

CHARACTER.DTA as copied is the first 75 of the 149 blocks the file used
(its area descriptor says so), so about half of the buckets are missing.
character.dta.orig is every live character in the buckets that survive.
An existing data/character.dta is the player's own save and is never
touched.
"""
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import vmsfile as V                                        # noqa: E402

ROOT = os.path.dirname(os.path.dirname(HERE))
SRC = os.path.join(ROOT, 'quest')
DST = os.path.join(ROOT, 'port', 'data')


def read(name):
    with open(os.path.join(SRC, name), 'rb') as f:
        return f.read()


def write(name, data):
    with open(os.path.join(DST, name), 'wb') as f:
        f.write(data)


def check_dungeon(cells):
    """The file must read the way GETDUNGEON reads it: through the pointer
    table in records 1-96, to 48 levels whose stairs are where the level
    says they are, and which between them account for every record."""
    def val(n):
        exists, d = cells[n - 1]
        assert exists, 'record %d is read but was never written' % n
        return int(d)

    starts = []
    for d in range(6):
        k = (d + 1) * 16 - 15
        starts += [val(k + 2 * l) * 10000 + val(k + 2 * l + 1) for l in range(8)]
    used = set(range(1, 97))
    for i, p in enumerate(starts):
        ll, lw = val(p), val(p + 1)
        assert 1 <= ll <= 40 and 1 <= lw <= 21, 'level %d: %dx%d' % (i, ll, lw)
        span = range(p, p + 2 + ll * lw + 4)
        assert not used & set(span), 'level %d overlaps another' % i
        used |= set(span)
        grid = [[val(p + 2 + x * lw + y) for y in range(lw)] for x in range(ll)]
        ux, uy, dx, dy = (val(span[-4 + j]) for j in range(4))
        assert grid[ux - 1][uy - 1] // 100 in (16, 18), 'level %d: stairs up' % i
        if i % 8 == 7:
            assert (dx, dy) == (0, 0), 'level 8 has a way down'
        else:
            assert grid[dx - 1][dy - 1] // 100 in (17, 19), 'level %d: stairs down' % i
    written = {n for n, (e, _) in enumerate(cells, 1) if e}
    assert used == written, 'records outside the levels: %r' % sorted(written ^ used)[:10]


def main():
    os.makedirs(DST, exist_ok=True)

    cells = V.relative_cells(read('dungeon.dta'), 4)
    check_dungeon(cells)
    write('dungeon.dta', b''.join(d for _, d in cells))
    holes = [n for n, (e, _) in enumerate(cells, 1) if not e]
    print('%-14s %5d records, 48 levels checked; never written: %s'
          % ('dungeon.dta', len(cells),
             '%d-%d' % (holes[0], holes[-1]) if holes else 'none'))

    for name, recl in (('magic.dta', 54), ('moral.dta', 80)):
        recs = V.relative_records(read(name), recl)
        write(name, b''.join(recs))
        print('%-14s %5d records of %d bytes' % (name, len(recs), recl))

    for name in ('mon.dta', 'dunnam.dta', 'access.fil'):
        text = V.var_text(read(name))
        write(name, text)
        print('%-14s %5d lines' % (name, text.count(b'\n')))

    ix = V.indexed_file(read('character.dta'))
    assert ix['keysz'] == 15 and ix['reclen'] == 252
    write('character.dta.orig', b''.join(r['data'] for r in ix['records']))
    present = sum(1 for _, _, p in ix['buckets'] if p)
    print('%-14s %5d live characters from %d of %d data buckets '
          '(%d of %d blocks survive)'
          % ('character.dta', len(ix['records']), present, len(ix['buckets']),
             len(read('character.dta')) // V.BLOCK, ix['blocks_used']))
    save = os.path.join(DST, 'character.dta')
    if not os.path.exists(save):
        shutil.copy(os.path.join(DST, 'character.dta.orig'), save)
    else:
        print('%-14s left alone (it is the game\'s save)' % '')


if __name__ == '__main__':
    main()
