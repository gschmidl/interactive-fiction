#!/usr/bin/env python3
"""Does the port set the game up as the site's NEUSPIEL has it?

    python tests\\crosscheck.py

NEUSPIEL is a game saved at its first command, SICHR MEIN, on Friday 13
June 1980 at 14:34.  This does the same in the port, in a scratch copy:
--fresh (the game sets itself up from ADV.DATA), NEIN to the wizard, NEIN
to the instructions, SICHR MEIN, JA - with the clock held at that minute,
and -u so that the Friday afternoon is not prime time - and compares
saves\\MEIN.SAV with NEUSPIEL.DAT variable by variable: every word of
text, the vocabulary, the travel table, the objects, and the state of the
game after one command.

The section files on the tape are a later edition than the one NEUSPIEL
was set up from: five edits were made since, all in text (the list is in
src\\edition1980.py, which writes ADV1980.DATA with them undone).  So the
port is run twice: --fresh, on the files as the tape has them (ADV.DATA),
where the differences are reported, and --fresh=1980, on ADV1980.DATA as
build.sh made it, where there must be none.
"""
import os
import shutil
import subprocess
import struct
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import state  # noqa: E402


def names():
    """(byte offset, size, name) of every element saved, in file order"""
    return [(o, s, '%s(%d)' % (n, i) if i else n)
            for o, s, n, i in state.names()]


def value(data, off, size):
    return struct.unpack('<q' if size == 8 else '<i', data[off:off + size])[0]


def turn_one(data, option):
    """the port's own NEUSPIEL: its save after SICHR MEIN at turn 1, set
    up by OPTION from the data file DATA"""
    tmp = tempfile.mkdtemp(prefix='abenteuer-')
    try:
        shutil.copy(os.path.join(PORT, 'abenteuer.exe'), tmp)
        shutil.copy(os.path.join(PORT, data), tmp)
        r = subprocess.run([os.path.join(tmp, 'abenteuer.exe'), option, '-u',
                            '--date', '1980-06-13', '--time', '14:34'],
                           input=b'NEIN\nNEIN\nSICHR MEIN\nJA\n',
                           capture_output=True, timeout=60)
        save = os.path.join(tmp, 'saves', 'MEIN.SAV')
        if not os.path.exists(save):
            print(r.stdout.decode('latin-1'))
            sys.exit('crosscheck: the port did not save MEIN')
        return open(save, 'rb').read()
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def compare(mine, site, table):
    if len(mine) != len(site):
        sys.exit('crosscheck: %d bytes, NEUSPIEL.DAT %d' % (len(mine), len(site)))
    return [(n, value(mine, o, s), value(site, o, s)) for o, s, n in table
            if mine[o:o + s] != site[o:o + s]]


def main():
    site = open(os.path.join(PORT, 'NEUSPIEL.DAT'), 'rb').read()
    table = names()
    tape = compare(turn_one('ADV.DATA', '--fresh'), site, table)
    print('crosscheck: the tape\'s edition: %d of %d variables differ '
          '(text, and the vocabulary one entry on)' % (len(tape), len(table)))
    old = compare(turn_one('ADV1980.DATA', '--fresh=1980'), site, table)
    for n, a, b in old:
        print('  %-12s port %-14d NEUSPIEL %d' % (n, a, b))
    print('crosscheck: the 1980 edition: %d of %d variables differ'
          % (len(old), len(table)))
    return old


if __name__ == '__main__':
    sys.exit(1 if main() else 0)
