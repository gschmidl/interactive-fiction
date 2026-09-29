"""Play random games of ADVENTURE-ENB on SINTRAN III under RetroCore and record them.

usage: reffuzz.py FIRST LAST [--commands N] [--start] [--out DIR] [--script FILE ...]
       (RetroCore must not be running: this starts and stops it)

Each seed from FIRST to LAST plays one game on the reference machine (NOTES.md,
"The reference machine") as user DNF, with tools\\play.py's random player:
AENBF-REF (the port's program, with RANDOM not stirring in the uptime, so that
the world is the same every game), or with --start AENBS-REF (the program as
recovered, with only the error handlers it needs to start mended).  Every
answer is typed only once the program has asked for it.

Then each --script FILE is typed: the answers are its lines, one at each
question (# lines left out; see ScriptPlayer), into tests\\walk\\ (or
--script-out DIR), the session named after the file.  The scripts named
start-* are for --start:

    reffuzz.py 1 0 --start --script tests\\scripts\\start-water.txt

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
KEYS = {'{N}': '\x1bA', '{S}': '\x1bB', '{E}': '\x1bC', '{W}': '\x1bD', '{H}': '\x1bH'}


class ScriptPlayer:
    """Types a script's lines, one at each question: at ORDER: the line is the
    key(s), {N} {S} {E} {W} for the arrow keys and {H} for Home; at any other
    question (output that stops short of the end of a line) it is typed and
    Return after it.  The Tryck 'RETURN' questions are answered with Return,
    not from the script.  When the script is done it ends the game with S."""

    def __init__(self, lines):
        self.lines = [l.rstrip('\r') for l in lines if l.strip() and not l.startswith('#')]
        self.done = 0

    def answer(self, tail):
        t = tail.rstrip('\0\x7f')           # the program as recovered prompts with DEL
        if re.search(r"(TRYCK|Tryck) 'RETURN'[^\n]*$", t):
            return '\r', 'return'
        order = re.search(r'ORDER:$', t)
        if not order and (not t or t.endswith('\n')):
            return None
        if self.done >= len(self.lines):
            return ('S', 'end of the script') if order else ('\r', 'end of the script')
        line = self.lines[self.done]
        self.done += 1
        for k, v in KEYS.items():
            line = line.replace(k, v)
        return (line if order else line + '\r'), 'script %d' % self.done


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
    """end a game the player lost its way in: Esc does nothing (the program
    switches the escape function off), so S at ORDER:, Return at anything
    else, until SINTRAN's @; if it will not end, the session stops"""
    for _ in range(40):
        t._pump(1.0)
        tail = bytes(b & 0x7f for b in t.buf[-200:]).decode('latin-1').rstrip('\0\x7f')
        if ENDED.search(tail):
            t.buf.clear()
            return
        t.send(b'S' if tail.endswith('ORDER:') else b'\r', cr=False)
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
            t.send(k.encode('latin-1'), cr=False)
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
    ap.add_argument('--start', action='store_true', help='AENBS-REF, the program as recovered, started')
    ap.add_argument('--out')
    ap.add_argument('--script', action='append', default=[], help="then type this file's answers")
    ap.add_argument('--script-out', default=os.path.join(rc.PORT, 'tests', 'walk'))
    a = ap.parse_args()
    out = a.out or os.path.join(rc.PORT, 'tests', 'ref-start' if a.start else 'ref')
    program = 'AENBS-REF' if a.start else 'AENBF-REF'
    games = [(os.path.join(out, 'game%02d' % seed), Player(seed, a.commands))
             for seed in range(a.first, a.last + 1)]
    games += [(os.path.join(a.script_out, os.path.splitext(os.path.basename(f))[0]),
               ScriptPlayer(open(f, encoding='latin-1').read().split('\n'))) for f in a.script]
    for base, _ in games:
        os.makedirs(os.path.dirname(base), exist_ok=True)
    with rc.Machine():
        t = Term()
        login(t)
        for base, player in games:
            try:
                play(t, player, program, base)
                print('%s recorded' % os.path.basename(base))
            except Exception as e:           # keep going; the prefix is kept
                print('%s: %s' % (os.path.basename(base), e))
                end_program(t)
        t.send('LOGOUT')
        t._pump(1)


if __name__ == '__main__':
    main()
