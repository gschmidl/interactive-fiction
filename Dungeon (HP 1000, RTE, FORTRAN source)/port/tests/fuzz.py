#!/usr/bin/env python3
"""Random commands against dungeon.exe: no crash, no hang.

Words are the game's own - its vocabulary tables, read from the source on
the tape (#DUNGB's DATA statements, where a word is kept in pieces of three
letters) - with directions, and now and then an answer (Y, N, a file name)
for the questions the game asks.  Each run holds the clock at a different
time, and the dice are seeded from it, so a failing run repeats exactly:

    fuzz.py [runs] [commands-per-run] [first-seed] [exe]

A run fails if the program exits abnormally, writes to stderr, or has not
finished after 60 seconds.  It runs in a scratch copy (never the real
saves\\).
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
SQ = chr(39)


def words():
    src = open(os.path.join(PORT, '..', 'src_original', '#DUNGB.ftn'),
               encoding='latin-1').read()
    w = set()
    for m in re.finditer(r'DATA\s+[ABDOPV]VOK\w*\s*/(.*?)/', src, re.S):
        body = re.sub(r'\n\s{5}\S', '', m.group(1))
        # a word is two slots of three letters, the second 0 if unused
        first = None
        for tok in re.findall(r"'[^']*'|[^,\s]+", body):
            if tok.startswith(SQ):
                if first is None:
                    first = tok[1:-1].strip()
                else:
                    w.add(first + tok[1:-1].strip())
                    first = None
            else:
                if first is not None:
                    w.add(first)
                first = None
        if first is not None:
            w.add(first)
    return sorted(x for x in w if re.match(r'^[A-Z0-9-]+$', x))


# the game's own debugger, GDT: move the player to a room (AH), give an
# object (TK) - so that the random commands are tried all over the dungeon
def gdt(rnd):
    cmds = ['GUARDIAN', 'CHRIS,QUETZAL,27']
    if rnd.random() < 0.7:
        cmds += ['AH', str(rnd.randint(1, 150))]
    if rnd.random() < 0.5:
        cmds += ['TK', str(rnd.randint(1, 160))]
    return cmds + ['EX']


def main():
    runs = int(sys.argv[1]) if len(sys.argv) > 1 else 40
    ncmd = int(sys.argv[2]) if len(sys.argv) > 2 else 300
    seed0 = int(sys.argv[3]) if len(sys.argv) > 3 else 1
    exe = sys.argv[4] if len(sys.argv) > 4 else os.path.join(PORT,
                                                             'dungeon.exe')
    vocab = words()
    common = ['N', 'S', 'E', 'W', 'NE', 'NW', 'SE', 'SW', 'U', 'D', 'IN',
              'OUT', 'LOOK', 'TAKE', 'DROP', 'OPEN', 'CLOSE', 'READ',
              'INVENTORY', 'SCORE', 'TAKE ALL', 'DROP ALL', 'LIGHT LAMP']
    answers = ['Y', 'N', 'YES', 'NO', 'GAME1', 'GAME2', '']
    work = tempfile.mkdtemp(prefix='dngfuzz')
    shutil.copy(exe, os.path.join(work, 'dungeon.exe'))
    for f in ('@DUNGN', '@DUNGT', '@DUNGI'):
        shutil.copy(os.path.join(os.path.dirname(exe), f),
                    os.path.join(work, f))
    bad = 0
    for r in range(seed0, seed0 + runs):
        rnd = random.Random(r)
        cmds = []
        for _ in range(ncmd):
            k = rnd.random()
            if k < 0.02:
                cmds.extend(gdt(rnd))
                continue
            if k < 0.06:
                c = rnd.choice(answers)
            elif k < 0.40:
                c = rnd.choice(common)
            elif k < 0.70:
                c = rnd.choice(vocab)
            elif k < 0.95:
                c = rnd.choice(vocab) + ' ' + rnd.choice(vocab)
            else:
                c = ' '.join(rnd.choice(vocab)
                             for _ in range(rnd.randint(3, 6)))
            cmds.append(c)
        shutil.rmtree(os.path.join(work, 'saves'), ignore_errors=True)
        t = '%02d:%02d:%02d' % (rnd.randrange(24), rnd.randrange(60),
                                rnd.randrange(60))
        args = [os.path.join(work, 'dungeon.exe'), '--time', t,
                '--date', '1982-11-18']
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
                f.write('%s\n%s\n%s\n---\n%s' % (' '.join(args), why, err,
                                                  out))
            print('run %d (--time %s): %s -> %s' % (r, t, why, log))
    shutil.rmtree(work, ignore_errors=True)
    print('%d runs of %d commands, %d failed' % (runs, ncmd, bad))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
