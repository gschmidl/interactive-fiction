#!/usr/bin/env python3
"""Regression tests for the Pohl and Daimler ports.

    python tests\\regress.py [-v]

The options, the end of input, SUSPEND and --restore, and each fix with and
without --no-fixes; then a short tests\\fuzz.py and, if DOSBox 0.74-3 is
installed, a short tests\\crossdos.py against Daimler's own DOS program.
Both programs seed their dice with 511 at every start, so a command list
always replays the same game.  -v prints every transcript.
"""
import os
import random
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import common as C  # noqa: E402
import crossdos  # noqa: E402
import dosref  # noqa: E402
import fuzz  # noqa: E402

VERBOSE = '-v' in sys.argv[1:]
failed = []
# into the cave with the lamp lit; Pohl starts at the road, Daimler inside
START = {'pohl': ['n', 'in'], 'daimler': ['n']}
INTO_CAVE = ['take lamp', 'take keys', 'out', 's', 's', 's', 'unlock grate', 'lamp on', 'd']
MOVES = ['n', 's', 'e', 'w', 'ne', 'nw', 'se', 'sw', 'u', 'd', 'back']
# a random walk from there on which a dwarf blocks the way (found by trial):
# the walk's seed and the number of its steps
BLOCKED = {'pohl': (6, 83), 'daimler': (8, 245)}


def report(name, ok, text=''):
    print('%-58s %s' % (name, 'ok' if ok else 'FAILED'))
    if not ok:
        failed.append(name)
    if text and (VERBOSE or not ok):
        print('    ' + text.replace('\n', '\n    '))


def game(variant, name, lines, parts, args=(), absent=(), code=0, cwd=None):
    out, err, rc = C.run(variant, lines, args, cwd=cwd or tempfile.mkdtemp())
    miss, at = None, 0
    for p in parts:
        i = out.find(p, at)
        if i < 0:
            miss = p
            break
        at = i + len(p)
    bad = [a for a in absent if a in out]
    ok = miss is None and not bad and rc == code and not err
    report('%s: %s' % (variant, name), ok, out + err +
           ('\n[missing: %s]' % miss if miss else '') +
           ('\n[should not be there: %s]' % bad if bad else '') +
           ('\n[exit %d, not %d]' % (rc, code) if rc != code else ''))
    return out


def t_options(v):
    out, err, rc = C.run(v, [], ['--help'])
    report('%s: --help lists the options, exit 0' % v, rc == 0 and out.startswith('usage: advent'), out)
    out, err, rc = C.run(v, [], ['-h'])
    report('%s: -h the same' % v, rc == 0 and out.startswith('usage: advent'), out)
    out, err, rc = C.run(v, ['n', 'quit', 'y'], ['--frob'])
    report('%s: --frob refused, exit 2' % v, rc == 2 and 'unknown option' in err, out + err)
    game(v, 'the authors\' own -x still says "unknown flag"', ['n', 'quit', 'y'],
         ['unknown flag: x', 'Score:'], ['-x'])


def t_end_of_input(v):
    out = game(v, 'end of input at a command: the end, exit 0', START[v] + ['look'], ['>'])
    game(v, 'end of input at the first question: the same', [], ['Would you like instructions?'])


def t_save_restore(v):
    cwd = tempfile.mkdtemp()
    game(v, 'SUSPEND saves NAME.adv and stops', START[v] + ['take lamp', 'suspend', 'pocket'],
         ['What do you want to name the saved game?', 'OK -- "C" you later...'], cwd=cwd)
    report('%s: ... the file is there' % v, os.path.exists(os.path.join(cwd, 'pocket.adv')))
    game(v, '--restore: the lamp is still carried', ['pocket', 'inventory', 'quit', 'y'],
         ['What is the name of the saved game?', 'Brass lantern', 'Score:'], ['--restore'], cwd=cwd)
    game(v, '-r: the same', ['pocket', 'inventory', 'quit', 'y'],
         ['Brass lantern', 'Score:'], ['-r'], cwd=cwd)


def t_fix1(v):
    seed, steps = BLOCKED[v]
    rng = random.Random(seed)
    lines = START[v] + INTO_CAVE + [rng.choice(MOVES) for _ in range(steps + 1)] + ['quit', 'y']
    game(v, 'Fix 1: a dwarf with a big knife blocks the way back', lines,
         ['A little dwarf with a big knife blocks your way.'])
    game(v, '... --no-fixes: never, as the precedence bug had it', lines, [],
         ['--no-fixes'], absent=['blocks your way'])


def t_daimler_fixes():
    v = 'daimler'
    game(v, 'Fix 3: READ LAMP says the message Daimler meant', ['n', 'read lamp', 'quit', 'y'],
         ["I'm afraid I don't understand.", 'Score:'])
    game(v, '... --no-fixes: Turbo C\'s signed char, text from the wrong place',
         ['n', 'read lamp', 'quit', 'y'],
         ['e an object are attempting something beyond their', 'Score:'], ['--no-fixes'])
    game(v, 'Fix 4: LOG LAMP is not understood', ['n', 'log lamp', 'quit', 'y'],
         ["I don't understand that!", 'Score:'])
    game(v, '... --no-fixes: "Fatal error number 39", exit 1', ['n', 'log lamp', 'quit', 'y'],
         ['Fatal error number 39'], ['--no-fixes'], absent=['Score:'], code=1)


def main():
    for v in C.VARIANTS:
        t_options(v)
        t_end_of_input(v)
        t_save_restore(v)
        t_fix1(v)
    t_daimler_fixes()
    print()
    sys.argv = ['fuzz.py', '10']
    if fuzz.main() != 0:
        failed.append('fuzz')
    if os.path.exists(dosref.DOSBOX):
        sys.argv = ['crossdos.py', '4']
        if crossdos.main() != 0:
            failed.append('crossdos')
    else:
        print('crossdos: skipped, no %s' % dosref.DOSBOX)
    print()
    print('%d FAILED: %s' % (len(failed), ', '.join(failed)) if failed else 'all passed')
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
