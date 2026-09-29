"""Capture the editing-key sessions on a RetroCore SINTRAN III.

usage: python tools/refkeys.py [lower del bs ctla ctlq long empty 8bit]

Logs in as GAMES on the terminal port (127.0.0.1:9000), starts SKAT, waits
for the first question, sends the keys, quits the game, logs out, and saves
the raw bytes as tests/captures/k_NAME.raw.  Run it against a copy of the
pack, never the eXo original.
"""
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ndtel import Term

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
TESTS = {
    'lower': [b'nei\r', b'inn\r'],
    'del': [b'NEI\r', b'INNX\x7f\r'],
    'bs': [b'NEI\r', b'INNX\x08\r'],
    'ctla': [b'NEI\r', b'INNX\x01\r'],
    'ctlq': [b'NEI\r', b'XYZ\x11INN\r'],
    'long': [b'NEI\r', b'GAAAAAAAAAAA NORDDDDDDDDD\r'],
    'empty': [b'NEI\r', b'\r', b'   \r'],
    '8bit': [b'NEI\r', b'\xe5PENT\r'],
}


def run(name, keys, settle=2.0):
    path = os.path.join(ROOT, 'tests', 'captures', 'k_%s.raw' % name)
    t = Term(log=path)
    t.send(b'\x1b', cr=False)
    t.expect('ENTER', 10)
    t.send('GAMES')
    t.expect('PASSWORD', 10)
    t.send('')
    t.expect('@', 10)
    t.drain(0.5)
    t.send('SKAT')
    t.expect(r'reglene\?', 20)
    t.drain(1.0)
    for k in keys:
        t.s.sendall(k)
        time.sleep(settle)
        t._pump(0.2)
    t.send('SLUTT')
    time.sleep(settle)
    t.send('J')
    time.sleep(settle)
    t.send('LOG OUT')
    time.sleep(1)
    t._pump(0.5)
    t.log.close()


if __name__ == '__main__':
    for n in sys.argv[1:] or list(TESTS):
        run(n, TESTS[n])
        print('captured', n)
