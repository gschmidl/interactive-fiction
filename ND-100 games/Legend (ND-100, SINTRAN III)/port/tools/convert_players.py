"""Bring the players of LEGEND v9.11 into the layout v10.0 reads.

usage: python tools\\convert_players.py [SRC_DIR] [OUT_DIR]

SRC_DIR (default ..\\src_original\\data) holds SPELARE-n-LU.DATA as the
author's backup set left them, written by v9.11; OUT_DIR (default data\\)
gets SPELARE-1..9-LU.DATA as the port's v10.0 writes them (line 14964).
v10.0 only offers scenarios 1-9, so 10-12 are left out.

The differences, from the two sources' save and load routines:

  v9.11                         v10.0
  SORT$ (SLAGSK[MPE, ...)       SORT, its number in SORT$() (1-12)
  GOD$ (ORDEN, KLANEN, LAGL\\S)  GOD: 1, 2, 3 (0 a god)
  SOMN 0 for the dead           0 (v10.0 as recovered wrote a NUL line, which
                                its INPUT# cannot read back: see basic\fixes.diff)
  -                             TPOS(1), TPOS(2), TPOS(3): 0

Everything else is copied line for line, text exactly as it was (even
parity, CR LF).  One record needs mending first: v9.11 wrote "Namnl|s" and
then the empty HEISSE$ as well for a player who gave no name, a line too
many (SPELARE-4, the fourth player); v10.0 writes only "Namnl|s".
The author added fields to these files the same way (NY-VARIABEL-LU).
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)

SORTS = ['SLAGSK[MPE', 'L[RLING', 'MAGIKER', 'PR[ST', 'KRIGARE', 'TROLLKARL',
         'MYSTIKER', 'MUNK', 'RIDDARE', 'H[XM[STARE', '[RKEMAGIKER', 'KONUNG']
GODS = {'GUD': 0, 'ORDEN': 1, 'KLANEN': 2, 'LAGL\\S': 3}
FIELDS = ('MEDL SORT GOD PLAY NAM MU HP MPOS NIVA STY TOT CARRY DODAF DODAL TROLLK ALDER '
          'STEG IQ GULD EP MPKT VISD SKICK CHARM RUST SOMN KILLED PLOCKAT BANKADE TEMPEL '
          'FACK HUNGRIG SVART').split()


def parity(text):
    out = bytearray()
    for ch in text:
        b = ord(ch)
        if bin(b).count('1') % 2:
            b |= 0x80
        out.append(b)
    return bytes(out)


def lines_of(path):
    b = bytes(x & 0x7f for x in open(path, 'rb').read())
    end = b.find(b'\x17')
    if end >= 0:
        b = b[:end]
    return b.decode('latin-1').split('\r\n')


def number(text):
    return ' %d ' % int(text) if int(text) >= 0 else '%d ' % int(text)


def convert(path):
    lines = lines_of(path)
    count = int(lines[0])
    at, players = 1, []
    for p in range(count):
        rec = dict(zip(FIELDS, lines[at:at + len(FIELDS)]))
        if rec['NAM'] == 'Namnl|s' and rec['MU'] == '' and lines[at + 6] == '\0':
            del lines[at + 5]                          # the extra empty HEISSE$
            rec = dict(zip(FIELDS, lines[at:at + len(FIELDS)]))
        at += len(FIELDS)
        players.append(rec)
    out = [number(str(count))]
    for rec in players:
        for f in FIELDS:
            v = rec[f]
            if f == 'SORT':
                v = number(str(SORTS.index(v) + 1))
            elif f == 'GOD':
                v = number(str(GODS[v]))
            elif f not in ('MEDL', 'PLAY', 'NAM', 'MU'):
                v = number(v)
            out.append(v)
        out += [' 0 ', ' 0 ', ' 0 ']
    return parity('\r\n'.join(out) + '\r\n') + b'\x17', players


def main():
    src = sys.argv[1] if len(sys.argv) > 1 else os.path.join(PORT, '..', 'src_original', 'data')
    dst = sys.argv[2] if len(sys.argv) > 2 else os.path.join(PORT, 'data')
    os.makedirs(dst, exist_ok=True)
    for n in range(1, 10):
        name = 'SPELARE-%d-LU.DATA' % n
        data, players = convert(os.path.join(src, name))
        open(os.path.join(dst, name), 'wb').write(data)
        print('%s: %d players' % (name, len(players)))


if __name__ == '__main__':
    main()
