"""Play a random game of MORDOR on SINTRAN III under RetroCore and record it.

usage: reffuzz.py SEED COMMANDS OUTBASE [--prog MORDORF-REF] [--keep-map] [--original]

The machine is the one refcap.py describes.  The typing is chosen as the game
asks (tools\\play.py): at "Command:" a random command or move, at the fight
questions T/I or F/R, at a name prompt someone in the fellowship.  A seed
always plays the same game, and after COMMANDS commands it types QUIT-GAME.
Every key is sent only once the game has taken the one before (Esc typed
ahead would break the program).

Writes OUTBASE.raw (the program's output as received, from SINTRAN's echo of
the command on), OUTBASE.keys (every byte typed after the command, for
replaying into the port; the DEL typed ahead to pass the day-time lock is not
in it: replay with --unlimited) and OUTBASE.log (the choices, with times).
They are written after every answer, so an interrupted run leaves its prefix.
"""
import argparse
import os
import re
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ndtel import Term  # noqa: E402
from play import PROMPT, Player  # noqa: E402

ENDED = re.compile(r'\r\n@$')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('seed', type=int)
    ap.add_argument('commands', type=int)
    ap.add_argument('outbase')
    ap.add_argument('--prog', default='MORDORF-REF')
    ap.add_argument('--keep-map', action='store_true')
    ap.add_argument('--original', action='store_true',
                    help='the program as recovered (names typed shifted); use with --prog MORDOR-REF')
    ap.add_argument('--timeout', type=float, default=600)
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
        return bytes(b & 0x7f for b in t.raw[since:]).decode('latin-1')

    def quiet(settle):
        last, stamp = len(t.raw), time.time()
        while time.time() - stamp < settle:
            t._pump(0.05)
            if len(t.raw) != last:
                last, stamp = len(t.raw), time.time()

    def wait(since, rx):
        pat = re.compile(rx, re.S) if isinstance(rx, str) else rx
        end = time.time() + a.timeout
        while not (pat.search(text(since)) or ENDED.search(text(since))):
            if time.time() > end:
                save()
                raise TimeoutError('waiting for %r; the end of the output: %r' % (pat.pattern, text(since)[-300:]))
            t._pump(0.05)

    def send(k, why):
        mark = len(t.raw)
        t.send(k.encode('latin-1'), cr=False)
        keys.extend(k.encode('latin-1'))
        log.append('%s %-22r %s' % (time.strftime('%H:%M:%S'), k, why))
        return mark

    def save():
        open(a.outbase + '.raw', 'wb').write(bytes(t.raw[start:]))
        open(a.outbase + '.keys', 'wb').write(bytes(keys))
        open(a.outbase + '.log', 'w', encoding='latin-1').write('\n'.join(log) + '\n')

    t.send(b'\x1b', cr=False)
    t.expect('ENTER', 60)
    t.send('DNF')
    t.expect('PASSWORD', 20)
    t.send('')
    t.expect(r'@', 30)
    quiet(0.5)
    if not a.keep_map:
        t.send('DELETE-FILE MORDOR-MAP-MJ:DATA')
        quiet(1.0)
        t.send('CREATE-FILE MORDOR-MAP-MJ:DATA,0')
        quiet(1.0)
    start = len(t.raw)
    t.send((a.prog + '\r\x7f').encode(), cr=False)
    wait(start, r'<N>: $')

    player = Player(a.seed, a.commands, a.original)
    for k, why, rx in player.opening():
        mark = send(k, why)
        wait(mark, rx)
        # the instructions: a page at a time, before the hero screen
        while text(max(start, len(t.raw) - 100)).endswith('finished reading: '):
            quiet(0.3)
            mark = send('\r', 'page')
            wait(mark, rx)
    save()
    while True:
        tail = text(max(start, len(t.raw) - 2000))
        if ENDED.search(tail):
            break
        choice = player.answer(tail)
        if choice is None:
            quiet(1.0)
            if text(max(start, len(t.raw) - 2000)) == tail:
                save()
                raise RuntimeError('no prompt recognised: %r' % tail[-300:])
            continue
        mark = send(*choice)
        wait(mark, PROMPT)
        quiet(0.1)
        save()
    save()
    t.send('LOGOUT')
    quiet(0.5)


if __name__ == '__main__':
    main()
