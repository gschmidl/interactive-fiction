"""Play random games of the adventure on SINTRAN III under RetroCore and record them.

usage: reffuzz.py FIRST LAST [--commands N] [--original] [--out DIR] [--script FILE]
       (RetroCore must not be running: this starts and stops it)

With --script the commands of FILE are typed instead, one at each prompt (its
# lines, the debug pokes among them, left out), and the session is named
after the file: reffuzz.py 1 1 --script tests\\walk-debug.txt --out tests\\walk

Each seed from FIRST to LAST plays one game on the reference machine (NOTES.md,
"The reference machine") as user DNF, with tools\\play.py's random player:
ADV-INTER-CB-MJ (the port's program) with CAVE-FUN-MJ:ADV (the port's game
file), or with --original AIC (the interpreter as recovered) with CAVE-ORIG:ADV
(the game as recovered).  Saved games (SAV1, SAV2, ...) are deleted before each
game.  Every answer is typed only once the program has asked for it.

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
from play import Player, ScriptPlayer  # noqa: E402

ENDED = re.compile(r'\r\n@$')


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


def command(t, cmd):
    t.send(cmd)
    end = time.time() + 30
    while time.time() < end:
        t._pump(0.1)
        if re.search(rb'\r\n@ ?$', bytes(b & 0x7f for b in t.buf[-100:])):
            break
    t._pump(0.3)
    t.buf.clear()


def play(t, seed, commands, original, outbase, timeout=120, script=None):
    for n in range(1, 30):
        command(t, 'DELETE-FILE SAV%d:SYMB' % n)
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

    t.send('AIC' if original else 'ADV-INTER-CB-MJ')
    name = 'CAVE-ORIG' if original else 'CAVE-FUN'
    player = ScriptPlayer(script, name) if script else Player(seed, commands, name)
    try:
        while True:
            since = len(t.raw)
            deadline = time.time() + timeout
            choice = None
            while time.time() < deadline:
                t._pump(0.1)
                tail = text(max(0, len(t.raw) - 3000))
                if ENDED.search(bytes(b & 0x7f for b in t.raw[-50:]).decode('latin-1')):
                    break
                if len(t.raw) > since:
                    choice = player.answer(tail)
                    if choice:
                        t._pump(0.3)          # anything more to come?
                        if text(max(0, len(t.raw) - 3000)) == tail:
                            break
                        choice = None
            if ENDED.search(bytes(b & 0x7f for b in t.raw[-50:]).decode('latin-1')):
                break
            if choice is None:
                raise TimeoutError('no prompt recognised: %r' % text(max(0, len(t.raw) - 300)))
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
    ap.add_argument('--original', action='store_true')
    ap.add_argument('--out')
    ap.add_argument('--script', help="type this file's commands instead (tests\\walk-debug.txt)")
    a = ap.parse_args()
    out = a.out or os.path.join(rc.PORT, 'tests', 'ref-original' if a.original else 'ref')
    os.makedirs(out, exist_ok=True)
    with rc.Machine():
        t = Term()
        login(t)
        for seed in range(a.first, a.last + 1):
            base = os.path.join(out, 'game%02d' % seed)
            script = None
            if a.script:
                script = open(a.script).read().split('\n')
                base = os.path.join(out, os.path.splitext(os.path.basename(a.script))[0])
            try:
                play(t, seed, a.commands, a.original, base, script=script)
                print('game%02d recorded' % seed)
            except Exception as e:           # keep going; the prefix is kept
                print('game%02d: %s' % (seed, e))
                t.send(b'\x1b', cr=False)
                t._pump(2)
                t.buf.clear()
        t.send('LOGOUT')
        t._pump(1)


if __name__ == '__main__':
    main()
