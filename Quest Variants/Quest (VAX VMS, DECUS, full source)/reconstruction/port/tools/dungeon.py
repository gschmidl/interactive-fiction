"""Decode the DECUS copy of DUNGEON.DTA.

The web/DECUS copy lost the first character of every 4-character record:
the file carries RMS "FORTRAN carriage control" (RAT=FTN), so the tool that
converted it to a stream file consumed byte 1 as a carriage-control character
and emitted the control it stands for.  Mapping:

    ' '      -> LF (0x0A)   value recoverable (3 digits, 0..999)
    '1'      -> FF (0x0C)   value recoverable (1000..1999)
    '2'..'9' -> LF (0x0A)   leading digit LOST

Every record was written by QUEST/DNDOP as FORMAT(I4), so it is right
justified: a surviving first digit of '0' therefore proves a digit was lost.
"""

LF, FF = 0x0A, 0x0C


def load(path):
    d = open(path, 'rb').read()
    assert len(d) % 4 == 0
    return [d[4 * i:4 * i + 4] for i in range(len(d) // 4)]


def value(rec, hi=' '):
    """Decode one raw record; `hi` is the assumed lost leading character."""
    c = ' ' if rec[0] == LF else '1'
    if rec[0] == LF and rec[1:2].isdigit():
        c = hi
    s = (c + rec[1:].decode('latin1')).strip()
    return int(s) if s else 0


def decode_min(rec):
    """Decode using only provable information (no guessing)."""
    c = ' ' if rec[0] == LF else '1'
    a = rec[1:2].decode('latin1')
    if rec[0] == LF and a == '0':
        c = '2'                      # ' 0xx' is impossible from I4 -> digit lost
    s = (c + rec[1:].decode('latin1')).strip()
    return int(s) if s else 0


def ambiguous(rec):
    """True when the lost leading character cannot be proven."""
    return rec[0] == LF and rec[1:2].decode('latin1') in '1234567'


def levels(recs):
    """Walk the self-describing level chain.  Returns [(start, LL, LW)]."""
    out, k = [], 97
    while k + 1 <= len(recs):
        LL, LW = decode_min(recs[k - 1]), decode_min(recs[k])
        out.append((k, LL, LW))
        k += 2 + LL * LW + 4
    return out


def level_map(recs, start, LL, LW):
    base = start + 2
    m = [[decode_min(recs[base - 1 + (x * LW) + y]) for y in range(LW)]
         for x in range(LL)]
    s = base + LL * LW
    stairs = tuple(decode_min(recs[s - 1 + i]) for i in range(4))
    return m, stairs


NORTH = ['MMMMMMM', 'MM---MM', 'MM   MM', 'MM+++MM',
         'MM+I+MM', 'MM+B+MM', 'MM+S+MM', 'MM+D+MM']
WEST = ['M', '!', ' ', '+', 'I', 'B', 'S', 'D']
FEAT = {1: 'FNT', 2: 'TEL', 3: 'TEL', 4: 'TEL', 5: 'THR', 6: '[_]', 7: 'PIT',
        8: '(S)', 9: '(P)', 10: '(D)', 11: '(F)', 12: '/N/', 13: '/E/',
        14: '/S/', 15: '/W/', 16: 'DSU', 17: 'DSD', 18: ' SU', 19: ' SD',
        20: '80%', 21: '50%', 22: ' NM', 23: ' NT', 25: '3XM', 26: '-MG',
        27: 'DRA'}
TAG = {2: 'L', 3: 'D', 4: 'A'}


def render(m, LL, LW):
    """Reimplementation of DNDOP's BUILDMAP drawing."""
    out = []
    for x in range(LL):
        row = [[' '] * (LW * 6 + 2) for _ in range(4)]
        for y in range(LW):
            code = m[x][y]
            w, nw, obj = code % 10, (code // 10) % 10, code // 100
            n7 = y * 6
            if x != 0 and m[x - 1][y] % 10 > 0:
                row[0][n7] = 'M'
            if nw:
                for i, ch in enumerate(NORTH[nw]):
                    row[0][n7 + i] = ch
            if w:
                row[0][n7] = 'M'
                row[1][n7] = 'M'
                row[2][n7] = WEST[w]
                row[3][n7] = 'M'
            if obj in FEAT:
                for i, ch in enumerate(FEAT[obj]):
                    row[2][n7 + 2 + i] = ch
                if obj in TAG:
                    row[3][n7 + 3] = TAG[obj]
        i1 = LW * 6
        for r in row:
            r[i1] = r[0]
            out.append(''.join(r).rstrip())
    return out
