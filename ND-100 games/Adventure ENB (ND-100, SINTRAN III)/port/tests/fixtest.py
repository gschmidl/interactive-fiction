"""Show each of the port's fixes: the same typing on the program as recovered
and as the port ships it.

usage: python tests\\fixtest.py [-v]

The program as recovered does not start (the first case); the others use
build\\start\\, the program as recovered with only its error handlers mended.
Each case sets the scene with --debug pokes (tools\\findvars.py finds where
each build keeps the player, the monsters and the rest), in the reference
world (-Z) unless it says otherwise, and says what the recovered program
does and what the fixed one does.  -v prints the end of every answer.
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
sys.path.insert(0, os.path.join(PORT, 'tools'))
from findvars import outside, real  # noqa: E402

EXE = os.path.join(PORT, 'advenb.exe')
ORIGINAL = os.path.join(PORT, 'build', 'original', 'ADVENTURE-ENB.PROG')
START = os.path.join(PORT, 'build', 'start', 'ADVENTURE-ENB.PROG')
FIXED = os.path.join(PORT, 'data', 'ADVENTURE-ENB.PROG')
ARROW = {'{N}': '\x1bA', '{S}': '\x1bB', '{E}': '\x1bC', '{W}': '\x1bD'}
HANG = 'hangs'


def poke(a, words):
    return '#poke %o %s\r' % (a, ' '.join('%o' % w for w in words))


def no_monsters(v):
    return poke(v['PAMON'], [0] * 31) + poke(v['OFMON'], [0] * 31)


def at(v, ns, ew=None):
    return poke(v['NS'], real(ns)) + ('' if ew is None else poke(v['EW'], real(ew)))


def strong(v):
    return poke(v['FORSV'], real(1000)) + poke(v['SKYDD'], real(1000)) + poke(v['STRENGTH'], real(10000))


def down(v):
    """into the caves, with nothing to fear there"""
    return no_monsters(v) + strong(v) + at(v, *v['cave']) + 'N'


def giant(v, ns, ew):
    """monster 30 (a giant, MORMON(30) 5) on the player"""
    return poke(v['PAMON'] + 30, [ns]) + poke(v['OFMON'] + 30, [ew])


def keys(*parts):
    s = ''.join(parts)
    for k, x in ARROW.items():
        s = s.replace(k, x)
    return s


# name: (what it shows, clock, keys (v -> str), the recovered program's answer, the fix's);
# an answer is text the output has, a function of the output, or HANG
CASES = [
    ('start', 'with no instruction files, the error handler looped for ever',
     ['-Z', '1'], lambda v: 'N\r', HANG, 'V{nta ett tag tack'),
    ('row-86', 'RND*85+2 went to 86, the map to 85: a river there stopped the game',
     ['--uptime', '300'], lambda v: 'N\r?\r', 'DIMENSION OUT OF RANGE', 'Dina koordinater'),
    ('road-edge', 'on a road, the second step north went off the map, to 86',
     ['-Z', '1'], lambda v: keys('N\r', no_monsters(v), at(v, 84), '{N}?\r'),
     'koordinater {r  86', 'koordinater {r  85'),
    ('water-13', 'the 13th thing taken stopped the game (HA$ has 12)',
     ['-Z', '1'], lambda v: keys('N\r', no_monsters(v), '{E}', 'T' * 13, '?\r'),
     'DIMENSION OUT OF RANGE', 'Du kan inte b{ra mer'),
    ('hire-11', 'hiring more than 10 men stopped the game (LEJA has 10)',
     ['-Z', '1'], lambda v: keys('N\r', no_monsters(v), at(v, *v['town']), 'L11\r', '1\r' * 11),
     'DIMENSION OUT OF RANGE', lambda out: out.count('Hur m}nga vill du leja') == 2),
    ('hired-man', 'a fight with a hired man: he died at every blow, and never all the way',
     ['-Z', '1'], lambda v: keys('N\r', no_monsters(v), at(v, *v['town']), 'L1\r60\r',
                                 giant(v, *v['town']), 'V', '\r' * 30),
     lambda out: out.count('En man d|dad') > 1 and 'ensam kvar' not in out,
     lambda out: out.count('En man d|dad') == 1 and 'Du {r ensam kvar' in out),
    ('cave-items', 'with all seven things in the caves taken, the next room hung the game',
     ['-Z', '1'], lambda v: keys('N\r', down(v), poke(v['SPE$(1)'], [0] * 14), '{N}{S}{E}{W}{N}{S}V'),
     HANG, lambda out: out.rstrip('\r\n').endswith('ORDER:Vila\r\nORDER:') or 'ORDER:Vila' in out),
    ('princess', 'the princess, at 15,1, would only follow from 20,1',
     ['-Z', '1'], lambda v: keys('N\r', down(v), poke(v['G3'], real(15)), poke(v['G4'], real(1)), 'F'),
     'Felaktigt kommando', 'Prinsessan f|ljer dig'),
]


def run(prog, clock, typed, timeout=20):
    args = [EXE, '--prog', prog, '--raw', '--no-hold', '--debug'] + clock
    try:
        p = subprocess.run(args, input=typed.encode('latin-1'), stdout=subprocess.PIPE, timeout=timeout)
        out, hung = p.stdout, False
    except subprocess.TimeoutExpired as e:
        out, hung = e.stdout or b'', True
    return bytes(b & 0x7f for b in out).decode('latin-1').replace('\0', ''), hung


def shows(want, out, hung):
    if want == HANG:
        return hung
    if hung:
        return False
    return want(out) if callable(want) else want in out


def main():
    verbose = '-v' in sys.argv
    import tempfile
    work = tempfile.mkdtemp(prefix='advenb-fixtest-')
    found = {START: outside(START, work), FIXED: outside(FIXED, work)}
    from findvars import inside
    for prog in (START, FIXED):
        inside(prog, work, found[prog])
    bad = 0
    for case, what, clock, typing, want_orig, want_fixed in CASES:
        results = []
        for label, prog, want in (('recovered', ORIGINAL if case == 'start' else START, want_orig),
                                  ('fixed', FIXED, want_fixed)):
            v = found.get(prog, found[START])
            out, hung = run(prog, clock, typing(v), timeout=5 if HANG in (want_orig, want_fixed) else 30)
            ok = shows(want, out, hung)
            results.append(ok)
            if verbose or not ok:
                tail = re.sub(r'\n+', '\n', out.replace('\r', ''))[-300:]
                print('   %s %-9s %s%r' % ('' if ok else 'WRONG', label, '(hung) ' if hung else '', tail))
        bad += not all(results)
        print('%-4s %-11s %s' % ('ok' if all(results) else 'FAIL', case, what))
    print('%d of %d fixes shown' % (len(CASES) - bad, len(CASES)))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
