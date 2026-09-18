"""Play MY_WORLD through: every puzzle, every room there is a way to, every
treasure to the cottage.  The game has no end of its own; this is its best.

usage: python tests\\playthrough.py [-v]

In the reference world (-Z), typed as a player would, with two --debug pokes
after every command (tools\\findvars.py finds where): the orc and its bola
taken away (a battle-axe kills at random, once they have come), and what
the pirate snatched given back (he hides it in his den, in the maze, which
the route passes before it; it would only be a longer walk).

The route: round the forest; to the cottage, the lamp, the keys, the
bottle; the gate (OPEN GATE); the cage, the bird (GET BIRD: not while the
plank is carried), the plank; the fissure (MAKE BRIDGE); the gold, the
mithril, the soft room, the animal room, the troll's bridge (not crossed:
it costs a treasure); the snake (FREE BIRD), the bird again, the silver; the
pits; the maze, all of it, and the pirate's chest; out by the stalactite, and
XYZZY to the cottage, where everything is dropped and the bird freed.  It
must end with every treasure there, the score 213 (45 rooms of 47 and the
gate, the bridge and the snake: the two beyond the troll are not seen) and
"an expert adventurer".  -v prints the session.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
sys.path.insert(0, os.path.join(PORT, 'tools'))
from findvars import SIZE, find  # noqa: E402
from portfuzz import Run  # noqa: E402

EXE = os.path.join(PORT, 'myworld.exe')
DATA = os.path.join(PORT, 'data')
PROG = os.path.join(DATA, 'ADVENTURE-MJ.PROG')
TREASURES = {11: 'Silver bars', 17: 'Nugget of gold', 18: 'Mithril nugget', 20: 'Treasure chest'}

# (command, the room it leads to, or what the answer must have)
ROUTE = [
    ('N', 3), ('NE', 4), ('SE', 5), ('NE', 6), ('N', 1), ('N', 2), ('S', 5), ('SE', 7), ('S', 8), ('S', 9),
    ('E', 10), ('GET LAMP', 'Lamp: Taken.'), ('GET KEYS', 'Set of keys: Taken.'),
    ('GET BOTTLE', 'Bottle: Taken.'), ('LIGHT LAMP', 'The lamp is now on!'),
    ('W', 9), ('S', 11), ('S', 12), ('OPEN GATE', 'The gate is now open!'), ('S', 13), ('S', 14),
    ('GET CAGE', 'Wicker cage: Taken.'), ('S', 15), ('SE', 25), ('GET BIRD', 'Bird: taken.'), ('NW', 15),
    ('GET PLANK', 'Plank: Taken.'), ('SE', 25), ('SE', 16), ('D', 17), ('W', 18),
    ('MAKE BRIDGE', 'The plank now lies over the fissure!'), ('W', 20), ('W', 26), ('N', 27),
    ('GET NUGGET', 'Nugget of gold: Taken.'), ('S', 26), ('S', 28), ('S', 29), ('S', 30),
    ('GET MITHRIL', 'Mithril nugget: Taken.'), ('SW', 31), ('W', 33), ('E', 31), ('NE', 30), ('SE', 32),
    ('S', 34), ('N', 32), ('NW', 30), ('N', 29), ('N', 28), ('N', 26), ('E', 20), ('E', 18), ('E', 17),
    ('E', 19), ('E', 21), ('FREE BIRD', 'The little bird attacks the snake, and drives it off!'),
    ('GET BIRD', 'Bird: taken.'), ('NE', 22), ('GET SILVER', 'Silver bars: Taken.'), ('SW', 21), ('SW', 23),
    ('D', 24), ('U', 23), ('NE', 21), ('NE', 22), ('N', 37), ('N', 41), ('S', 39), ('SW', 38), ('S', 40),
    ('N', 42), ('S', 44), ('N', 38), ('N', 45), ('U', 46), ('S', 47),
]
HOME = [
    ('N', 46), ('U', 43), ('D', 25), ('NW', 15), ('N', 14), ('XYZZY', 10),
    ('DROP NUGGET', 'Dropped.'), ('DROP MITHRIL', 'Dropped.'), ('DROP SILVER', 'Dropped.'),
    ('DROP CHEST', 'Dropped.'), ('FREE BIRD', 'Freed.'), ('SCORE', 'points'),
]


class Session:
    def __init__(self, v):
        self.v = v
        self.r = Run([EXE, '--data', DATA, '--prog', PROG, '--raw', '--no-hold', '-Z', '1', '--debug'])
        self.r.settle(0)
        self.carried = set()
        self.log = []

    def peek(self, a):
        since = self.r.size
        self.r.send('#peek %o 1\r' % a)
        self.r.settle(since)
        return int(re.search(r'%06o: (\d+)' % a, self.r.text()[since:]).group(1), 8)

    def poke(self, a, value):
        since = self.r.size
        self.r.send('#poke %o %d.\r' % (a, value))
        self.r.settle(since)

    def say(self, command, want):
        since = self.r.size
        self.r.send(command + '\r')
        self.r.settle(since)
        out = self.r.text()[since:].replace('\r', '')
        self.log.append(out)
        if 'patch you?' in out or 'IN LINE' in out:
            raise AssertionError('%r: %r' % (command, out[-300:]))
        v = self.v
        self.poke(v['ORC'], 0)
        self.poke(v['BOLA'], 0)
        self.poke(v['SAK'] + SIZE * 4, 0)                  # the orc, SAK[5]
        if 'bearded pirate' in out:
            for k in self.carried:
                self.poke(v['SAK'] + SIZE * (k - 1), -1)
        for k, name in TREASURES.items():
            if '%s: Taken.' % name in out:
                self.carried.add(k)
        if isinstance(want, int):
            if self.peek(v['RUMNR']) != want:
                raise AssertionError('%r: not in room %d: %r' % (command, want, out[-300:]))
        elif want not in out:
            raise AssertionError('%r: no %r in %r' % (command, want, out[-300:]))
        return out


def main():
    v = find(PROG)
    s = Session(v)
    try:
        for command, want in ROUTE:
            s.say(command, want)
        for _ in range(400):                               # the chest comes and goes
            if 'chest' in s.say('LOOK', 'pirates').split('pirates', 1)[1].lower():
                if 'Taken.' in s.say('GET CHEST', 'Command'):
                    break
        else:
            raise AssertionError('the chest never came')
        for command, want in HOME:
            end = s.say(command, want)
    finally:
        s.r.p.kill()
        if '-v' in sys.argv:
            print(re.sub(r'\n+', '\n', s.r.text().replace('\r', '')))
    print(re.sub(r'\n+', '\n', end).strip())
    ok = 'reached 213 points' in end and 'expert adventurer' in end and 'explored  96 %' in end
    print('played through' if ok else 'NOT AS IT SHOULD')
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
