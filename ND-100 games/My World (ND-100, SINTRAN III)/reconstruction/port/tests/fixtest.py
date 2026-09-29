"""Show each of the port's fixes: the same typing on the program as recovered
and as the port ships it.

usage: python tests\\fixtest.py [-v]

The program as recovered stops at its first command (the first three cases
show where), so the last two use build\\start\\: the program as recovered
with only lines 263, 289 and 741 mended.  Scenes are set with --debug pokes
(tools\\findvars.py finds the player's room and the things in each build), in
the reference world (-Z).  -v prints the end of every answer.
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
sys.path.insert(0, os.path.join(PORT, 'tools'))
from findvars import SIZE, find  # noqa: E402

EXE = os.path.join(PORT, 'myworld.exe')
DATA = os.path.join(PORT, 'data')
ORIGINAL = os.path.join(PORT, 'build', 'original', 'ADVENTURE-MJ.PROG')
START = os.path.join(PORT, 'build', 'start', 'ADVENTURE-MJ.PROG')
FIXED = os.path.join(DATA, 'ADVENTURE-MJ.PROG')


def poke(a, value):
    return '#poke %o %d.\r' % (a, value)


def room(v, n):
    return poke(v['RUMNR'], n)


def thing(v, k, where):
    """SAK[k].RUM: a room, -1 carried, 0 nowhere"""
    return poke(v['SAK'] + SIZE * (k - 1), where)


# name: (what it shows, the recovered program, keys (v -> str), its answer, the fix's)
CASES = [
    ('separators', 'the first command stopped the game: character 0, a separator, was skipped for ever',
     ORIGINAL, lambda v: 'N\r', 'ARITHMETIC OVERFLOW\n IN LINE 226', 'Command: N\nYou are in a forest'),
    ('pirate', 'the pirate\'s chance, 0 treasures in 150, stopped it too (got to by a command of 80 letters)',
     ORIGINAL, lambda v: 'N' + ' ' * 78 + 'X\r', 'ARITHMETIC OVERFLOW\n IN LINE 741',
     lambda out: out.count('You are in a forest') == 2 and 'IN LINE' not in out),
    ('long-word', 'a word of more than 16 letters stopped the game',
     ORIGINAL, lambda v: 'SUPERCALIFRAGILISTIC\rI\r', 'SUBSCRIPT OUT OF RANGE\n IN LINE 238',
     lambda out: 'You are carrying: Nothing' in out and 'IN LINE' not in out),
    ('caged-bird', 'GET BIRD, the bird in its cage on the floor: nothing at all',
     START, lambda v: room(v, 25) + thing(v, 7, 0) + thing(v, 8, 0) + thing(v, 10, 25) +
     'LOOK\rGET BIRD\rI\r',
     lambda out: re.search(r'Command: GET BIRD\nCommand: I\nYou are carrying: Nothing', out),
     'Command: GET BIRD\nCage and bird: Taken.'),
    ('bridge-load', 'the plank laid over the fissure was still counted in the load: 9 things, "can\'t carry"',
     START, lambda v: room(v, 15) + 'GET PLANK\r' + room(v, 18) + 'MAKE BRIDGE\r' +
     ''.join(thing(v, k, 18) for k in (1, 2, 3, 4, 8, 11, 16, 17, 18, 20)) + 'GET ALL\r',
     'You can\'t carry that much', lambda out: 'Treasure chest: Taken.' in out and 'carry that much' not in out),
    ('chest', 'the pirate\'s chest came and went 98 turns in 100: seen with LOOK, gone for GET',
     START, lambda v: room(v, 47) + thing(v, 20, 47) + 'LOOK\rGET CHEST\r',
     'I see no Treasure chest here!', 'Treasure chest: Taken.'),
]


def run(prog, typed):
    p = subprocess.run([EXE, '--data', DATA, '--prog', prog, '--raw', '--no-hold', '-Z', '1', '--debug'],
                       input=typed.encode('latin-1'), stdout=subprocess.PIPE, timeout=60)
    out = bytes(b & 0x7f for b in p.stdout).decode('latin-1').replace('\0', '').replace('\r', '')
    return out


def shows(want, out):
    return bool(want(out)) if callable(want) else want in out


def main():
    verbose = '-v' in sys.argv
    found = {}
    bad = 0
    for case, what, recovered, typing, want_orig, want_fixed in CASES:
        results = []
        for label, prog, want in (('recovered', recovered, want_orig), ('fixed', FIXED, want_fixed)):
            if prog not in found:                       # (the recovered one needs no pokes)
                found[prog] = find(prog) if prog != ORIGINAL else {}
            out = run(prog, typing(found[prog]))
            ok = shows(want, out)
            results.append(ok)
            if verbose or not ok:
                print('   %s %-9s %r' % ('' if ok else 'WRONG', label, out[-300:]))
        bad += not all(results)
        print('%-4s %-11s %s' % ('ok' if all(results) else 'FAIL', case, what))
    print('%d of %d fixes shown' % (len(CASES) - bad, len(CASES)))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
