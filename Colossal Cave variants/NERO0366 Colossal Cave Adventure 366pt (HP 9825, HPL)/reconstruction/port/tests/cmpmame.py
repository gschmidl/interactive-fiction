"""Replay the MAME sessions on the port and compare, byte for byte.

usage: cmpmame.py

For each tests/sessionN.txt there is mameN.txt, what the original program
printed on an HP 9871 at 80 columns when mamerun.py typed those commands on
MAME's hp9825t.  The port gets the same commands through a pipe (so it
prints each command as the 9825 does) at width 80, and must print the same.
The sessions keep to the places where the game draws no random numbers.

restore1.txt / restore2.txt: a game suspended below the grate and restored
(mamerun.py --disk D restore1.txt ...; then --disk D --restore restore2.txt).
The port must match mame_r2.txt with --no-fixes up to the move into the
forest, whose exits are random.
"""
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
EXE = os.path.join(HERE, '..', 'advent.exe')


def commands(name):
    return [c.rstrip('\r\n') for c in open(os.path.join(HERE, name), encoding='ascii')
            if c.strip() and not c.startswith('#')]


def port(cmds, save, extra=()):
    p = subprocess.run([EXE, '-w', '80', '--no-pause', '--saves', save] + list(extra),
                       input='\n'.join(cmds) + '\n', capture_output=True, text=True, encoding='latin-1')
    return p.stdout


def ref(name):
    return open(os.path.join(HERE, name), encoding='latin-1').read().lstrip('\x00\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b\x9c\x9d\x9e\x9fG�')


def main():
    ok = True
    tmp = tempfile.mkdtemp(prefix='nero_cmp_')
    save = os.path.join(tmp, 'ADVSAV')
    n = 1
    while os.path.exists(os.path.join(HERE, 'session%d.txt' % n)):
        want = ref('mame%d.txt' % n).splitlines()
        cmds = commands('session%d.txt' % n)
        got = port(cmds, os.path.join(tmp, 'none'), ['--reflow']).splitlines()
        same = want == got
        ok &= same
        print('session %d: %s (%d lines)' % (n, 'IDENTICAL' if same else 'DIFFERENT', len(want)))
        # without --reflow only the line breaks may differ
        lines = port(cmds, os.path.join(tmp, 'none')).splitlines()
        words = ' '.join(lines).split() == ' '.join(want).split()
        ok &= words
        print('  data lines kept: %s' % ('same words' if words else 'WORDS DIFFER'))
        if not same:
            for i, (a, b) in enumerate(zip(want + [''] * len(got), got + [''] * len(want))):
                if a != b:
                    print('  line %d\n    original: %r\n    port:     %r' % (i + 1, a, b))
                    break
        n += 1
    # suspend and restore, the original's own behaviour (--no-fixes)
    port(commands('restore1.txt') + ['suspend', 'y'], save, ['--no-fixes'])
    got = port(['n'] + commands('restore2.txt'), save, ['--no-fixes']).splitlines()
    want = ref('mame_r2.txt').splitlines()
    cut = want.index('s') + 2          # the line after the move into the forest
    same = got[:cut] == want[:cut]
    ok &= same
    print('restore --no-fixes: %s (first %d lines)' % ('IDENTICAL' if same else 'DIFFERENT', cut))
    sys.exit(0 if ok else 1)


main()
