"""Put a small text file on PRIMOS with ED, then read it back and check it.

Step 8's lesson was that ED mangles long uploads; this one verifies what
landed and says so, so it is only used for the few short probe programs.
"""
import sys
import time

from primesh import Prime


def put(p, name, text):
    lines = [l.rstrip() for l in text.rstrip('\n').split('\n')]
    # ED with no file argument comes up in INPUT mode already.
    p.cmd('ED', prompt=r'EDIT|INPUT|\nER! ', timeout=30)
    time.sleep(0.5)
    p.read()
    for l in lines:
        p.line(l, delay=0.02)
        time.sleep(0.05)
    p.line('')                      # leave input mode
    time.sleep(0.5)
    p.read()
    p.line('FILE ' + name)
    return p.expect(r'\nOK, |\nER! ', 60)


def check(p, name, text):
    out = p.cmd('SLIST ' + name, timeout=60)
    want = [l.rstrip() for l in text.rstrip('\n').split('\n')]
    got = [l.rstrip() for l in out.split('\n')]
    # SLIST echoes the command and ends with OK,
    got = [l for l in got if l and not l.startswith('SLIST ')
           and not l.startswith('OK,')]
    if got != want:
        print('MISMATCH: %d lines back, %d sent' % (len(got), len(want)))
        for a, b in zip(got + [''] * 99, want + [''] * 99):
            if a != b:
                print('  got  %r\n  want %r' % (a, b))
                break
        return False
    print('%s: %d lines, verified' % (name, len(want)))
    return True


if __name__ == '__main__':
    name, path = sys.argv[1], sys.argv[2]
    text = open(path, encoding='latin-1').read()
    p = Prime(log='edput.log')
    try:
        p.login()
        p.cmd('A *>ADVENTURE.UFD', timeout=30)
        put(p, name, text)
        ok = check(p, name, text)
    finally:
        p.close()
    sys.exit(0 if ok else 1)
