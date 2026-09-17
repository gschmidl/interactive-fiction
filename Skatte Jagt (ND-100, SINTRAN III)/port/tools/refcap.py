"""Capture a reference transcript of SKAT on the RetroCore SINTRAN system.

usage: refcap.py SCRIPT OUT [--settle SECONDS] [--prog SKAT]
SCRIPT: one game command per line (lines starting with '#' ignored).
Writes OUT (raw bytes as received, telnet stripped) and OUT.txt (parity
stripped).  After each command waits until the terminal has been quiet for
--settle seconds.  Quits the game and logs out at the end if still running.
"""
import sys, os, time, argparse
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ndtel import Term

def quiet(t, settle, limit=120):
    end = time.time() + limit
    last = len(t.raw)
    stamp = time.time()
    while time.time() < end:
        t._pump(0.1)
        if len(t.raw) != last:
            last = len(t.raw); stamp = time.time()
        elif time.time() - stamp >= settle:
            return
    raise TimeoutError('no quiet period')

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('script'); ap.add_argument('out')
    ap.add_argument('--settle', type=float, default=1.5)
    ap.add_argument('--prog', default='SKAT')
    a = ap.parse_args()
    cmds = [l.rstrip('\r\n') for l in open(a.script, encoding='latin-1')]
    cmds = [c for c in cmds if not c.startswith('#')]
    t = Term()
    t.raw = bytearray()
    orig = t._pump
    def pump(dt, _o=orig):
        n = len(t.buf); _o(dt); t.raw.extend(t.buf[n:])
    t._pump = pump
    t.send(b'\x1b', cr=False); t.expect('ENTER', 20)
    t.send('GAMES'); t.expect('PASSWORD', 20); t.send(''); t.expect(r'@', 20)
    quiet(t, 0.5)
    start = len(t.raw)
    t.send(a.prog)
    quiet(t, a.settle)
    for c in cmds:
        t.send(c)
        quiet(t, a.settle)
    body = bytes(t.raw[start:])
    tail = bytes(b & 0x7f for b in t.raw[-40:])
    if not tail.rstrip().endswith(b'@'):
        t.send('SLUTT'); quiet(t, a.settle); t.send('J'); quiet(t, a.settle)
    t.send('LOG OUT'); quiet(t, 0.5)
    open(a.out, 'wb').write(body)
    open(a.out + '.txt', 'wb').write(bytes(b & 0x7f for b in body))

if __name__ == '__main__':
    main()
