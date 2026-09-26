"""Play ADVENTURE-ENB to the end with the port's --debug pokes: can it be won?

usage: python tests\\winnable.py [-v]

In the reference world (-Z), with the monsters above ground taken away,
defence, armour and strength made large (the caves' monsters still come, and
are fought), and the long ways above ground gone over by moving the player
(tools\\findvars.py finds where the program keeps all that):

  take water from the river by the bridge; down into the cave; walk the
  caves until the lamp lies in a room, and take it (T); walk on until the
  keys lie in one (the lamp shows the rest), and take them; walk to the
  princess (15,1, behind two locked doors); ask her to follow (F); give her
  the water and the lamp (G, vatten / en lampa, PR); walk back to the way
  up and go up (U); onto a castle, and rest there (V)

The princess must be friendly enough, STAT 30: asking gives 10, and each of
the two gifts 15.  (One gift of "vatten" takes all the water carried: line
3710 empties every place that holds what was given.)  The game must end
"Du har lyckats fullst{ndigt".  -v prints the session.
"""
import os
import random
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
sys.path.insert(0, os.path.join(PORT, 'tools'))
from findvars import EXE, real, find  # noqa: E402
from portfuzz import Run  # noqa: E402

FIXED = os.path.join(PORT, 'data', 'ADVENTURE-ENB.PROG')
ARROW = {'N': '\x1bA', 'S': '\x1bB', 'E': '\x1bC', 'W': '\x1bD'}


class Session:
    def __init__(self, prog):
        self.r = Run([EXE, '--prog', prog, '--raw', '--no-hold', '-Z', '1', '--debug'])

    def say(self, keys, want=None):
        """type keys; fight any monster that comes (D, Return); the output"""
        r = self.r
        since = r.size
        r.send(keys)
        r.settle(since)
        while True:
            seg = r.tail(600)
            seg = seg[seg.rfind('ORDER:', 0, len(seg) - 6):]
            if seg.endswith('ORDER:') and ('Pl|tsligt' in seg or 'Det kan du inte g|ra nu' in seg):
                more = 'D'
            elif re.search(r"'RETURN' ?f\|r att forts\{tta (sl\}ss|striden)$", seg):
                more = '\r'
            else:
                break
            n = r.size
            r.send(more)
            r.settle(n)
        out = r.text()[since:].replace('\r', '')
        if want and want not in out:
            raise AssertionError('%r: no %r in %r' % (keys, want, out[-400:]))
        return out

    def poke(self, a, words):
        self.say('#poke %o %s\r' % (a, ' '.join('%o' % w for w in words)), 'ok')

    def peek_real(self, a):
        out = self.say('#peek %o 3\r' % a)
        e, h, l = [int(x, 8) for x in re.search(r'%06o: (\d+) (\d+) (\d+)' % a, out).groups()]
        return round(((h << 16) | l) / 2 ** 32 * 2 ** ((e & 0o77777) - 0o40000)) if e else 0


def main():
    v = find(FIXED)
    s = Session(FIXED)
    rnd = random.Random(1)
    log = []

    def where():
        return s.peek_real(v['G3']), s.peek_real(v['G4'])

    def walk_to(g3, g4):
        for _ in range(80):
            at = where()
            if at == (g3, g4):
                return
            d = 'N' if at[0] < g3 else 'S' if at[0] > g3 else 'E' if at[1] < g4 else 'W'
            out = s.say(ARROW[d])
            assert 'kan inte g} genom' not in out, out
        raise AssertionError('never got to %d,%d' % (g3, g4))

    def find_thing(thing):
        """walk about until thing lies in the room, and take it"""
        for n in range(400):
            out = s.say(ARROW[rnd.choice('NSEW')])
            if 'I ett h|rn st}r %s' % thing in out:
                s.say('T', '%s medtaget' % thing)
                log.append('%s taken after %d steps' % (thing, n + 1))
                return
        raise AssertionError('no %s in 400 steps' % thing)

    try:
        s.say('N\r', 'ORDER:')
        s.poke(v['PAMON'], [0] * 31)
        s.poke(v['OFMON'], [0] * 31)
        for name, x in (('FORSV', 1000), ('SKYDD', 1000), ('STRENGTH', 10000)):
            s.poke(v[name], real(x))
        s.say('\x1bC', 'en flod')
        s.say('T', 'vatten medtaget')
        s.poke(v['NS'], real(v['cave'][0]))
        s.poke(v['EW'], real(v['cave'][1]))
        s.say('N', 'nere i grottorna')
        way_up = where()
        find_thing('en lampa')
        find_thing('en nyckelknippa')
        walk_to(15, 1)
        s.say('F', 'Prinsessan f|ljer dig')
        for gift in ('vatten', 'en lampa'):
            s.say('G', 'Skriv vad du vill ge')
            s.say(gift + '\r', 'Till vem')
            s.say('PR\r', 'Prinsessan neg och tog emot')
        walk_to(*way_up)
        s.say('U', 'Upp')
        s.poke(v['NS'], real(v['castle'][0]))
        s.poke(v['EW'], real(v['castle'][1]))
        end = s.say('V')
    finally:
        s.r.p.kill()
        if '-v' in sys.argv:
            print(re.sub(r'\n+', '\n', s.r.text().replace('\r', '')))
    won = 'Du har lyckats fullst{ndigt' in end
    print('\n'.join(log))
    print('way up at %d,%d; castle at %d,%d' % (way_up + v['castle']))
    print(re.sub(r'\n+', '\n', end).strip())
    print('won' if won else 'NOT WON')
    return 0 if won else 1


if __name__ == '__main__':
    sys.exit(main())
