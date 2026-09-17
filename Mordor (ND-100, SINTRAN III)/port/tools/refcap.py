"""Capture a reference session of MORDOR on SINTRAN III under RetroCore.

usage: refcap.py SCRIPT OUT [--prog MORDORF-REF] [--keep-map] [--settle 1.5]

The machine: a copy of the eXo RetroCore set-up whose pack holds user DNF with
MORDORF-REF:PROG (the fixed program with the reference patches described
in NOTES.md) and MORDOR-RULES-MJ:DATA.  RetroCore listens on TCP 9000.

SCRIPT holds one unit of typing per line, sent as it stands and followed by a
wait for the terminal to go quiet.  Escapes: \\e = Esc, \\r = Return, \\d = DEL,
\\a = Ctrl-A (the ND delete key), \\q = Ctrl-Q (delete the line),
\\\\ = backslash.  Lines starting with # are comments.  (Nothing is added:
write \\r where Return is pressed.)  A line  KEYS || REGEX  waits instead
until REGEX matches the parity-stripped output that follows the keys (and
then for quiet); a line that is only  || REGEX  waits for the program's
first output.  Use this wherever the game computes for a while: typing ahead
of SINTRAN is not like typing ahead into a pipe (Esc breaks at once).

Unless --keep-map, the map file is deleted and created empty first, so the
game builds a new map.  DEL is typed ahead of the program so that the day-time
lock (08-15 on the clock) lets the session start, as the club's players did.

OUT gets the raw bytes from the program's first output to the end; OUT.txt
the same with parity stripped.  With --keys, the typed bytes go to that file
so the port can be fed exactly the same input.
"""
import argparse
import os
import re
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ndtel import Term  # noqa: E402


def decode(line):
    out = bytearray()
    i = 0
    while i < len(line):
        ch = line[i]
        if ch == '\\' and i + 1 < len(line):
            n = line[i + 1]
            out += {'e': b'\x1b', 'r': b'\r', 'd': b'\x7f', 'a': b'\x01', 'q': b'\x11',
                    '\\': b'\\'}.get(n, ('\\' + n).encode())
            i += 2
            continue
        out.append(ord(ch))
        i += 1
    return bytes(out)


def quiet(t, settle, limit=900):
    end = time.time() + limit
    last = len(t.raw)
    stamp = time.time()
    while time.time() < end:
        t._pump(0.05)
        if len(t.raw) != last:
            last = len(t.raw)
            stamp = time.time()
        elif time.time() - stamp >= settle:
            return
    raise TimeoutError('no quiet period')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('script')
    ap.add_argument('out')
    ap.add_argument('--prog', default='MORDORF-REF')
    ap.add_argument('--keep-map', action='store_true')
    ap.add_argument('--settle', type=float, default=1.5)
    ap.add_argument('--keys')
    ap.add_argument('--timeout', type=float, default=900)
    ap.add_argument('-v', '--verbose', action='store_true')
    a = ap.parse_args()
    units = []
    for l in open(a.script, encoding='latin-1'):
        l = l.rstrip('\r\n')
        if l.startswith('#') or not l:
            continue
        keys, sep, rx = l.partition('||')
        units.append((decode(keys.rstrip() if sep else keys), rx.strip() if sep else None))
    t = Term()
    t.raw = bytearray()
    orig = t._pump

    def pump(dt, _o=orig):
        n = len(t.buf)
        _o(dt)
        t.raw.extend(t.buf[n:])
    t._pump = pump
    t.send(b'\x1b', cr=False)
    t.expect('ENTER', 60)
    t.send('DNF')
    t.expect('PASSWORD', 20)
    t.send('')
    t.expect(r'@', 30)
    quiet(t, 0.5)
    if not a.keep_map:
        t.send('DELETE-FILE MORDOR-MAP-MJ:DATA')
        quiet(t, 1.0)
        t.send('CREATE-FILE MORDOR-MAP-MJ:DATA,0')
        quiet(t, 1.0)
    t.buf.clear()
    start = len(t.raw)
    t.send(a.prog + '\r' + '\x7f', cr=False)
    typed = bytearray()
    first = True
    for u, rx in units:
        mark = len(t.raw)
        if u:
            t.send(u, cr=False)
            typed += u
        elif not first:
            mark = len(t.raw)
        if rx:
            pat = re.compile(rx.encode('latin-1'), re.S)
            end = time.time() + a.timeout
            while not pat.search(bytes(b & 0x7f for b in t.raw[mark:])):
                if time.time() > end:
                    open(a.out + '.partial', 'wb').write(bytes(t.raw[start:]))
                    raise TimeoutError('waiting for %r after %r' % (rx, u))
                t._pump(0.05)
            if a.verbose:
                sys.stderr.write('[%r ok]\n' % rx)
            quiet(t, 0.3)
        else:
            quiet(t, a.settle)
        first = False
    body = bytes(t.raw[start:])
    t.send(b'\x1b', cr=False)
    quiet(t, 0.5)
    t.send('LOGOUT')
    quiet(t, 0.5)
    open(a.out, 'wb').write(body)
    open(a.out + '.txt', 'wb').write(bytes(b & 0x7f for b in body))
    if a.keys:
        open(a.keys, 'wb').write(bytes(typed))


if __name__ == '__main__':
    main()
