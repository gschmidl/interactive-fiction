#!/usr/bin/env python3
"""Regression tests for the KNUT0350 port.

    python tests\\regress.py [-v]

Each check runs advent.exe on a list of input lines, with --seed so that
the dice are fixed.  At the end tests\\win.py and a short tests\\fuzz.py
run too.  -v prints every transcript.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import common as C  # noqa: E402
import fuzz  # noqa: E402
import win  # noqa: E402

VERBOSE = '-v' in sys.argv[1:]
failed = []


def report(name, ok, text=''):
    print('%-50s %s' % (name, 'ok' if ok else 'FAILED'))
    if not ok:
        failed.append(name)
    if text and (VERBOSE or not ok):
        print('    ' + text.replace('\n', '\n    '))


def game(name, lines, parts, args=(), absent=(), code=0):
    out, err, rc = C.run(lines, ['--seed', '5'] + list(args))
    miss = None
    at = 0
    for p in parts:
        i = out.find(p, at)
        if i < 0:
            miss = p
            break
        at = i + len(p)
    bad = [a for a in absent if a in out]
    ok = miss is None and not bad and rc == code
    report(name, ok, out + err + ('\n[missing: %s]' % miss if miss else '') +
           ('\n[should not be there: %s]' % bad if bad else '') +
           ('\n[exit %d, not %d]' % (rc, code) if rc != code else ''))
    return out


def t_options():
    out, err, rc = C.run([], ['--help'])
    report('--help: the options, exit 0', rc == 0 and out.startswith('usage: advent'), out + err)
    out, err, rc = C.run([], ['-h'])
    report('-h: the same', rc == 0 and out.startswith('usage: advent'), out + err)
    for bad in (['--frob'], ['--seed'], ['--seed', 'x'], ['--seed=5x']):
        out, err, rc = C.run(['no', 'quit', 'y'], bad)
        report('%s: refused, exit 2' % ' '.join(bad), rc == 2 and 'advent:' in err, out + err)
    game('--seed=5, --no-fixes, --debug: accepted', ['no', 'quit', 'y'],
         ['Welcome to Adventure!!', 'You scored'], ['--seed=5', '--no-fixes', '--debug'])
    # NORTH in the forest is a toss of a coin: forest again, or the woods by
    # the road (and north from there is the road).  Above ground nothing
    # else rolls the dice.
    walk = ['no'] + ['n'] * 40 + ['quit', 'y']
    a = C.run(walk, ['--seed', '11'])[0]
    b = C.run(walk, ['--seed', '11'])[0]
    c = C.run(walk, ['--seed', '12'])[0]
    report('--seed N: the same game every time', a == b, a)
    report('... and another N, another game', a != c)


def t_end_of_input():
    out = game('end of input at a command: ends, exit 0', ['no', 'look'],
               ['end of a road', 'end of a road'])
    report('... quietly, with a new line', out.endswith('* \n'), out)
    game('end of input at a yes/no question: the same', [], ['Would you like instructions?'])
    game('end of input at "really quit?": the same', ['no', 'quit'],
         ['Do you really want to quit now?'], absent=['You scored'])


def t_long_lines():
    line = 'look' + ' ' * 70 + 'xyzzy'
    game('Fix 3: the rest of a long line is dropped', ['no', line, 'quit', 'y'],
         ['I will repeat the', 'You scored'], absent=['Nothing happens.'])
    game('... --no-fixes: it is the next command, as in 1999',
         ['no', line, 'quit', 'y'], ['I will repeat the', 'Nothing happens.', 'You scored'],
         ['--no-fixes'])
    word = 'x' * 71
    for args in ([], ['--no-fixes']):
        game('a 71-letter word: one word, nothing read past it %s' % ' '.join(args),
             ['no', word, 'quit', 'y'], ["Sorry, I don't know the word", 'You scored'],
             args, absent=['Please stick to 1- and 2-word commands.'])
        # the program's strings end at a NUL, but fgets reads on to the
        # newline: the line is "look", and the next line is the next command
        game('a NUL in a line: the line ends there %s' % ' '.join(args),
             ['no', 'look\0xyzzy', 'quit', 'y'],
             ['I will repeat the', 'Do you really want to quit now?', 'You scored'],
             args, absent=['Nothing happens.'])


def t_say():
    for args in ([], ['--no-fixes']):
        game('SAY an unknown word: "Okay", no read before the table %s' % ' '.join(args),
             ['no', 'say blorp', 'say xyzzy', 'quit', 'y'],
             ['Okay, "blorp".', 'Nothing happens.', 'You scored'], args)


def t_ranks():
    for bonus, fixed, orig in ((3, ['rank amateur', 'you need 1 more point.'],
                                ['novice class adventurer', 'you need 65 more points.']),
                               (317, ['Master Adventurer Class A.', 'you need 1 more point.'],
                                ['Adventure Grandmaster!', 'would be a neat trick!'])):
        lines = ['no', '#set bonus %d' % bonus, 'quit', 'y']
        score = 'You scored %d points' % (32 + bonus)
        game('Fix 1: %d points ranks as Woods ranked it' % (32 + bonus), lines,
             [score] + fixed, ['--debug'])
        game('... --no-fixes: as Knuth ranked it', lines, [score] + orig,
             ['--debug', '--no-fixes'])


def t_closing_liquid():
    lines = ['no', 'in', 'take bottle', '#set tally 0',
             '#put %d -1' % C.OBJ['LAMP'], '#prop %d 1' % C.OBJ['LAMP'],
             '#loc %d' % C.LOC['emist'], '#set clock1 1', 'inventory',
             '#set clock2 1', 'inventory', '#show']
    game('Fix 2: a bottle of water carried as the cave closes',
         lines, ['closing soon.', 'The cave is now closed', '# loc %d, holding 0,' % C.LOC['neend']],
         ['--debug'])
    game('... --no-fixes: the count of things carried goes to -1',
         lines, ['The cave is now closed', '# loc %d, holding -1,' % C.LOC['neend']],
         ['--debug', '--no-fixes'])


def t_debug_off():
    game('without --debug, # commands are only words', ['no', '#show', 'quit', 'y'],
         ["Sorry, I don't know the word", 'You scored'], absent=['# loc'])


def main():
    t_options()
    t_end_of_input()
    t_long_lines()
    t_say()
    t_ranks()
    t_closing_liquid()
    t_debug_off()
    print()
    sys.argv = ['win.py']
    if win.main() != 0:
        failed.append('win')
    sys.argv = ['fuzz.py', '20']
    if fuzz.main() != 0:
        failed.append('fuzz')
    print()
    print('%d FAILED: %s' % (len(failed), ', '.join(failed)) if failed else 'all passed')
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
