"""Play a random game of LEGEND on SINTRAN III under RetroCore and record it.

usage: reffuzz.py SEED COMMANDS OUTBASE [--original] [--timeout SECONDS]

The machine is the RetroCore pack of NOTES.md, "The reference machine",
with the game's files put there by tools\\rebuild.py.  The session logs in as
DNF, puts back every file a game changes, from the pristine copies under user
LEGENDORIG (SPELARE-1..9, SAKKARE-1..9, VEMFIL, BORT; LEGEND-MSG and
ACC-LEGEND empty), and starts LEGENDF-REF (the port's program) or, with
--original, LEGEND-REF (the recovered source as it was).  The typing comes
from tools\\play.py, each answer sent only once the game has printed its
prompt: LEGEND clears the input buffer (CIBUF) before most prompts, so keys
typed ahead would be lost on SINTRAN and kept by a pipe.

Writes OUTBASE.raw (the program's output from the echo of its name on),
OUTBASE.keys (every byte typed to the program) and OUTBASE.log (the choices;
its first line gives the clock the title showed and the terminal's logical
device number, for replaying with -Z and --terminal).
They are written after every answer, so an interrupted run leaves its prefix.
"""
import argparse
import os
import re
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ndtel import Term  # noqa: E402
from play import Player  # noqa: E402

RESTORE = (['SPELARE-%d-LU:DATA' % n for n in range(1, 10)] +
           ['SAKKARE-%d-LU:DATA' % n for n in range(1, 10)] +
           ['VEMFIL-LU:DATA', 'BORT-LU:DATA'])
EMPTY = ['LEGEND-MSG-LU:DATA', 'ACC-LEGEND-LU:DATA']
ENDED = re.compile(r'\r\n@$')
TITLE = re.compile(r'Legend +v10\.0 +(\d+)\. ?(\d+) +(\d+)/ ?(\d+) (\d{4})')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('seed', type=int)
    ap.add_argument('commands', type=int)
    ap.add_argument('outbase')
    ap.add_argument('--original', action='store_true')
    ap.add_argument('--timeout', type=float, default=300)
    a = ap.parse_args()

    t = Term()
    t.raw = bytearray()
    orig = t._pump

    def pump(dt, _o=orig):
        n = len(t.buf)
        _o(dt)
        t.raw.extend(t.buf[n:])
    t._pump = pump
    keys, log = bytearray(), []

    def text(since):
        return bytes(b & 0x7f for b in t.raw[since:]).decode('latin-1').replace('\0', '')

    def command(cmd, quiet=0.4):
        t.send(cmd)
        end = time.time() + 30
        while not re.search(r'\r\n@ ?$', text(max(0, len(t.raw) - 200))) and time.time() < end:
            t._pump(0.1)
        t._pump(quiet)

    def save():
        open(a.outbase + '.raw', 'wb').write(bytes(t.raw[start:]))
        open(a.outbase + '.keys', 'wb').write(bytes(keys))
        open(a.outbase + '.log', 'w', encoding='latin-1').write('\n'.join(log) + '\n')

    t.send(b'\x1b', cr=False)
    banner = t.expect('ENTER', 60).decode('latin-1')
    m = re.search(r'to TERMINAL [^(\r\n]*\((\d+)\)', banner)
    terminal = m.group(1) if m else '?'
    t.send('DNF')
    t.expect('PASSWORD', 20)
    t.send('')
    t.expect(r'@', 30)
    t._pump(0.5)
    for f in RESTORE:
        command('DELETE-FILE %s' % f)
        command('COPY-FILE "%s" (LEGENDORIG)%s' % (f, f))
    for f in EMPTY:
        command('DELETE-FILE %s' % f)
        command('CREATE-FILE %s,0' % f)
    t._pump(1.0)

    start = len(t.raw)
    t.send('LEGENDF-REF' if not a.original else 'LEGEND-REF')
    player = Player(a.seed, a.commands)
    clock = None
    end_all = time.time() + a.timeout * 20
    while time.time() < end_all:
        # wait for a prompt: output has arrived and the game has asked something
        since = len(t.raw)
        deadline = time.time() + a.timeout
        choice = None
        while time.time() < deadline:
            t._pump(0.1)
            tail = text(max(start, len(t.raw) - 2000))
            if ENDED.search(tail):
                break
            if len(t.raw) > since or since == start:
                choice = player.answer(tail)
                if choice:
                    t._pump(0.3)              # anything more to come?
                    tail2 = text(max(start, len(t.raw) - 2000))
                    if tail2 == tail:
                        break
                    choice = None
        if clock is None:
            m = TITLE.search(text(start))
            if m:
                clock = '%s-%s-%s %s:%s' % (m.group(5), m.group(4), m.group(3), m.group(1), m.group(2))
                log.insert(0, 'clock %s terminal %s' % (clock, terminal))
        if ENDED.search(text(max(start, len(t.raw) - 200))):
            break
        if choice is None:
            save()
            raise TimeoutError('no prompt recognised: %r' % text(max(start, len(t.raw) - 300)))
        k, why = choice
        t.send(k.encode('latin-1'), cr=False)
        keys.extend(k.encode('latin-1'))
        log.append('%s %-24r %s' % (time.strftime('%H:%M:%S'), k, why))
        save()
    save()
    t.send('LOGOUT')
    t._pump(0.5)


if __name__ == '__main__':
    main()
