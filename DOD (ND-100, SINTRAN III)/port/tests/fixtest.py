"""Show each of the port's fixes: the same typing, with the same dice, on the
program as recovered and as the port ships it.

usage: python tests\\fixtest.py [-v]

The dice are the uptime's: each case names an --uptime the scene comes up
with (found by trying), and both programs draw the same numbers until they
part.  -v prints the end of every answer.
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
EXE = os.path.join(PORT, 'dod.exe')
ORIGINAL = os.path.join(PORT, 'build', 'original', 'DOD-ENB.PROG')
FIXED = os.path.join(PORT, 'data', 'DOD-ENB.PROG')
DEAD = 'NI [R ALLA D\\DA'

# name: (what it shows, uptime, answers, the recovered program's answer, the fix's);
# an answer is text the output has, or a function of the output
CASES = [
    ('way-out', 'line 3120, the way out, is lost: it ran into what lies below the program (here, a new game)',
     444, ['N', 'F', 'E'],
     lambda out: out.count('***********DOD***********') == 2, 'DU [R UTE I FRIHETEN DU FICK 10 PO[NG'),
    ('numbers', 'crossing the river one by one, every person had the company\'s number',
     444, ['N', 'F', 'E'],
     lambda out: out.count('PERSON NR. 15 ') == 15, lambda out: 'PERSON NR. 1 \\VERLEVDE' in out),
    ('river-twice', 'at the second crossing the dead of the first were counted again',
     117, ['N', 'E', 'H', 'V', 'E', 'H', 'V', 'E', 'H', 'A'],
     lambda out: out.rstrip().endswith('VAD G\\R NI     ?A'), 'DET [R NU  1 PERSONER KVAR'),
    ('dead-trolls', 'killed to the last by the trolls: the game ended without a word',
     143, ['N'] + ['A'] * 6, lambda out: DEAD not in out and out.rstrip().endswith('?A'), DEAD),
    ('dead-harpies', 'the same with the harpies',
     1274, ['N'] + ['A'] * 6, lambda out: DEAD not in out and out.rstrip().endswith('?A'), DEAD),
    ('dead-orcs', 'the same with the orcs',
     104, ['N'] + ['A'] * 6, lambda out: DEAD not in out and out.rstrip().endswith('?A'), DEAD),
    ('medusa', 'going on past Medusa, "five died", and none did',
     0, ['N'] + ['F'] * 5, lambda out: out.count('FEM PERSONER DOG') == 4 and DEAD not in out,
     lambda out: out.count('FEM PERSONER DOG') == 3 and DEAD in out),
]


def run(prog, uptime, answers):
    p = subprocess.run([EXE, '--prog', prog, '--raw', '--no-hold', '--uptime', str(uptime)],
                       input=''.join(a + '\r' for a in answers).encode('latin-1'),
                       stdout=subprocess.PIPE, timeout=30)
    out = bytes(b & 0x7f for b in p.stdout).decode('latin-1').replace('\0', '')
    return re.sub(r'\n+', '\n', out.replace('\r', ''))


def shows(want, out):
    return want(out) if callable(want) else want in out


def main():
    verbose = '-v' in sys.argv
    bad = 0
    for case, what, uptime, answers, want_orig, want_fixed in CASES:
        results = []
        for label, prog, want in (('recovered', ORIGINAL, want_orig), ('fixed', FIXED, want_fixed)):
            out = run(prog, uptime, answers)
            ok = shows(want, out)
            results.append(ok)
            if verbose or not ok:
                print('   %s %-9s %r' % ('' if ok else 'WRONG', label, out[-300:]))
        bad += not all(results)
        print('%-4s %-12s %s' % ('ok' if all(results) else 'FAIL', case, what))
    print('%d of %d fixes shown' % (len(CASES) - bad, len(CASES)))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
