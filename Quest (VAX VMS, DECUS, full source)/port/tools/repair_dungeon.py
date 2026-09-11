"""Rebuild DUNGEON.DTA in the form QUEST's FORTRAN actually reads.

Input : src_original/dungeon.dta  -- the DECUS/web copy, in which the first
        character of every 4-character record has been replaced by the ASCII
        control it stands for as a FORTRAN carriage-control byte.
Output: port/data/dungeon.dta     -- 10173 records of exactly 4 characters,
        no separators, as produced by  WRITE(24'K,'(I4)') VALUE  on VMS.

What is repaired, and on what evidence:

 1. Leading character.   LF -> ' ' , FF -> '1'.  A surviving second character
    of '0' proves the leading character was a digit (I4 right justifies, so
    " 0nw" can never be written); with map codes bounded by 2788 that digit
    can only be '2'.  144 map records and 5 pointer records are recovered
    this way.

 2. Pointer table (records 1..96).   The table shipped in the file is stale:
    it is internally inconsistent (dungeon 1 level 4 would start at 550 with
    dimensions 20x20, overlapping its own level 5 at 766), and several of its
    records lost a leading digit.  It is regenerated from the level chain,
    which is self-describing and provably correct:
      * walking start -> start + 2 + LL*LW + 4 from record 97 yields exactly
        48 levels and ends exactly on end-of-file;
      * every LL is in 1..40 and every LW in 1..21 (the bounds of MAP(40,21));
      * the square named by STAIRSUPX/Y carries object 18 or 16, and the
        square named by STAIRSDOWNX/Y carries object 19 or 17, in all 48
        levels -- 90 independent cross-checks, no mismatches;
      * exactly six levels have down-stairs (0,0), and they are chain
        positions 8, 16, 24, 32, 40 and 48 -- i.e. level 8 of each dungeon,
        which fixes the dungeon/level ordering.

 3. Ambiguous records.   A leading character of '1'..'7' under a LF prefix is
    either a blank (object 1..7) or a lost '2' (object 21..27).  658 map
    records are affected and the surviving bytes cannot distinguish them.
    They are written as the low object, and listed in dungeon_ambiguous.txt.
    See README.md for why the low reading is the better estimate, and for
    what is known to be lost (the four dragons, object 27).
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import dungeon as D                                        # noqa: E402

ROOT = os.path.dirname(os.path.dirname(HERE))
SRC = os.path.join(ROOT, 'src_original', 'dungeon.dta')
OUT = os.path.join(ROOT, 'port', 'data', 'dungeon.dta')
REPORT = os.path.join(ROOT, 'port', 'data', 'dungeon_ambiguous.txt')


def main():
    recs = D.load(SRC)
    lv = D.levels(recs)
    assert len(lv) == 48, len(lv)
    assert lv[-1][0] + 2 + lv[-1][1] * lv[-1][2] + 4 == len(recs) + 1

    out = [None] * len(recs)

    # 2. pointer table, regenerated from the chain
    for d in range(6):
        k = (d + 1) * 16 - 15
        for l in range(8):
            p = lv[d * 8 + l][0]
            out[k - 1 + 2 * l] = '%4d' % (p // 10000)
            out[k + 2 * l] = '%4d' % (p % 10000)

    # 1./3. level data, decoded record by record
    amb = []
    for i, (start, LL, LW) in enumerate(lv):
        for j in range(2 + LL * LW + 4):
            n = start + j
            r = recs[n - 1]
            if 2 <= j < 2 + LL * LW and D.ambiguous(r):
                y, x = (j - 2) % LW + 1, (j - 2) // LW + 1
                amb.append((n, i // 8 + 1, i % 8 + 1, x, y,
                            D.decode_min(r), D.decode_min(r) + 2000))
            out[n - 1] = '%4d' % D.decode_min(r)

    assert all(v is not None and len(v) == 4 for v in out)
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, 'wb') as f:
        f.write(''.join(out).encode('ascii'))

    with open(REPORT, 'w') as f:
        f.write('Records whose lost leading character cannot be proven.\n'
                'Written as "low"; "high" is the alternative (lost 2).\n\n'
                '  rec  dun lvl    x   y    low   high\n')
        for n, d, l, x, y, lo, hi in amb:
            f.write('%5d  %3d %3d  %3d %3d  %5d  %5d\n' % (n, d, l, x, y, lo, hi))

    print('wrote %s  (%d records, %d bytes)' % (OUT, len(out), len(out) * 4))
    print('pointer table regenerated for 6 dungeons x 8 levels')
    print('ambiguous map records: %d  -> %s' % (len(amb), REPORT))


if __name__ == '__main__':
    main()
