#!/usr/bin/env python3
"""Random commands against adv.exe: no glitch, no crash, no hang.

Words come from the game's own vocabulary (the symbol records of
advi.dat).  Each run holds the clock at a different time, which is all the
dice depend on, so a failing run can be repeated exactly:

    fuzz.py [runs] [commands-per-run] [first-seed]

A run fails if the interpreter reports a "Glitch!" (a line of its own;
the NEWS text mentions the word too),
exits abnormally, or has not finished after 60 seconds.  Runs use a
scratch copy of the program and database (never the real saves).
"""

import os
import random
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.normpath(os.path.join(HERE, '..'))


def vocabulary(path):
    with open(path, 'rb') as f:
        d = f.read()
    assert d[:8] == b'CPVKEYED'
    n = int.from_bytes(d[8:12], 'big')
    p = 12
    words = []
    for _ in range(n):
        kl = d[p]
        key = int.from_bytes(d[p + 1:p + 1 + kl], 'big')
        ln = int.from_bytes(d[p + 1 + kl:p + 3 + kl], 'big')
        data = d[p + 3 + kl:p + 3 + kl + ln]
        p += 3 + kl + ln
        if 9001000 <= key < 9100000:
            for i in range(0, len(data) - 7, 8):
                w = data[i:i + 6].decode('latin-1').strip()
                if w and all(32 < ord(c) < 127 for c in w):
                    words.append(w)
    return words


def main():
    runs = int(sys.argv[1]) if len(sys.argv) > 1 else 40
    ncmd = int(sys.argv[2]) if len(sys.argv) > 2 else 300
    seed0 = int(sys.argv[3]) if len(sys.argv) > 3 else 1
    words = vocabulary(os.path.join(PORT, 'advi.dat'))
    common = ['n', 's', 'e', 'w', 'u', 'd', 'ne', 'nw', 'se', 'sw', 'in',
              'out', 'look', 'inventory', 'take', 'drop', 'lamp', 'on',
              'off', 'yes', 'no', 'xyzzy', 'plugh', 'score']
    work = tempfile.mkdtemp(prefix='advfuzz')
    for f in ('adv.exe', 'advt.dat', 'advi.dat'):
        shutil.copy(os.path.join(PORT, f), work)
    bad = 0
    for r in range(seed0, seed0 + runs):
        rnd = random.Random(r)
        cmds = []
        for _ in range(ncmd):
            k = rnd.random()
            if k < 0.45:
                c = rnd.choice(common)
            elif k < 0.8:
                c = rnd.choice(words)
            else:
                c = rnd.choice(words) + ' ' + rnd.choice(words)
            cmds.append(c.lower() if rnd.random() < 0.5 else c)
        shutil.rmtree(os.path.join(work, 'saves'), ignore_errors=True)
        t = '%02d:%02d:%02d.%03d' % (rnd.randrange(24), rnd.randrange(60),
                                     rnd.randrange(60), rnd.randrange(1000))
        args = [os.path.join(work, 'adv.exe'), '-u', '--time', t,
                '--date', '2026-09-19']
        try:
            p = subprocess.run(args, input='\n'.join(cmds) + '\n',
                               capture_output=True, text=True,
                               encoding='latin-1', timeout=60, cwd=work)
            out = p.stdout
            why = None
            if any(l.startswith(' Glitch!') for l in out.splitlines()):
                why = 'glitch'
            elif p.returncode != 0:
                why = 'exit %d' % p.returncode
        except subprocess.TimeoutExpired:
            why = 'timeout'
            out = ''
        if why:
            bad += 1
            log = os.path.join(HERE, 'fuzz-fail-%d.txt' % r)
            with open(log, 'w', encoding='latin-1') as f:
                f.write('%s\n%s\n---\n%s' % (' '.join(args), why, out))
            print('run %d (--time %s): %s -> %s' % (r, t, why, log))
    shutil.rmtree(work, ignore_errors=True)
    print('%d runs of %d commands, %d failed' % (runs, ncmd, bad))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
