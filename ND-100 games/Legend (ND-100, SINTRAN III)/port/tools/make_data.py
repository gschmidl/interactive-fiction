"""Build data\\ from src_original\\data: LEGEND's files as the author's
20-IN-LEGEND mode file installed them for play.

usage: python tools\\make_data.py [OUT_DIR]

  copied as they are     RUMFIL-0..4 and 8 (rooms), SAKKARE-1..9 (things),
                         INSTR-LEGEND, HELPFIL, BROAD-LEGEND, BORT, VEMFIL
  in v10.0's layout      SPELARE-1..9 (see convert_players.py)
  created empty          RUMFIL-5..7 (empty on the disk too), LEGEND-MSG-LU and
                         ACC-LEGEND-LU (@CR-FI in the mode file), the
                         semaphores SCENARIO-1..9-LU:SMPH (@SET-TEMP-FI)
  made up                (SCRATCH)SIGNATURES-EAO:DATA, the club's list of
                         signatures and names, which is not on the disks:
                         only its end mark, so every player is asked a name
"""
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from convert_players import convert, parity  # noqa: E402

PORT = os.path.dirname(HERE)
SRC = os.path.join(PORT, '..', 'src_original', 'data')


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(PORT, 'data')
    os.makedirs(out, exist_ok=True)
    copied = ['RUMFIL-%d-LU.DATA' % n for n in (0, 1, 2, 3, 4, 8)]
    copied += ['SAKKARE-%d-LU.DATA' % n for n in range(1, 10)]
    copied += ['INSTR-LEGEND-LU.DATA', 'HELPFIL-LU.DATA', 'BROAD-LEGEND-LU.DATA', 'BORT-LU.DATA',
               'VEMFIL-LU.DATA']
    for f in copied:
        shutil.copyfile(os.path.join(SRC, f), os.path.join(out, f))
    for n in range(1, 10):
        data, players = convert(os.path.join(SRC, 'SPELARE-%d-LU.DATA' % n))
        open(os.path.join(out, 'SPELARE-%d-LU.DATA' % n), 'wb').write(data)
    empty = ['RUMFIL-%d-LU.DATA' % n for n in (5, 6, 7)]
    empty += ['LEGEND-MSG-LU.DATA', 'ACC-LEGEND-LU.DATA']
    empty += ['SCENARIO-%d-LU.SMPH' % n for n in range(1, 10)]
    for f in empty:
        open(os.path.join(out, f), 'wb').close()
    open(os.path.join(out, 'SIGNATURES-EAO.DATA'), 'wb').write(parity('.,.\r\n') + b'\x17')
    print('%d files in %s' % (len(copied) + 9 + len(empty) + 1, out))


if __name__ == '__main__':
    main()
