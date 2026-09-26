"""Capture an SVHA Adventure session on the RetroCore reference, with its clock.

usage: svhacap.py SCRIPT OUT [--settle SECONDS]

SVHA seeds its random numbers from the day and the minute it starts, so the
session is only repeatable if the emulator sees the same minute.  The login
banner carries SINTRAN's own time; the program is started early in a minute
and OUT.json records the SINTRAN time it was started at, plus the -Z value
(the host epoch 28 years later) that makes svha.exe see the same minute.

Commands are typed one key at a time and the next one is sent once the
terminal has been quiet for --settle seconds.
"""
import argparse, json, os, re, sys, time
from datetime import datetime, timedelta
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ndtel import Term

MONTHS = ['JANUARY', 'FEBRUARY', 'MARCH', 'APRIL', 'MAY', 'JUNE', 'JULY', 'AUGUST',
          'SEPTEMBER', 'OCTOBER', 'NOVEMBER', 'DECEMBER']


def quiet(t, settle, limit=180):
    end = time.time() + limit
    last, stamp = len(t.raw), time.time()
    while time.time() < end:
        t._pump(0.1)
        if len(t.raw) != last:
            last, stamp = len(t.raw), time.time()
        elif time.time() - stamp >= settle:
            return
    raise TimeoutError('no quiet period')


def typeline(t, line):
    for ch in line + b'\r':
        t.s.sendall(bytes([ch]))
        time.sleep(0.03)
        t._pump(0.01)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('script')
    ap.add_argument('out')
    ap.add_argument('--settle', type=float, default=1.5)
    a = ap.parse_args()
    cmds = [l.rstrip('\r\n') for l in open(a.script, encoding='latin-1') if not l.startswith('#')]

    t = Term()
    t.raw = bytearray()
    orig = t._pump

    def pump(dt, _o=orig):
        n = len(t.buf)
        _o(dt)
        t.raw.extend(t.buf[n:])
    t._pump = pump

    t.send(b'\x1b', cr=False)
    banner = t.expect('ENTER', 20).decode('latin-1')
    host_at_banner = datetime.now()
    m = re.search(r'(\d\d)\.(\d\d)\.(\d\d)\s+(\d+)\s+([A-Z]+)\s+(\d{4})', banner)
    sin = datetime(int(m.group(6)), MONTHS.index(m.group(5)) + 1, int(m.group(4)),
                   int(m.group(1)), int(m.group(2)), int(m.group(3)))
    offset = sin - host_at_banner              # SINTRAN time = host time + offset
    t.send('GAMES')
    t.expect('PASSWORD', 20)
    t.send('')
    t.expect('@', 20)
    quiet(t, 0.5)
    while True:                                # start within seconds 2..20 of a minute
        s = (datetime.now() + offset).second
        if 2 <= s <= 20:
            break
        time.sleep(0.2)
    started = datetime.now() + offset
    start = len(t.raw)
    t.send('SVH')
    t.expect(r'instructions\?', 240)
    quiet(t, a.settle)
    for c in cmds:
        typeline(t, c.encode('latin-1'))
        quiet(t, a.settle)
    body = bytes(t.raw[start:])
    if not bytes(b & 0x7f for b in t.raw[-40:]).rstrip().endswith(b'@'):
        typeline(t, b'QUIT')
        quiet(t, a.settle)
        typeline(t, b'YES')
        quiet(t, a.settle)
    t.send('LOG OUT')
    quiet(t, 0.5)
    open(a.out, 'wb').write(body)
    shifted = started.replace(year=started.year + 28, second=0)
    meta = {'sintran_start': started.strftime('%Y-%m-%d %H:%M:%S'),
            'clock': int(time.mktime(shifted.timetuple())) + 30}
    json.dump(meta, open(a.out + '.json', 'w'), indent=1)
    print(meta)


if __name__ == '__main__':
    main()
