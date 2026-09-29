#!/usr/bin/env python3
"""Random play of adventure4.exe: commands made of the game's own
vocabulary (ADVINIT4.DAT's 562 words), mostly verb + object or a direction,
some SAVE / RESTORE / SUSPEND, answers to its questions (yes, no, junk), and
junk lines, under --seed and a timeout, in a scratch copy of the port.

A run fails on a non-zero exit, anything on stderr (gfortran's run-time
errors), or a timeout.  "Glitch!" lines - the executive's own complaints
about the A-code - are counted and shown, not failed: they are the
original's.

    fuzz.py [runs [commands]]

The glitches seen - Bad WHERE / Bad BITVAL from WAKE or PROD with a place or
a verb, Bad EXEC code -100 after the toad at the gate - were played on the
Prime too (sessions 7 and 8): the original prints them the same way.  A
build with -fcheck=bounds cannot run the game at all: the A-code reads past
OBJVAL at the start (see src\convert.py).
"""
import os
import random
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.abspath(os.path.join(HERE, '..'))
DIRS = ['n', 's', 'e', 'w', 'ne', 'nw', 'se', 'sw', 'u', 'd', 'in', 'out',
        'back', 'look']
ANSWERS = ['y', 'n', 'yes', 'no', 'maybe', '']


def vocabulary():
    v = open(os.path.join(PORT, 'ADVINIT4.DAT'), 'rb').read()
    n = int.from_bytes(v[:2], 'big')
    words, classes = [], []
    for i in range(n):
        w = bytes(b & 0x7F for b in v[2 + 12 * i:14 + 12 * i]).decode().strip()
        k = int.from_bytes(v[2 + 12 * n + 2 * i:4 + 12 * n + 2 * i], 'big')
        words.append(w)
        classes.append((k & 0x7FFF) // 1000)
    verbs = [w for w, c in zip(words, classes) if c == 3]
    things = [w for w, c in zip(words, classes) if c in (1, 2)]
    return verbs, things


def command(r, verbs, things):
    x = r.random()
    if x < 0.35:
        return r.choice(DIRS)
    if x < 0.65:
        return '%s %s' % (r.choice(verbs), r.choice(things))
    if x < 0.80:
        return r.choice(verbs)
    if x < 0.84:
        return r.choice(['save', 'restore', 'suspend', 'score', 'inventory',
                         'quit', 'brief', 'fast', 'normal'])
    if x < 0.94:
        return r.choice(ANSWERS)
    n = r.randint(1, 30)
    return ''.join(r.choice('abcdefghijklmnopqrstuvwxyz ,.%!') for _ in
                   range(n))


def main():
    args = sys.argv[1:]
    runs = int(args[0]) if args else 100
    ncmd = int(args[1]) if len(args) > 1 else 400
    verbs, things = vocabulary()
    work = tempfile.mkdtemp(prefix='adv4fuzz')
    try:
        for f in ('ADVINIT1.DAT', 'ADVINIT2.DAT', 'ADVINIT3.DAT',
                  'ADVINIT4.DAT'):
            shutil.copy(os.path.join(PORT, f), work)
        shutil.copy(os.path.join(PORT, 'adventure4.exe'), work)
        exe = os.path.join(work, 'adventure4.exe')
        bad = glitches = prompts = ended = 0
        for seed in range(runs):
            r = random.Random(seed)
            cmds = ['no'] + [command(r, verbs, things) for _ in range(ncmd)]
            opts = ['--seed', str(seed + 1)] + (['-u'] if seed % 2 else [])
            try:
                p = subprocess.run([exe] + opts,
                                   input=('\n'.join(cmds) + '\n').encode(),
                                   capture_output=True, timeout=120)
            except subprocess.TimeoutExpired:
                bad += 1
                print('seed %d: TIMEOUT' % seed)
                continue
            out = p.stdout.decode('latin-1')
            prompts += out.count('? ')
            if 'You have scored' in out:
                ended += 1
            for l in out.split('\n'):
                if 'Glitch!' in l:
                    glitches += 1
                    print('seed %d: %s' % (seed, l.strip()))
            if p.returncode or p.stderr.strip():
                bad += 1
                print('seed %d: exit %d %s' % (seed, p.returncode,
                                               p.stderr.decode('latin-1')
                                               .strip()[:500]))
        print('%d runs of %d commands: %d prompts read, %d games ended, '
              '%d glitches, %d failed'
              % (runs, ncmd, prompts, ended, glitches, bad))
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
