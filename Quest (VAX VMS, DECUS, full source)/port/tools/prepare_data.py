"""Convert the remaining QUEST data files to the fixed-length record form
that the game's direct-access OPENs describe.

MAGIC.DTA, MORAL.DTA and MON.DTA were stored on VMS with "carriage return
carriage control" (RAT=CR), so the copy on the web has one LF appended to
each record.  Unlike DUNGEON.DTA nothing was consumed: dropping the LF
restores the record exactly.

  magic.dta  RECL=54  FORMAT(A10,A36,4I2)   relative, direct
  moral.dta  RECL=80  FORMAT(A80)           relative, direct
  mon.dta    ---      FORMAT(A20,I8,I6)     sequential, stays a text file
  dunnam.dta ---      FORMAT(A37)           sequential, stays a text file
  character.dta        252-byte records, indexed on name + username
"""
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
SRC = os.path.join(ROOT, 'src_original')
DST = os.path.join(ROOT, 'port', 'data')


def strip_terminators(name, reclen):
    data = open(os.path.join(SRC, name), 'rb').read()
    stride = reclen + 1
    if len(data) % stride:
        sys.exit('%s: %d bytes is not a multiple of %d' % (name, len(data), stride))
    n = len(data) // stride
    out = bytearray()
    for i in range(n):
        rec = data[i * stride:(i + 1) * stride]
        if rec[reclen] != 0x0A:
            sys.exit('%s: record %d does not end in LF' % (name, i + 1))
        if 0x0A in rec[:reclen] or 0x0C in rec[:reclen]:
            sys.exit('%s: record %d has an embedded control byte' % (name, i + 1))
        out += rec[:reclen]
    open(os.path.join(DST, name), 'wb').write(out)
    print('%-14s %5d records of %d bytes' % (name, n, reclen))


def main():
    os.makedirs(DST, exist_ok=True)
    strip_terminators('magic.dta', 54)
    strip_terminators('moral.dta', 80)
    for name in ('mon.dta', 'dunnam.dta', 'access.fil'):
        shutil.copy(os.path.join(SRC, name), os.path.join(DST, name))
        print('%-14s copied unchanged' % name)
    # the five characters that were live on the Ball State VAX in 1985
    shutil.copy(os.path.join(SRC, 'character.dta'),
                os.path.join(DST, 'character.dta.orig'))
    if not os.path.exists(os.path.join(DST, 'character.dta')):
        shutil.copy(os.path.join(SRC, 'character.dta'),
                    os.path.join(DST, 'character.dta'))
    print('%-14s %5d records of 252 bytes' %
          ('character.dta', os.path.getsize(os.path.join(SRC, 'character.dta')) // 252))


if __name__ == '__main__':
    main()
