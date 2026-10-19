"""Capture SVHA Adventure editing-key sessions on the RetroCore reference.

usage: svhakeys.py [NAME ...]    -> ref/svha/k_NAME.raw
"""
import os, sys, time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ndtel import Term

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
TESTS = {
    'lower': [b'no\r', b'in\r'],
    'del': [b'NO\r', b'INX\x7f\r'],
    'bs': [b'NO\r', b'INX\x08\r'],
    'ctla': [b'NO\r', b'INX\x01\r'],
    'ctlq': [b'NO\r', b'XYZ\x11IN\r'],
    'ctlk': [b'NO\r', b'XYZ\x0bIN\r'],
    'delstart': [b'NO\r', b'\x7f\x01IN\r'],
    'long': [b'NO\r', b'GOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO NORTH\r'],
    'empty': [b'NO\r', b'\r', b'   \r'],
    'ctrl': [b'NO\r', b'I\x02N\x07\r'],
}


def run(name, keys, settle=2.0):
    t = Term(log=os.path.join(ROOT, 'ref', 'svha', 'k_%s.raw' % name))
    t.send(b'\x1b', cr=False)
    t.expect('ENTER', 20)
    t.send('GAMES')
    t.expect('PASSWORD', 10)
    t.send('')
    t.expect('@', 10)
    t.drain(0.5)
    t.send('SVH')
    t.expect(r'instructions\?', 180)
    t.drain(1.0)
    for k in keys:
        # one key at a time: a burst longer than SINTRAN's type-ahead buffer
        # loses characters on the real system, which is not what we measure
        for ch in k:
            t.s.sendall(bytes([ch]))
            time.sleep(0.03)
            t._pump(0.01)
        time.sleep(settle)
        t._pump(0.2)
    t.send('QUIT')
    time.sleep(settle)
    t.send('YES')
    time.sleep(settle)
    t.drain(2)
    t.send('LOG OUT')
    time.sleep(1)
    t._pump(0.5)
    t.log.close()


if __name__ == '__main__':
    for n in sys.argv[1:] or list(TESTS):
        run(n, TESTS[n])
        print('captured', n)
