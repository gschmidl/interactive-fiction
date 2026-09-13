"""Account for every byte of the web copy (src_original/) from the VMS
copy (quest/), and score the repair that was made before the VMS copy
turned up.

src_original/ is the DECUS area as downloaded from digiater.nl; quest/ is
the same area as RMS stored it.  Each web file is re-derived here from its
VMS original by the conversion it went through, and must match exactly:

  sources, text data     variable-length records joined with LF
  images, LIBRARY.OLB    identical bytes, padded with NULs to a whole block
  DEBUG.OBJ              its records concatenated, no separators
  MAGIC.DTA, MORAL.DTA   each relative record followed by LF
  DUNGEON.DTA            each record that EXISTS, with its first character
                         taken as FORTRAN carriage control and replaced by
                         the control it stands for:  ' ' and '2'-'9' -> LF,
                         '1' -> FF.  Records never written are dropped,
                         which renumbers every level after the gap.
  CHARACTER.DTA          the live records met by following the data
                         buckets' next-bucket links from the first bucket,
                         until a link points past the end of the file.

Then the repair of 2026-09-11 (made from the web copy alone) is compared
with the truth, square by square.
"""
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import vmsfile as V                                        # noqa: E402
import dungeon as D                                        # noqa: E402

ROOT = os.path.dirname(os.path.dirname(HERE))
WEB = os.path.join(ROOT, 'src_original')
VMS = os.path.join(ROOT, 'quest')


def rd(d, name):
    with open(os.path.join(d, name), 'rb') as f:
        return f.read()


def same(name, derived):
    web = rd(WEB, name)
    assert web == derived, '%s: web copy is not what the conversion gives' % name
    print('  %-14s %6d bytes  accounted for' % (name, len(web)))


def ftn(rec):
    c = rec[:1]
    if c == b'1':
        return b'\x0c' + rec[1:]
    assert c == b' ' or c.isdigit() and c != b'0'
    return b'\x0a' + rec[1:]


def chain_walk(data):
    """What a reader that trusts the next-bucket links recovers."""
    k0 = data[:V.BLOCK]
    vbn = struct.unpack_from('<I', k0, 0x54)[0]
    out = []
    while True:
        bk = V._bucket(data, vbn, 2)
        if bk is None:
            return out, vbn
        out += [r['data'] for r in V.indexed_file(data)['records'] if r['vbn'] == vbn]
        if bk['last']:
            return out, None
        vbn = bk['next']


def main():
    names = sorted(os.listdir(WEB))
    assert names == sorted(os.listdir(VMS)), 'the two copies list different files'

    print('web copy from VMS copy:')
    for name in names:
        vms = rd(VMS, name)
        if name.endswith(('.for', '.mar', '.txt', '.fil')) or name in ('mon.dta', 'dunnam.dta'):
            same(name, V.var_text(vms))
        elif name.endswith(('.exe', '.q7r', '.olb')):
            same(name, vms + b'\0' * (-len(vms) % V.BLOCK))
        elif name == 'debug.obj':
            same(name, b''.join(V.var_records(vms)))
        elif name in ('magic.dta', 'moral.dta'):
            recl = 54 if name == 'magic.dta' else 80
            same(name, b''.join(r + b'\n' for r in V.relative_records(vms, recl)))
        elif name == 'dungeon.dta':
            cells = V.relative_cells(vms, 4)
            same(name, b''.join(ftn(d) for e, d in cells if e))
        elif name == 'character.dta':
            recs, stop = chain_walk(vms)
            same(name, b''.join(recs))
            print('  %14s the bucket chain leaves the file for VBN %d after %d records'
                  % ('', stop, len(recs)))
        elif name in ('error.dat', 'fatal.dat'):
            same(name, vms)
        else:
            sys.exit('%s: no rule' % name)

    # ---- the repair, scored ------------------------------------------------
    cells = V.relative_cells(rd(VMS, 'dungeon.dta'), 4)
    truth = [int(d) for e, d in cells if e]           # web numbering
    web = D.load(os.path.join(WEB, 'dungeon.dta'))
    assert len(web) == len(truth)
    proven = wrong = low = high = 0
    highs = {}
    for start, ll, lw in D.levels(web):
        for j in range(2 + ll * lw + 4):
            n = start + j - 1
            if 2 <= j < 2 + ll * lw and D.ambiguous(web[n]):
                if truth[n] == D.decode_min(web[n]):
                    low += 1
                else:
                    assert truth[n] == D.decode_min(web[n]) + 2000
                    high += 1
                    highs[truth[n] // 100] = highs.get(truth[n] // 100, 0) + 1
            elif truth[n] == D.decode_min(web[n]):
                proven += 1
            else:
                wrong += 1
    hole = [n for n, (e, _) in enumerate(cells, 1) if not e]
    print()
    print('the repair made from the web copy alone:')
    print('  level records it decoded as proven   %5d, wrong %d' % (proven, wrong))
    print('  ambiguous squares written low        %5d right, %d wrong' % (low, high))
    print('  what the wrong ones really were      %s'
          % ', '.join('object %d x%d' % kv for kv in sorted(highs.items())))
    print('  pointer table: not stale -- records %d-%d were never written, and'
          % (hole[0], hole[-1]))
    print('  dropping them moved every later level down by %d' % len(hole))
    assert wrong == 0


if __name__ == '__main__':
    main()
