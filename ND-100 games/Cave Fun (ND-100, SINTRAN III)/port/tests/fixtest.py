"""Show each of the port's fixes: the same typing on the program and game file
as recovered, and as the port ships them.

usage: python tests\\fixtest.py [-v]

Each case sets the scene with --debug pokes (tools\\findvars.py finds where
each build keeps ROOM and its arrays) and says what the original answers and
what the fixed one answers.  -v prints the answers.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
sys.path.insert(0, os.path.join(PORT, 'tools'))
from runport import run  # noqa: E402
from findvars import find  # noqa: E402

ORIGINAL = (os.path.join(PORT, 'build', 'original', 'ADV-INTER-CB-MJ.PROG'),
            os.path.join(PORT, '..', 'src_original', 'CAVE-FUN-MJ.ADV'), 'CAVE-ORIG')
FIXED = (os.path.join(PORT, 'data', 'ADV-INTER-CB-MJ.PROG'),
         os.path.join(PORT, 'data', 'CAVE-FUN-MJ.ADV'), 'CAVE-FUN')


def at(v, room):
    """teleport: the next turn's end describes the room"""
    return ['#poke %o %d.' % (v['ROOM'], room)]


def carry(v, *objs):
    return ['#poke %o 177777' % (v['ADV'] + 351 * 11 + o) for o in objs]


def put(v, obj, room):
    return ['#poke %o %d.' % (v['ADV'] + 351 * 11 + obj, room)]


def flag(v, n, value=1):
    return ['#poke %o %d.' % (v['SF'] + n, value)]


def lamp(v):
    return carry(v, 1) + flag(v, 1)


def no_orc(v):
    """rule 29 (the orc appears) becomes a verb rule: the AUTO rules end there"""
    return ['#poke %o 99.' % (v['ADV'] + 351 * 13 + 29)]


# name: (what it shows, keys after the adventure's name, the original's answer, the fix's)
CASES = [
    ('troll', 'S at the troll\'s bridge, typed alone, walked past him unpaid',
     lambda v: no_orc(v) + lamp(v) + at(v, 113) + ['LOOK', 'S'],
     'SOUTHERN END OF A BRIDGE', 'THE TROLL INSISTS'),
    ('go-n', 'GO N was not understood',
     lambda v: ['GO N'], "I DIDN'T UNDERSTAND", 'FOREST WITH TREES'),
    ('get-unknown', 'after a GET, GET of an unknown word: I SEE NO ... HERE, not WHAT IS ...??',
     lambda v: put(v, 16, 1) + ['GET ROD', 'GET XYZZY'], 'I SEE NO XYZZY HERE', 'WHAT IS XYZZY??'),
    ('drop-rug', 'after a GET, DROP of an unknown word dropped the Persian rug, if carried',
     lambda v: put(v, 16, 1) + carry(v, 35) + ['GET ROD', 'DROP XYZZY'],
     'PERSIAN RUG: DROPPED', 'WHAT IS XYZZY??'),
    ('load-count', 'things a rule gave the player were not counted in the load',
     lambda v: carry(v, 10, 11, 17, 18, 19, 21, 25, 29) + ['N', 'GET LAMP'],
     'OLD BRASS LAMP: TAKEN', 'YOUR LOAD IS TO HEAVY'),
    ('lamp-on-floor', 'a lit lamp lying in a dark room did not stop a fall',
     lambda v: no_orc(v) + put(v, 1, 13) + flag(v, 1) + at(v, 13) + ['LOOK', 'W'],
     'YOU FELL INTO A PIT', 'YOU CANNOT GO THAT WAY'),
    ('load-door', 'LOAD left a door opened since the save open',
     lambda v: carry(v, 5) + at(v, 11) + ['LOOK', 'SAVE "FIXTEST"', 'UNLOCK', 'LOAD FIXTEST', 'LOOK'],
     'LARGE OPEN IRON DOOR', 'LARGE LOCKED IRON DOOR'),
    ('bottle', 'once the plant was 40 m, the bottle filled and emptied itself every turn',
     lambda v: (at(v, 135) + flag(v, 21) + carry(v, 3) + put(v, 2, 0) +
                ['LOOK', 'WATER', 'GET I', 'GET I']),
     'BOTTLE OF WATER', 'EMPTY BOTTLE'),
    ('lock-62', 'LOCK at the iron door by the pit said so and left it open',
     lambda v: no_orc(v) + lamp(v) + carry(v, 12) + at(v, 62) + ['LOOK', 'UNLOCK', 'LOCK', 'N'],
     'LARGE ROOM WITH OPENINGS', 'YOU CANNOT GO THAT WAY'),
]


def last_answer(out):
    parts = out.split('\n > ')
    return parts[-2].split('\n', 1)[1] if len(parts) > 1 and '\n' in parts[-2] else ''


def main():
    verbose = '-v' in sys.argv
    builds = {}
    for label, (prog, adv, name) in (('original', ORIGINAL), ('fixed', FIXED)):
        builds[label] = (prog, adv, name, find(prog, adv, name))
    bad = 0
    for case, what, keys, want_orig, want_fixed in CASES:
        results = []
        for label, want in (('original', want_orig), ('fixed', want_fixed)):
            prog, adv, name, v = builds[label]
            typed = [name] + keys(v) + ['END', 'N']
            out = run(''.join(k + '\r' for k in typed).encode('latin-1'), prog=prog, adv=adv,
                      extra=['--debug'], name=name)
            ans = last_answer(out.split('\n > END')[0] + '\n > ')
            ok = want in ans
            results.append(ok)
            if verbose or not ok:
                print('   %s %-8s %r' % ('' if ok else 'WRONG', label, ans[:200]))
        bad += not all(results)
        print('%-4s %-14s %s' % ('ok' if all(results) else 'FAIL', case, what))
    print('%d of %d fixes shown' % (len(CASES) - bad, len(CASES)))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
