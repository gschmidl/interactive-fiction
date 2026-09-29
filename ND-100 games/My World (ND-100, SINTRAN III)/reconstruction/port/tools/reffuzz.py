"""Play random games of MY_WORLD on SINTRAN III under RetroCore and record them.

usage: reffuzz.py FIRST LAST [--commands N] [--original] [--out DIR] [--script FILE ...]
       (RetroCore must not be running: this starts and stops it)

Each seed from FIRST to LAST plays one game on the reference machine (NOTES.md,
"The reference machine") as user DNF, with tools\\play.py's random player:
AMJF-REF (the port's program, with RAN not stirring in the uptime, so that the
same typing meets the same dice), or with --original AMJ-REF (the program as
recovered).  Every answer is typed only once the program has asked for it.

Then each --script FILE is typed: the answers are its lines, one at each
question (# lines left out), into tests\\walk\\ (or --script-out DIR), the
session named after the file; the scripts named original-* are typed into
AMJ-REF.  A line longer than 16 characters is typed 16 at a time: SINTRAN's
input buffer holds 72 characters typed ahead, and loses the rest.

Writes DIR\\gameNN.raw (what the terminal got from the program's name on),
gameNN.keys (every byte typed to the program) and gameNN.log (the choices).
"""
import argparse
import os
import re
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rc  # noqa: E402
from ndtel import Term  # noqa: E402
from play import Player  # noqa: E402

ENDED = re.compile(r'\r\n@ ?$')


class ScriptPlayer:
    """Types a script's lines, one at each question (output that stops short
    of the end of a line), Return after each; then QUIT."""

    def __init__(self, lines):
        self.lines = [l.rstrip('\r') for l in lines if l.strip() and not l.startswith('#')]
        self.done = 0

    def answer(self, tail):
        t = tail.rstrip('\0')
        if not t or t.endswith('\n'):
            return None
        self.done += 1
        if self.done > len(self.lines):
            return 'QUIT\r', 'end of the script'
        return self.lines[self.done - 1] + '\r', 'script %d' % self.done


def login(t):
    t.send(b'\x1b', cr=False)
    t.expect(r'ENTER|@\s*$', 120)
    t._pump(1.0)
    t.buf.clear()
    t.send(b'\x1b', cr=False)
    t._pump(1.0)
    if b'ENTER' in bytes(b & 0x7f for b in t.buf):
        t.buf.clear()
        t.send('DNF')
        t.expect('PASSWORD', 20)
        t.send('')
        t.expect(r'@', 30)
    t._pump(0.5)
    t.buf.clear()


def end_program(t):
    """end a game that is still going: Esc, SINTRAN's user break, and @"""
    for _ in range(10):
        t.send(b'\x1b', cr=False)
        t._pump(1.0)
        tail = bytes(b & 0x7f for b in t.buf[-200:]).decode('latin-1').rstrip('\0')
        if ENDED.search(tail):
            t.buf.clear()
            return
    raise SystemExit('the program would not end: session stopped')


def play(t, player, program, outbase, timeout=120):
    t.raw = bytearray()
    orig = t._pump

    def pump(dt, _o=orig):
        n = len(t.buf)
        _o(dt)
        t.raw.extend(t.buf[n:])
    t._pump = pump
    keys, log = bytearray(), []

    def text(since):
        return bytes(b & 0x7f for b in t.raw[since:]).decode('latin-1').replace('\0', '').replace('\r', '')

    def save():
        open(outbase + '.raw', 'wb').write(bytes(t.raw))
        open(outbase + '.keys', 'wb').write(bytes(keys))
        open(outbase + '.log', 'w', encoding='latin-1').write('\n'.join(log) + '\n')

    def ended():
        return ENDED.search(bytes(b & 0x7f for b in t.raw[-50:]).decode('latin-1'))

    t.send(program)
    try:
        while True:
            since = len(t.raw)
            deadline = time.time() + timeout
            choice = None
            while time.time() < deadline:
                t._pump(0.1)
                tail = text(max(0, len(t.raw) - 3000))
                if ended():
                    break
                if len(t.raw) > since:
                    choice = player.answer(tail)
                    if choice:
                        t._pump(0.3)          # anything more to come?
                        if text(max(0, len(t.raw) - 3000)) == tail:
                            break
                        choice = None
            if ended():
                break
            if choice is None:
                raise TimeoutError('no question recognised: %r' % text(max(0, len(t.raw) - 300)))
            k, why = choice
            for i in range(0, len(k), 16):        # SINTRAN's input buffer holds 72 typed
                t.send(k[i:i + 16].encode('latin-1'), cr=False)     # ahead: a long line
                if len(k) > 16:                   # all at once loses the rest
                    t._pump(0.3)
            keys.extend(k.encode('latin-1'))
            log.append('%-24r %s' % (k, why))
            save()
    finally:
        save()
        t._pump = orig
        t.buf.clear()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('first', type=int)
    ap.add_argument('last', type=int)
    ap.add_argument('--commands', type=int, default=150)
    ap.add_argument('--original', action='store_true', help='AMJ-REF, the program as recovered')
    ap.add_argument('--out')
    ap.add_argument('--script', action='append', default=[], help="then type this file's answers")
    ap.add_argument('--script-out', default=os.path.join(rc.PORT, 'tests', 'walk'))
    a = ap.parse_args()
    out = a.out or os.path.join(rc.PORT, 'tests', 'ref-original' if a.original else 'ref')
    program = 'AMJ-REF' if a.original else 'AMJF-REF'
    games = [(os.path.join(out, 'game%02d' % seed), Player(seed, a.commands))
             for seed in range(a.first, a.last + 1)]
    games = [(base, player, program) for base, player in games]
    games += [(os.path.join(a.script_out, os.path.splitext(os.path.basename(f))[0]),
               ScriptPlayer(open(f, encoding='latin-1').read().split('\n')),
               'AMJ-REF' if os.path.basename(f).startswith('original-') else program) for f in a.script]
    for base, _, _ in games:
        os.makedirs(os.path.dirname(base), exist_ok=True)
    with rc.Machine():
        t = Term()
        login(t)
        for base, player, program in games:
            try:
                play(t, player, program, base)
                print('%s recorded' % os.path.basename(base), flush=True)
            except SystemExit:
                raise
            except Exception as e:           # keep going; the prefix is kept
                print('%s: %s' % (os.path.basename(base), e), flush=True)
                end_program(t)
        t.send('LOGOUT')
        t._pump(1)


if __name__ == '__main__':
    main()
