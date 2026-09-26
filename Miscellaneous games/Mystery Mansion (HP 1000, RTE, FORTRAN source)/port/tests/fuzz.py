#!/usr/bin/env python3
"""Random commands against mmm.exe: no crash, no hang.

Words are the game's own (its 8H word tables, read from the source on the
tape) and the directions.  Each run holds the clock at a different time -
the mystery and the dice come from it - so a failing run repeats exactly:

    fuzz.py [runs] [commands-per-run] [first-seed] [exe]

A run fails if the program exits abnormally, writes to stderr, or has not
finished after 60 seconds.  It runs in a scratch copy (never the real
saves\\).  The game asks again when an answer is not a number, or not YES
or NO, so a run goes on until its input ends; some commands are answers
(0, YES, NO...) to let those loops finish.
"""

import os
import random
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.normpath(os.path.join(HERE, '..'))


def words():
    src = open(os.path.join(PORT, '..', 'src_original', '&MMM.ftn'),
               encoding='latin-1').read()
    w = set()
    for m in re.finditer(r'8H(.{8})', src):
        t = m.group(1).strip()
        if t and re.match(r"^[A-Z']+$", t):
            w.add(t)
    return sorted(w)


def main():
    runs = int(sys.argv[1]) if len(sys.argv) > 1 else 40
    ncmd = int(sys.argv[2]) if len(sys.argv) > 2 else 300
    seed0 = int(sys.argv[3]) if len(sys.argv) > 3 else 1
    exe = sys.argv[4] if len(sys.argv) > 4 else os.path.join(PORT, 'mmm.exe')
    vocab = words()
    common = ['N', 'S', 'E', 'W', 'NORTH', 'SOUTH', 'EAST', 'WEST', 'UP',
              'DOWN', 'LOOK', 'GET', 'TAKE', 'DROP', 'LIST', 'LIGHT',
              'OPEN', 'CLOSE', 'GO', 'AND', 'SCORE', 'HELP']
    answers = ['0', '1', '5', 'NO', 'YES', 'N', 'Y', '']
    work = tempfile.mkdtemp(prefix='mmmfuzz')
    shutil.copy(exe, os.path.join(work, 'mmm.exe'))
    bad = 0
    for r in range(seed0, seed0 + runs):
        rnd = random.Random(r)
        cmds = []
        for _ in range(ncmd):
            k = rnd.random()
            if k < 0.08:
                # answers, so that the game's own retry loops (cartridge
                # number, yes or no) end
                c = rnd.choice(answers)
            elif k < 0.4:
                c = rnd.choice(common)
            elif k < 0.75:
                c = rnd.choice(vocab)
            elif k < 0.95:
                c = rnd.choice(common) + ' ' + rnd.choice(vocab)
            else:
                c = ' '.join(rnd.choice(vocab) for _ in range(rnd.randint(2, 5)))
            cmds.append(c)
        shutil.rmtree(os.path.join(work, 'saves'), ignore_errors=True)
        t = '%02d:%02d:%02d' % (rnd.randrange(24), rnd.randrange(60),
                                rnd.randrange(60))
        args = [os.path.join(work, 'mmm.exe'), '--time', t,
                '--date', '2026-09-19']
        if rnd.random() < 0.3:
            # Wolpert's set-up: a security code for any hour, a name
            args += ['--site', '-u']
            cmds.insert(0, rnd.choice(['FUZZ', 'A PLAYER', '']))
        try:
            p = subprocess.run(args, input='\n'.join(cmds) + '\n',
                               capture_output=True, text=True,
                               encoding='latin-1', timeout=60, cwd=work)
            out, err = p.stdout, p.stderr
            why = None
            if p.returncode != 0:
                why = 'exit %d' % p.returncode
            elif err.strip():
                why = 'stderr'
        except subprocess.TimeoutExpired:
            why, out, err = 'timeout', '', ''
        if why:
            bad += 1
            log = os.path.join(HERE, 'fuzz-fail-%d.txt' % r)
            with open(log, 'w', encoding='latin-1') as f:
                f.write('%s\n%s\n%s\n---\n%s' % (' '.join(args), why, err, out))
            print('run %d (--time %s): %s -> %s' % (r, t, why, log))
    shutil.rmtree(work, ignore_errors=True)
    print('%d runs of %d commands, %d failed' % (runs, ncmd, bad))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
